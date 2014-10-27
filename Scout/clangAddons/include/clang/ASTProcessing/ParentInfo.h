//===--- Cleanup.h - "Umbrella" header for AST library ----------*- C++ -*-===//
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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_PARENTINFO_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_PARENTINFO_H

#include "clang/AST/ParentMap.h"

//--------------------------------------------------------- 
namespace clang {

class ParentInfo
{
public:
  ParentInfo(Stmt* root);
  void updateParentMap(Stmt* S);    // adds and/or updates the parent/child-relations of the stmt tree of S

  // postconditon: result != 0
  Stmt* getParent(Stmt* s) const;

  // postconditon: result != 0 
  Expr* getFullExpression(Expr* subexpr) const;

  // what is the statement containing an expression:
  // if in a ReturnStmt or IndirectGotoStmt it's that ReturnStmt or IndirectGotoStmt
  // if it's the expr in "case expr: body;" it's that  CaseStmt
  // if it's in a DeclStmt, which is part of for(init;;), while(cond), switch(cond), if(cond), it's the appropriate For/While/Do/Switch/IfStmt
  // if it's in a other DeclStmt, it's that DeclStmt
  // otherwise it's the full expression
  // postconditon: (fullexpr == result || parent(fullexpr) == result || (parent(parent(fullexpr)) == result && isa<DeclStmt>(parent(fullexpr)))
  Stmt* getStatementOfExpression(Expr* subexpr) const;

  // returns true, if Node is a real child of Parent:
  // (i.e. Node == Parent yields false)
  bool isChildOf(const Stmt* Node, const Stmt* Parent) const;

private:
  Stmt* getStatementOfStmtWhereDeclIsAllowed(Stmt* result, Stmt* parent) const;

  ParentMap m_ParentMap;           
};

//--------------------------------------------------------- 
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_PARENTINFO_H
