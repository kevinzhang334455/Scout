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

#ifndef SCOUT_CLANGADDONS_VECTORIZING_EXPRESSIONMATCH_H
#define SCOUT_CLANGADDONS_VECTORIZING_EXPRESSIONMATCH_H

#include "llvm/ADT/SmallVector.h"
#include <string>
#include <list>

//--------------------------------------------------------- 
namespace clang {

class CXXMethodDecl;
class Expr;

namespace ASTProcessing {

class StmtEditor;
typedef llvm::SmallVector<std::list<Expr*>, 4> tAllSubExpressions;

//--------------------------------------------------------- 
std::string buildExpressionString(CXXMethodDecl* MD, Expr* E);
std::string buildInternalExpressionString(const char* pSimpleExpr);

//--------------------------------------------------------- 
bool matchExpressionString(const std::string& exprString, const Expr* E,
  llvm::SmallVector<Expr*, 4>& subExprs, StmtEditor& editor);

//--------------------------------------------------------- 
bool matchExpressionString(const std::string& exprString, const Expr* E,
                           tAllSubExpressions& subExprs, StmtEditor& editor);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_VECTORIZING_EXPRESSIONMATCH_H
