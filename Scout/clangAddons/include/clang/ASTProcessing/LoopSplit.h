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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_LOOPSPLIT_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_LOOPSPLIT_H

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class ForStmt;
class StmtEditor;

//--------------------------------------------------------- 
namespace ASTProcessing {

// split loop bodies at pragmas 
// for(stmt, expr, expr)
// {
//   stmt1;
// #pragma scout loop split
//   stmt2;
// }
//
// becomes:

// for(stmt, expr, expr)
//   stmt1;
// for(stmt, expr, expr)
//   stmt2;

// precondition: Node is a child of the stmt tree hold by StmtEditor
void splitLoops(StmtEditor& editor, Stmt* Node);

// loop is splitted before splitBefore:
void splitLoops(StmtEditor& editor, ForStmt* Node, Stmt* splitBefore);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_LOOPSPLIT_H
