//===--- RewriteObjC.cpp - Playground for the code rewriter ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Hacks and fun related to the code rewriter.
//
//===----------------------------------------------------------------------===//

#include "clang/ASTProcessing/InvariantIf.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/ModificationNoteBuilder.h"
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/DeclCXX.h"    
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
struct CollectReferredDecls : public StmtVisitor<CollectReferredDecls, bool>
{
  llvm::DenseSet<Decl*>&  m_ReferredDecls;
  Stmt*                   m_StmtExclude;
  CollectReferredDecls(llvm::DenseSet<Decl*>& referredDecls, Stmt* stmtToExclude) :
    m_ReferredDecls(referredDecls), 
    m_StmtExclude(stmtToExclude)
  {}

  bool VisitDeclRefExpr(DeclRefExpr* Node)
  {
    if (m_StmtExclude == Node)
    {
      return true;
    }
    m_ReferredDecls.insert(Node->getDecl());
    return false;
  }

  bool VisitDeclStmt(DeclStmt* Node)
  {
    if (m_StmtExclude == Node)
    {
      return true;
    }
    std::for_each(Node->decl_begin(), Node->decl_end(), 
      std::bind(&insertValue<Decl*>, &m_ReferredDecls, _1));
    return false;
  }

  bool VisitStmt(Stmt* Node)
  {
    return m_StmtExclude == Node;
  }
};


//--------------------------------------------------------- 
struct GenerateSplit : StmtEditor
{
  ForStmt* m_ForNode;

  explicit GenerateSplit(StmtEditor& editor, ForStmt* forNode) : 
    StmtEditor(editor), m_ForNode(forNode) {}


  Stmt* getThenElse(IfStmt* Node, bool bElsePart)
  {
    return bElsePart ? Node->getElse() : Node->getThen();
  }

  Stmt* getThenElse(ConditionalOperator* Node, bool bElsePart)
  {
    return bElsePart ? Node->getFalseExpr() : Node->getTrueExpr();
  }

  template<class T>
  void replaceEqualIfs(ForStmt* forNode, const std::string& conditionExpr, bool bElsePart)
  {
    std::string compareExpr;
    for (stmt_iterator<T> i = stmt_ibegin(forNode->getBody()), 
         e = stmt_iend(forNode->getBody()); i != e;)
    {
      getAstAsString(*this, i->getCond(), compareExpr);
      if (conditionExpr == compareExpr)
      {
        Stmt* replaceNode = getThenElse(*i, bElsePart);
        if (replaceNode)
        {
          replaceStatement(*i, replaceNode);
        }
        else
        {
          removeStmt(*i);
        }
        i = stmt_ibegin(forNode->getBody());
      }
      else
      {
        ++i;
      }
    }
  }

  void replaceAllEqualIfs(ForStmt* forNode, const std::string& conditionExpr, bool bElsePart)
  {
    replaceEqualIfs<IfStmt>(forNode, conditionExpr, bElsePart);
    replaceEqualIfs<ConditionalOperator>(forNode, conditionExpr, bElsePart);
  }

  void getSubExprs(Stmt* ifNode, Expr*& cond, Stmt*& trueStmt, Stmt*& falseStmt)
  {
    if (IfStmt* IS = dyn_cast<IfStmt>(ifNode))
    {
      cond = IS->getCond();
      trueStmt = IS->getThen();
      falseStmt = IS->getElse();
    }
    else if (ConditionalOperator* CO = dyn_cast<ConditionalOperator>(ifNode))
    {
      cond = CO->getCond();
      trueStmt = CO->getTrueExpr();
      falseStmt = CO->getFalseExpr();
    }
    else
    {
      assert(0 && "inevitable crash");
    }
  }

  Stmt* split(Stmt* ifNode, const char* reason, boost::logic::tribool conditionResult)
  {
    std::string conditionExpr;
    Expr *condExpr;
    Stmt *trueStmt, *falseStmt;
    getSubExprs(ifNode, condExpr, trueStmt, falseStmt);
    getAstAsString(*this, condExpr, conditionExpr);
    if (!boost::logic::indeterminate(conditionResult))
    {
      attachComment(m_ForNode, reason);
      replaceAllEqualIfs(m_ForNode, conditionExpr, !conditionResult); 
      return m_ForNode;
    }

    StmtCloneMapping cloneMapping;
    ForStmt* elseForStmt = cloneStmtTree(*this, m_ForNode, &cloneMapping);
    Stmt* ifNodeCloned = cloneMapping.m_StmtMapping[ifNode];
    assert(ifNodeCloned && "ifNode must be part of the for loop");
    Stmt* enclosingIf = replaceStatement(m_ForNode, If_(condExpr, m_ForNode, elseForStmt));
    attachComment(enclosingIf, reason);
    
    {
      attachComment(m_ForNode, "if-splitted loop, then condition");
      llvm::df_iterator<Stmt*> e = llvm::df_end(cast<Stmt>(m_ForNode)),
                               i = std::find(llvm::df_begin(cast<Stmt>(m_ForNode)), e, ifNode);
      assert(i != e && "ifNode must be part of the for loop");
      replaceStatement(*i, trueStmt);
      replaceAllEqualIfs(m_ForNode, conditionExpr, false); 
    }

    {
      attachComment(elseForStmt, "if-splitted loop, else condition");
      llvm::df_iterator<Stmt*> e = llvm::df_end(cast<Stmt>(elseForStmt)),
                               i = std::find(llvm::df_begin(cast<Stmt>(elseForStmt)), e, ifNodeCloned);
      assert(i != e && "ifNodeCloned must be part of the cloned for loop");
      if (falseStmt != 0)
      {
        replaceStatement(*i, falseStmt);
      }
      else
      {
        removeStmt(*i);
      }
      replaceAllEqualIfs(elseForStmt, conditionExpr, true); 
    }
    return enclosingIf;
  }

