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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_LOOPBLOCKING_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_LOOPBLOCKING_H

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class ForStmt;
class StmtEditor;

//--------------------------------------------------------- 
namespace ASTProcessing {

// blocks a fortran-like loop with a constant for-conditon:
//
// for(stmt; i <|<=|!= expr; ++i)    // expr is constant during loop execution
// {
//   body;
// }
//
// becomes:
//
// {
//   i_bound = expr[+1];  // increment, if <= was used
//   for(stmt; i < i_bound; i += uTileSize)
//   {
//     temp_bound = i_bound - i;
//     temp_bound = temp_bound < tileSize ? temp_bound : tileSize;
//     for(temp = 0; temp < temp_bound; ++temp)
//     {
//       body_{i replaced by (i+temp)};
//     }
//   }
// }
//
// note: if we block a loop, that is later vectorized and uTileSize
// is divideable by vector_size, then all pragmatic alignment properties 
// (noremainder, alignment) apply to the inner loop as well
//
// precondition: Node is a child of the stmt tree hold by StmtEditor
// returns: false, if Node is not a blockable fortran loop 
bool blockLoop(StmtEditor& editor, ForStmt* Node, int tileSize, bool emitError);

bool blockLoops(StmtEditor& editor, Stmt* Node);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_LOOPBLOCKING_H
