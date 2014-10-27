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

#include "clang/ASTProcessing/Inline.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/ModificationNoteBuilder.h"
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/ReplaceExprWithStmt.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/AST/ASTContext.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/DeclCXX.h"    
#include "llvm/ADT/DenseMap.h"

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
struct ReplaceParameters : StmtVisitor<ReplaceParameters> 
{
  typedef llvm::DenseMap<const ParmVarDecl*, ValueDecl*> tReplaceFnArgsMap;
  const tReplaceFnArgsMap& m_ReplaceFnArgsMap;

  ReplaceParameters(tReplaceFnArgsMap& replaceFnArgsMap) : m_ReplaceFnArgsMap(replaceFnArgsMap) {}

  void VisitDeclRefExpr(DeclRefExpr* Node)
  {
    if (ParmVarDecl *PVD = dyn_cast<ParmVarDecl>(Node->getDecl())) 
    {
      tReplaceFnArgsMap::const_iterator i = m_ReplaceFnArgsMap.find(PVD);
      if (i != m_ReplaceFnArgsMap.end())
      {
        Node->setDecl(i->second);
      }
      else
      {
        std::string errorDecl = PVD->getNameAsString();
        assert(0 && "no replacement for function argument?");
      }
    }
  }
};


//--------------------------------------------------------- 
struct ReplaceReturns : public StmtVisitor<ReplaceReturns, bool>, StmtEditor
{
  LabelStmt*  m_GotoTarget;
  Expr*       m_ResultTarget;

  ReplaceReturns(StmtEditor& editor, LabelStmt* gotoTarget, Expr* resultTarget) : StmtEditor(editor), m_GotoTarget(gotoTarget), m_ResultTarget(resultTarget) {}

  static Stmt* generate(StmtEditor* ctx, Stmt* stmtBody, LabelStmt* gotoTarget, Expr* resultTarget)
  {
    visit_df(stmtBody, ReplaceReturns(*ctx, gotoTarget, resultTarget));
    return stmtBody;
  }

  bool VisitReturnStmt(ReturnStmt* Node)
  {
    // return expr; -> { m_ResultTarget = expr; goto m_GotoTarget; }
    Stmt* newStmts[2];
    unsigned newStmtCnt = 0;
    if (Node->getRetValue()) 
    {
      if (m_ResultTarget)
      {
        newStmts[newStmtCnt++] = Assign_(Clone_(m_ResultTarget), Node->getRetValue());
      }
      else if (hasSideEffects(*this, Node->getRetValue()))
      {
        newStmts[newStmtCnt++] = Node->getRetValue();   // ensure the expression is executed
      }
    }
    newStmts[newStmtCnt++] = Goto_(m_GotoTarget);
    replaceStatement(Node, LazyCompound_(newStmts, newStmtCnt));
    return true;
  }
};

//--------------------------------------------------------- 
void streamlineImplicitCasts(StmtEditor& editor, Stmt* Node)
{
  for (stmt_iterator<ImplicitCastExpr> i = stmt_ibegin(Node), 
       e = stmt_iend(Node); i != e;)
  {
    ImplicitCastExpr* subCast, *E = *i;
    Expr* subE = E->getSubExpr();
    if (E->getCastKind() == CK_LValueToRValue)
    {
      bool hasParens = false;
      while (subE != 0)
      {
        if (ParenExpr* P = dyn_cast<ParenExpr>(subE))
        {
          subE = P->getSubExpr();
          hasParens = true;
          continue;
        }
        break;
      }

      if ((subCast = dyn_cast<ImplicitCastExpr>(subE)) != 0 &&
          subCast->getCastKind() == CK_LValueToRValue)
      {
        Expr* realExpr = subCast->getSubExpr();
        E->setSubExpr(hasParens ? editor.Paren_(realExpr) : realExpr);
        i = stmt_ibegin(Node);
        continue;
      }
      if (!(subE->isGLValue() || subE->getType()->isFunctionType() ||
            subE->getType()->isVoidType() || isa<CXXTemporaryObjectExpr>(subE)))
      { 
        E->setCastKind(CK_NoOp);
      }
    }
    ++i;
  }
}

//--------------------------------------------------------- 
struct InlineVisitor : public StmtVisitor<InlineVisitor, bool>, StmtEditor
{
  explicit InlineVisitor(StmtEditor& editor) : 
    StmtEditor(editor) {}

