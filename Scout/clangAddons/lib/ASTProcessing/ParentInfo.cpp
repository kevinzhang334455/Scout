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

#include "clang/ASTProcessing/ParentInfo.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/StmtCXX.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- 
ParentInfo::ParentInfo(Stmt* root) :
  m_ParentMap(root)
{}

//--------------------------------------------------------- 
void ParentInfo::updateParentMap(Stmt* S)
{
  m_ParentMap.addStmt(S);
}

//--------------------------------------------------------- 
Stmt* ParentInfo::getParent(Stmt* s) const
{
  Stmt* parentStmt = m_ParentMap.getParent(s);
  assert(parentStmt && "not in parent map, did you forget to update a clone or similiar?");
  return parentStmt;
}

//--------------------------------------------------------- 
bool ParentInfo::isChildOf(const Stmt* Node, const Stmt* Parent) const
{
  while ((Node = m_ParentMap.getParent(Node)) != 0 && Node != Parent);
  return Node == Parent;
}

//--------------------------------------------------------- 
Expr* ParentInfo::getFullExpression(Expr* subexpr) const
{
  Expr* result = NULL;
  for (Stmt* s = subexpr; 
       s != 0 && dyn_cast<Expr>(s) != NULL; s = getParent(s))
  {
    result = static_cast<Expr*>(s);
  }
  return result; 
}

//--------------------------------------------------------- 
Stmt* ParentInfo::getStatementOfStmtWhereDeclIsAllowed(Stmt* result, Stmt* parent) const
{
  if (IfStmt* IS = dyn_cast<IfStmt>(parent))
  {
    return result == IS->getCond() ? parent : result;
  }
  else if (SwitchStmt* SS = dyn_cast<SwitchStmt>(parent))
  {
    return result == SS->getCond() ? parent : result;
  }
  else if (WhileStmt* WS = dyn_cast<WhileStmt>(parent))
  {
    return result == WS->getCond() ? parent : result;
  }
  else if (DoStmt* DoS = dyn_cast<DoStmt>(parent))
  {
    return result == DoS->getCond() ? parent : result;
  }
  else if (ForStmt* FS = dyn_cast<ForStmt>(parent))
  {
    return result == FS->getBody() ? result : parent;
  }
  return result;
}

//--------------------------------------------------------- 
Stmt* ParentInfo::getStatementOfExpression(Expr* subexpr) const
{
  Expr* result = getFullExpression(subexpr);
  Stmt* parent = getParent(result);

  if (isa<CompoundStmt>(parent) || 
      isa<LabelStmt>(parent) ||
      isa<DefaultStmt>(parent) ||
      isa<CXXCatchStmt>(parent))
  {
    return result;
  } 
  else if (isa<ReturnStmt>(parent) || 
           isa<IndirectGotoStmt>(parent))
  {
    return parent;
  } 
  else if (CaseStmt* CS = dyn_cast<CaseStmt>(parent))
  {
    return result == CS->getSubStmt() ? result : parent;
  }
  else if (isa<DeclStmt>(parent))
  {
    return getStatementOfStmtWhereDeclIsAllowed(parent, getParent(parent));
  }
  else 
  {
    return getStatementOfStmtWhereDeclIsAllowed(result, parent);
  }
}

//--------------------------------------------------------- 
} // ns clang
