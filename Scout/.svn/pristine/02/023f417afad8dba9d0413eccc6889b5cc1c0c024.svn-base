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

#include "clang/ASTProcessing/DeclCollector.h" 
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/DeclCXX.h"    

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
struct SplitDeclStmts : StmtEditor
{
  DeclCollector m_Collector;
  
  SplitDeclStmts(StmtEditor& editor) : 
    StmtEditor(editor), 
    m_Collector(editor)
  {} 

  Expr* getInitExpr(VarDecl* VD)
  {
    Expr* Init = VD->getInit();
    if (Init != 0)
    {
      if (VD->isDirectInit())
      {    
        return CXXFunctionalCast_(Init);    // Type var(X) -> var = Type(X);
      }
      CXXConstructExpr *CCE = dyn_cast<CXXConstructExpr>(Init);
      if (CCE && !CCE->getConstructor()->isCopyConstructor())
      {
        return CXXTemporaryObject_(CCE);    // Type var; -> var = Type();
      }
    }
    return Init;
  }

  bool VisitDeclStmt(DeclStmt* Node)
  {
    DeclCollector::tVarDecls varDecls;
    m_Collector.collect(Node, &varDecls);
    if (varDecls.empty())
    {
      return false;
    }

    llvm::SmallVector<Stmt*, 8> initStmts;
    for (DeclCollector::tVarDecls::iterator i = varDecls.begin(), 
         e = varDecls.end(); i != e; ++i)
    {
      VarDecl* VD = *i;
      if (Expr* Init = getInitExpr(VD))
      {
        initStmts.push_back(Assign_(DeclRef_(VD), Init));
      }
      VD->setInit(0);
    }
    if (initStmts.empty())
    {
      removeStmt(Node);
    }
    // if there are any issues with the code below, then look at rev190,
    // where the comma oparator was used instead of an compound
    // however this had lead to other issues, mainly the comma operator 
    // was not analysed properly afterwards
    else if (initStmts.size() == 1)
    {
      replaceStatement(Node, initStmts[0]);
    }
    else
    {
      replaceStatement(Node, Compound_(&initStmts[0], initStmts.size()));
    }
    return true;
  }
};

//--------------------------------------------------------- 
} // anon namespace 



//--------------------------------------------------------- 
void DeclCollector::collect(DeclStmt* Node, tVarDecls* pCollectedDecls)
{
  // this doesn't necessary hold, if e.g 
  // inlined functions have declaration groups:
  //assert(isOriginalStmt(Node) || Node->isSingleDecl());
  VarDecl* VD;
  for (DeclStmt::decl_iterator i = Node->decl_begin(), e = Node->decl_end();
       i != e; ++i)
  {
    if ((VD = dyn_cast<VarDecl>(*i)) != 0 && !VD->getType()->isReferenceType())
    {   
      collect(VD, pCollectedDecls);
    }
  }
}

//--------------------------------------------------------- 
void DeclCollector::collect(VarDecl* VD, tVarDecls* pCollectedDecls)
{
  // FIXME: is it important not to remove volatile here?
  assert(!VD->getType()->isReferenceType());
  m_CollectedDecls[VD->getType().getUnqualifiedType().getAsOpaquePtr()].push_back(VD);
  if (pCollectedDecls)
  {
    pCollectedDecls->push_back(VD);
  }
}

//--------------------------------------------------------- 
void DeclCollector::emitCollectedDecls()
{
  CompoundStmt* rootCS = cast<CompoundStmt>(getRoot());
  insertCollectedDeclsAtCompoundBegin(rootCS);
  m_CollectedDecls.clear();
}

//--------------------------------------------------------- 
void DeclCollector::insertCollectedDeclsAtCompoundBegin(CompoundStmt* CS)
{ 
  llvm::SmallVector<Stmt*, 8> allStmts;
  StmtCloneMapping refRechainer;
  llvm::DenseSet<IdentifierInfo*> allEmittedIdentifiers;
  for (llvm::DenseMap<void*, tCollectedDecls>::iterator i = 
        m_CollectedDecls.begin(), e = m_CollectedDecls.end(); i != e; ++i)
  {
    assert(i->second.size() > 0);
    typedef llvm::DenseMap<IdentifierInfo*, VarDecl*> tIdentifierMap;
    tIdentifierMap emittedIdentifiers;
    llvm::SmallVector<Decl*, 8> unifiedDecls;
    for (tCollectedDecls::iterator i2 = i->second.begin(), e2 = i->second.end();
         i2 != e2; ++i2)
    {
      IdentifierInfo* varName = (*i2)->getIdentifier();
      if (allEmittedIdentifiers.insert(varName).second)
      {
        emittedIdentifiers[varName] = *i2;
        // FIXME: is it important not to remove volatile here?
        (*i2)->setType((*i2)->getType().getUnqualifiedType());
        unifiedDecls.push_back(*i2);
      }
      else if (!emittedIdentifiers.count(varName))
      {
        // we have a name clash among different types
        (*i2)->setDeclName(createVariable(*i2));
        emittedIdentifiers[(*i2)->getIdentifier()] = *i2;
        (*i2)->setType((*i2)->getType().getUnqualifiedType());
        unifiedDecls.push_back(*i2);
      } 
      else
      {
        refRechainer.m_DeclMapping[*i2] = emittedIdentifiers[varName];
      }
    }
    allStmts.push_back(DeclStmt_(&unifiedDecls[0], unifiedDecls.size()));
  }
  allStmts.append(CS->child_begin(), CS->child_end());
  replaceStmts(CS, allStmts.empty() ? 0 : &allStmts[0], allStmts.size());
  rechainRefs(refRechainer, CS);
}

//--------------------------------------------------------- 
void DeclCollector::makeCAst(StmtEditor& editor)
{
  SplitDeclStmts declStmtCollector(editor);
  bool bInTheActualBody = false;
  CompoundStmt* rootCS = cast<CompoundStmt>(editor.getRoot());
  for (int index = 0; index < (int)rootCS->size(); ++index)
  {
    Stmt* Node = *(rootCS->body_begin() + index);
    bInTheActualBody = bInTheActualBody || !isa<DeclStmt>(Node);
    for (stmt_iterator<DeclStmt> i = stmt_ibegin(Node), 
         e = stmt_iend(Node); i != e;)
    {
      bool bTopChanged = *i == Node;
      if (declStmtCollector.VisitDeclStmt(*i))
      {
        if (bTopChanged)
        {
          --index;
          break;
        }
        else
        {
          i = stmt_ibegin(Node);
        }
      }
      else
      {
        ++i;
      }
    }
  }
  declStmtCollector.m_Collector.emitCollectedDecls();
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

