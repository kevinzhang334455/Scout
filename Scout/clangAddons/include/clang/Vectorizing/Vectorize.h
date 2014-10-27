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

#ifndef SCOUT_CLANGADDONS_VECTORIZE_H
#define SCOUT_CLANGADDONS_VECTORIZE_H

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;

//--------------------------------------------------------- 
namespace ASTProcessing {

class IntrinsicEditor;

// precondition: Node is a child of the stmt tree hold by StmtEditor
bool vectorizeLoops(IntrinsicEditor& editor, Stmt* Node);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_VECTORIZE_H
