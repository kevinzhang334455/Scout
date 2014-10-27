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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_INLINE_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_INLINE_H

#include <functional>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class Expr;
class StmtEditor;

//--------------------------------------------------------- 
namespace ASTProcessing {

// inlines all inlineable function calls in Node
// precondition: Node is a child of the stmt tree hold by StmtEditor
bool doInline(StmtEditor& editor, Stmt* Node);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_INLINE_H