  Stmt* doSplit(Stmt* ifNode, const char* pSplitReason, boost::logic::tribool conditionResult)
  {
    assert(isa<IfStmt>(ifNode) || isa<ConditionalOperator>(ifNode));
    Stmt* enclosingIf = split(ifNode, pSplitReason, conditionResult);
    flattenBlocks(*this, enclosingIf, true);
    removeStmtsAfterBreakAndContinue(*this, enclosingIf);
    ModificationNote(ifNode, pSplitReason);
    return enclosingIf;
  }
};

//--------------------------------------------------------- 
struct FindAndSplitIfs : public StmtVisitor<FindAndSplitIfs, unsigned>, StmtEditor
{
  ForStmt* m_ForNode;
  llvm::DenseSet<Stmt*>&  m_SplittedIfs;
  bool                    m_bOnlyPragmaMarked;

  enum { TRAVERSE, SKIP_CHILDREN, SPLITTED };

  explicit FindAndSplitIfs(StmtEditor& editor, ForStmt* Node, 
      llvm::DenseSet<Stmt*>& splittedIfs, bool bOnlyPragmaMarked) : 
    StmtEditor(editor), m_ForNode(Node), m_SplittedIfs(splittedIfs),
    m_bOnlyPragmaMarked(bOnlyPragmaMarked)
  {}

  bool generate()
  {
    for (llvm::df_iterator<Stmt*> i = llvm::df_begin(m_ForNode->getBody()), 
         e = llvm::df_end(m_ForNode->getBody()); i != e;)
    {
      switch (Visit(*i))
      {
        case SPLITTED:
          return true;      
        case SKIP_CHILDREN:
          i.skipChildren();
          break;
        case TRAVERSE:
          ++i;
          break;
        default:
          assert(0);
      }
    }
    return false;
  }

  bool testConditionInvariance(const llvm::DenseSet<Decl*>& localReferredDecls, Expr* Node)
  {
    for (stmt_iterator<DeclRefExpr> i = stmt_ibegin(Node), 
         e = stmt_iend(Node); i != e; ++i)
    {
      VarDecl* VD = dyn_cast<VarDecl>(i->getDecl());
      if (VD)
      {
        if (VD->getType().isVolatileQualified() || 
            (!VD->hasLocalStorage()) ||
            localReferredDecls.count(VD) != 0)
        {
          return false;
        }
      }    
    }
    return true;
  }

  unsigned VisitIfStmt(IfStmt* Node)
  {
    if (m_SplittedIfs.count(Node))
    {    
      return TRAVERSE;
    }

    const char* pSplitReason = findAttachedPragma(Node, "condition", "invariant") != 0 ? 
      "if-condition moved outside the loop due to annotated pragma" : 0;

    if (pSplitReason == 0)
    {
      if (m_bOnlyPragmaMarked)
      {
        return TRAVERSE;
      }
      if (hasSideEffects(*this, Node->getCond()))
      {
        Note(Node, "condition may have side effects, unable to detect invariance");
        return TRAVERSE;
      }
      llvm::DenseSet<Decl*> localReferredDecls;
      visit_df(m_ForNode, CollectReferredDecls(localReferredDecls, Node->getCond()));
      if (!testConditionInvariance(localReferredDecls, Node->getCond()))
      {
        return TRAVERSE;
      }
      pSplitReason = "if-condition moved outside the loop due to no references to local loop vars";
    }

    Stmt* enclosingIf = GenerateSplit(*this, m_ForNode).doSplit(Node, pSplitReason, boost::logic::indeterminate);
    m_SplittedIfs.insert(enclosingIf);
    return SPLITTED;
  }

  unsigned VisitWhileStmt(WhileStmt*)
  {
    return SKIP_CHILDREN;
  }
  unsigned VisitForStmt(ForStmt*)
  {
    return SKIP_CHILDREN;
  }
  unsigned VisitDoStmt(DoStmt*)
  {
    return SKIP_CHILDREN;
  }
};


//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
bool splitInvariantIfs(StmtEditor& editor, Stmt* Node, bool bOnlyPragmaMarked)
{
  bool bResult = false;
  editor.setIdentifierPolicy("splitted");
  llvm::DenseSet<Stmt*> splittedIfs;
  for (stmt_iterator<ForStmt> i = stmt_ibegin(Node), 
       e = stmt_iend(Node); i != e;)
  {
    if (i->getBody() != 0)
    {
      FindAndSplitIfs visitor(editor, *i, splittedIfs, bOnlyPragmaMarked);
      if (visitor.generate())
      {
        i = stmt_ibegin(Node);
        bResult = true;
        continue;
      }
    }
    ++i;
  }
  editor.restoreIdentifierPolicy();
  return bResult;
}

//--------------------------------------------------------- 
void splitInvariantIf(StmtEditor& editor, ForStmt* Node, Stmt* invariantIf, 
                      boost::logic::tribool conditionResult)
{
  editor.setIdentifierPolicy("splitted");
  llvm::DenseSet<Stmt*> splittedIfs;
  const char* pSplitReason;
  if (boost::logic::indeterminate(conditionResult))
  {
    pSplitReason = "if-condition moved outside the loop due to external invariance detection";
  } 
  else
  {
    pSplitReason = conditionResult ? 
      "if-condition and else-part removed due to external detection of static true" :
      "if-condition and then-part removed due to external detection of static false";
  }
  GenerateSplit(editor, Node).doSplit(invariantIf, pSplitReason, conditionResult);
  editor.restoreIdentifierPolicy();
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

