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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_INVARIANTIF_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_INVARIANTIF_H

#include <boost/logic/tribool.hpp>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class ForStmt;
class IfStmt;
class StmtEditor;

//--------------------------------------------------------- 
namespace ASTProcessing {

// searches all for stmts in Node for invariant if's
// (that is the condition does't change due to the loop)
// invariant if's are then moved outside the loop and the 
// loop is duplicated with the 'then' and 'else' part
// in the two loops:
// for(stmt, expr, expr)
//   if (invariant)
//     stmt1;
//   else
//     stmt2;

// becomes:

// if (invariant)
//   for(stmt, expr, expr)
//     stmt1;
// else
//   for(stmt, expr, expr)
//     stmt2;

// precondition: Node is a child of the stmt tree hold by StmtEditor
bool splitInvariantIfs(StmtEditor& editor, Stmt* Node, bool bOnlyPragmaMarked);

// precondition: invariantIf is da IfStmt or a ConditionalOperator
// TODO: extend to SwitchStmt
void splitInvariantIf(StmtEditor& editor, ForStmt* Node, Stmt* invariantIf, 
                      boost::logic::tribool conditionResult);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_INVARIANTIF_H
