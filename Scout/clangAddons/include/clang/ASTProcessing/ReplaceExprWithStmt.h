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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_REPLACEEXPRWITHSTMT_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_REPLACEEXPRWITHSTMT_H

#include <functional>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class Expr;

//--------------------------------------------------------- 
namespace ASTProcessing {
class StmtEditor;

// Replace the expression 'from' with a statement produced by 
// fnGenerator.  fnGenerator, when given an expr as input, must produce
// some code which leaves its result in the given expr. The output
// from fnGenerator is then inserted into the original program in such a
// way that whenever the expression had previously been evaluated, the
// statements produced by the generator are run instead and their result is
// used in place of from. If expr given to fnGenerator is 0, the generated
// code ignores its result too.
// precondition: from is a child of the stmt tree hold by StmtEditor
// returns: the generated statement
Stmt* replaceExprWithStmt(StmtEditor& editor, Expr* from, const std::function<Stmt*(Expr*)>& fnGenerator);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_REPLACEEXPRWITHSTMT_H
