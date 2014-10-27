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

#include "clang/ASTProcessing/LoopUnroll.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "llvm/ADT/SmallVector.h"

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
class LoopUnroller : StmtEditor
{
public:
  LoopUnroller(StmtEditor& editor) : StmtEditor(editor) {}

  bool unroll(ForStmt* Node);

private:
  void doUnrolling(ForStmt* Node);
  int evalCondition(Expr* Node);
  bool resolveCondition(Expr* Node);
  bool foldParent(Expr* E);

  VarDecl* m_LoopVar;
  bool     m_bSequenceClosed; 
  llvm::APSInt m_StartIdx, m_EndIdx;
};

//--------------------------------------------------------- 
int LoopUnroller::evalCondition(Expr* Node)
{
  bool result;
  if (EvaluateAsBooleanCondition_(Node, result))
  {
    return result ? 1 : 0;
  }
  return -1;
}

//--------------------------------------------------------- 
bool LoopUnroller::resolveCondition(Expr* Node)
{
  ConditionalOperator* CO;
  IfStmt* IS;
  int evalResult;
  Stmt* parent;

  do 
  {
    parent = getParent(Node);
    if ((CO = dyn_cast<ConditionalOperator>(parent)) != 0 &&
        CO->getCond() == Node &&
        (evalResult = evalCondition(CO->getCond())) >= 0)
    {
      replaceStatement(parent, evalResult == 1 ? CO->getTrueExpr() : CO->getFalseExpr());
      return true;
    }
    if ((IS = dyn_cast<IfStmt>(parent)) != 0 &&
        IS->getCond() == Node &&
        (evalResult = evalCondition(IS->getCond())) >= 0)
    {
      Stmt* stmtToExecute = evalResult == 1 ? IS->getThen() : IS->getElse();
      if (stmtToExecute != 0)
      {
        replaceStatement(parent, stmtToExecute);
      }
      else
      {
        removeStmt(parent);
      }
      return true;
    }
  }
  while ((Node = dyn_cast<Expr>(parent)) != 0);
  return false;
}

//--------------------------------------------------------- 
bool LoopUnroller::foldParent(Expr* Node)
{
  APValue result, lastValidResult;
  Expr* lastFoldedNode = 0;
  while ((Node = dyn_cast<Expr>(getParent(Node))) != 0 && 
         EvaluateAsRValue_(Node, result) && 
         (result.isInt() || result.isFloat()))
  {
    lastFoldedNode = Node;
    lastValidResult = result;
  }
  if (lastFoldedNode != 0)
  {
    if (lastValidResult.isInt())
    {
      replaceStatement(lastFoldedNode, 
        Int_(lastValidResult.getInt(), lastFoldedNode->getType()));
    }
    else 
    {
      replaceStatement(lastFoldedNode, 
        Float_(lastValidResult.getFloat(), lastFoldedNode->getType()));
    }
    return true;
  }
  return false;
}

//--------------------------------------------------------- 
void LoopUnroller::doUnrolling(ForStmt* Node)
{
  llvm::SmallVector<Stmt*, 8> unrolledBody;
  CompoundStmt* CS = ensureCompoundParent(Node);
  if (Node->getBody() != 0)
  {
    while (m_bSequenceClosed ? m_StartIdx <= m_EndIdx : m_StartIdx < m_EndIdx)
    {
      Stmt* clonedBody = cloneStmtTree(*this, Node->getBody());

      // ensure a compound body, otherwise folding
      // top-level conditions (e.g.  for(...) if (...) {}) wouldn't work: 
      CompoundStmt* csBody = dyn_cast<CompoundStmt>(clonedBody);
      if (csBody == 0)
      {
        csBody = Compound_(&clonedBody, 1);
        updateParentMap(csBody);
      }

      unrolledBody.push_back(csBody);
      
      for (stmt_iterator<DeclRefExpr> i = stmt_ibegin(clonedBody), 
           e = stmt_iend(clonedBody); i != e;)
      {
        if ((*i)->getDecl() == m_LoopVar)
        {
          Expr* replaceNode = *i;
          ImplicitCastExpr* LRCast = dyn_cast<ImplicitCastExpr>(getParent(*i));
          if (LRCast != 0 && LRCast->getCastKind() == CK_LValueToRValue)
          {
            replaceNode = LRCast;
          }

          Expr* newStmt = Int_(m_StartIdx, m_LoopVar->getType()); 
          replaceStatement(replaceNode, newStmt);
          resolveCondition(newStmt) || foldParent(newStmt);
          i = stmt_ibegin(clonedBody);
          continue;
        }
        ++i;
      }
      ++m_StartIdx;
    }
  }
  if (unrolledBody.empty())
  {
    removeStmt(CS, Node);
  }
  else
  {
    replaceStatement(Node, Compound_(&unrolledBody[0], unrolledBody.size()));
  }
}