  DeclStmt* replaceImplicitThis(const FunctionDecl *FD, CallExpr* expr, CompoundStmt* S)
  {
    if (!isa<CXXMethodDecl>(FD))
    {
      return 0;
    }

    DeclStmt* tmpDecl = 0;
    if (CXXOperatorCallExpr* COP = dyn_cast<CXXOperatorCallExpr>(expr))
    {




    }

    CXXMemberCallExpr* CMC = dyn_cast<CXXMemberCallExpr>(expr);
    const MemberExpr *MemExpr;
    if (CMC != 0 &&   
        (MemExpr = dyn_cast<MemberExpr>(CMC->getCallee()->IgnoreParens())) != 0 &&
        !isa<CXXThisExpr>(MemExpr->getBase()))
    {
      Expr* own = MemExpr->getBase();
      tmpDecl = MemExpr->isArrow() ? 
                  TmpVar_(own->getType(), own) :
                  TmpVar_(Ctx().getPointerType(own->getType().getNonReferenceType()), UnaryOp_(own, UO_AddrOf));
      for (stmt_iterator<CXXThisExpr> i = stmt_ibegin(S), 
           e = stmt_iend(S); i != e; )
      {
        CXXThisExpr* CTE = *i;
        ++i;
        replaceStatement(CTE, DeclRef_(tmpDecl));
      }
    }
    return tmpDecl;
  }

  void replaceDeclContext(StmtClone::Mapping& cloneMap, const DeclContext* sourceFD)
  {
    for (StmtClone::DeclMapping::iterator i = cloneMap.m_DeclMapping.begin(),
           e = cloneMap.m_DeclMapping.end(); i != e; ++i)
    {
      if (i->first->getDeclContext() == sourceFD)
      {
        i->second->setDeclContext(&getFnDecl());
      }
    }
  } 


  void replaceParameterWithTemps(const FunctionDecl *FD, CallExpr* expr, CompoundStmt* S)
  {
    ReplaceParameters::tReplaceFnArgsMap  paramToTempMap;
    std::vector<Stmt*> allStmts(FD->getNumParams() + S->size());
    unsigned stmtInsertPos = FD->getNumParams();
    if (DeclStmt* thisTemp = replaceImplicitThis(FD, expr, S))
    {
      allStmts.push_back(0);
      allStmts[stmtInsertPos++] = thisTemp;
    }

    std::copy(S->body_begin(), S->body_end(), allStmts.begin() + stmtInsertPos);
    for (unsigned i = 0, e = FD->getNumParams(); i < e; ++i)
    {
      const ParmVarDecl *PVD = FD->getParamDecl(i);
      DeclStmt* tmpDecl = TmpVar_(PVD->getType().getNonReferenceType(), expr->getArg(i));
      paramToTempMap[PVD] = cast<ValueDecl>(tmpDecl->getSingleDecl());
      allStmts[i] = tmpDecl;
    }
    // since ReplaceParameters rechain some decl pointers only, we can execute it safely before adding
    // the tmp decl stmts to S, thus we can assert in ReplaceParameters 
    // (otherwise the init exprs of the tmp decl stmts could itself contain ParmVarDecl)
    visit_df(S, ReplaceParameters(paramToTempMap));
    replaceStmts(S, &allStmts[0], allStmts.size());
  }

  CompoundStmt* getInlineAbleBody(const FunctionDecl *&FD)
  {
    return 
      (!findAttachedPragma(FD, "function", "dummy")) && inlineable(FD) ? 
        dyn_cast_or_null<CompoundStmt>(FD->getBody(FD)) : 
        NULL;
  }

  void cleanup(Stmt* stmt)
  {
    flattenBlocks(*this, stmt);
    removeGotosToNextLabel(*this, stmt);
    moveDeclsToFirstWrite(*this, stmt);
    expandOneUsedInitDeclsToUse(*this, stmt);
    streamlineImplicitCasts(*this, stmt);
  }

  bool VisitCallExpr(CallExpr* expr)
  {
    const FunctionDecl *FD = findFunctionDecl(expr);
    CompoundStmt* Body = FD ? getInlineAbleBody(FD) : NULL;
    if (Body)
    {
      StmtClone::Mapping cloneMap;
      CompoundStmt* Body_copy = cloneStmtTree(*this, Body, &cloneMap);

      replaceParameterWithTemps(FD, expr, Body_copy);
      replaceDeclContext(cloneMap, FD); 
      LabelStmt* exitLabel = Label_(NullStmt_());
      appendStmt(Body_copy, exitLabel);
      Stmt* generatedStmt = replaceExprWithStmt(*this, expr, std::bind(&ReplaceReturns::generate, this, Body_copy, exitLabel, _1));
      if (FD->getIdentifier() != 0)
      {
        attachComment(generatedStmt, FD->getName().str().c_str());
      }
      cleanup(generatedStmt);
      std::string tmp;
      llvm::raw_string_ostream s(tmp);
      FD->getNameForDiagnostic(s, getPrintingPolicy(), false);
      ModificationNote(expr, "function call to %0 inlined {tgt:%1:%2}") << s.str();
      return true;
    }
    return false;
  }
};

//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
bool doInline(StmtEditor& editor, Stmt* Node)
{
  bool bResult = false;
  InlineVisitor inliner(editor);
  for (llvm::df_iterator<Stmt*> i = llvm::df_begin(Node), e = llvm::df_end(Node); i != e;)
  {
    if (inliner.Visit(*i))
    {
      i = llvm::df_begin(Node);
      bResult = true;
    }
    else
    {
      ++i;
    }
  }
  return bResult;
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

