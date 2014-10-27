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

#ifndef SCOUT_CLANGADDONS_VECTORIZING_ANALYSIS_H
#define SCOUT_CLANGADDONS_VECTORIZING_ANALYSIS_H

#include <boost/logic/tribool.hpp>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class ForStmt;
class IfStmt;
class VarDecl;

//--------------------------------------------------------- 
namespace ASTProcessing {

struct VectorizeInfo;

  
struct AnalyzeResult
{
  enum tResult { DONT_VECTORIZE, VECTORIZE, SPLIT_IF, GUARDING_IF };

  tResult m_Result;
  union {
    Stmt*   m_InvariantIf;
    IfStmt* m_GuardingIf;
  };
  boost::logic::tribool m_StaticInvariantIfResult;

  static AnalyzeResult Vectorize(bool doIt);
  static AnalyzeResult SplitIf(Stmt* invariantIf, boost::logic::tribool staticResult);
  static AnalyzeResult GuardingIf(IfStmt* guardingIf);
};

AnalyzeResult analyzeBody(ForStmt* Node, VectorizeInfo& info);

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_VECTORIZING_ANALYSIS_H