//--------------------------------------------------------- 
bool LoopUnroller::unroll(ForStmt* Node)
{
  const unsigned int rangeThreshhold = 10;

  const char* pErrMsg;
  m_LoopVar = isFortranLoop(Node, pErrMsg);
  if (m_LoopVar != 0)
  {
    // we know: for (???; var rel expr; ++var)
    BinaryOperator* condCmp = cast<BinaryOperator>(Node->getCond());
    if ((condCmp->getOpcode() == BO_NE ||
         condCmp->getOpcode() == BO_LT ||
         condCmp->getOpcode() == BO_LE) &&
        condCmp->getRHS()->isIntegerConstantExpr(m_EndIdx, Ctx()))
    { 
      m_bSequenceClosed = condCmp->getOpcode() == BO_LE;

      // we know: for (???; var <|<=|!= endIdx; ++var)
      Expr* initExpr = 0;
      VarDecl* initVar = 0;

      DeclRefExpr* initVarRef;
      BinaryOperator* BO;
      DeclStmt* DS;
      if ((BO = dyn_cast_or_null<BinaryOperator>(Node->getInit())) != 0 && 
          BO->getOpcode() == BO_Assign && 
          (initVarRef = dyn_cast<DeclRefExpr>(BO->getLHS())) != 0)
      {
        // we know: for (varX = ???; var <|<=|!= endIdx; ++var)
        initVar = dyn_cast<VarDecl>(initVarRef->getDecl());
        initExpr = BO->getRHS();
      }
      else if ((DS = dyn_cast_or_null<DeclStmt>(Node->getInit())) != 0 && 
               DS->isSingleDecl() && 
               (initVar = dyn_cast<VarDecl>(DS->getSingleDecl())) != 0)
      {
        // we know: for (int varX = ???; var <|<=|!= endIdx; ++var)
        initExpr = initVar->getInit();
      }
      if (initVar == m_LoopVar &&
          initExpr && 
          initExpr->isIntegerConstantExpr(m_StartIdx, Ctx()) && 
          m_EndIdx - m_StartIdx <= llvm::APSInt(llvm::APInt(32, rangeThreshhold), false))
      {
        // we know: for ([int] var = startIdx; var <|<=|!= endIdx; ++var)
        doUnrolling(Node);
        return true;
      }
    }
  }
  return false;
}

//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
bool unrollLoops(StmtEditor& editor, Stmt* Node)
{
  bool bResult = false;
  editor.setIdentifierPolicy("unrolled");
  LoopUnroller unroller(editor);
  for (stmt_iterator<ForStmt> i = stmt_ibegin(Node), e = stmt_iend(Node); i != e;)
  {
    if (editor.findAttachedPragma(*i, "loop", "unroll") != 0 &&
        unroller.unroll(*i))
    {
      bResult = true;
      if (*i == Node)
      {
        break;
      }
      i = stmt_ibegin(Node);
      continue;
    }
    ++i;
  }
  editor.restoreIdentifierPolicy();
  return bResult;
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

