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

#ifndef SCOUT_CLANGADDONS_VECTORIZING_TARGETSTMT_H
#define SCOUT_CLANGADDONS_VECTORIZING_TARGETSTMT_H

#include "clang/Vectorizing/VectorizeInfo.h"    
#include "clang/ASTProcessing/StmtClone.h"    
#include <list>

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

//--------------------------------------------------------- 
class TargetScalarStmt;

//--------------------------------------------------------- 
class TargetStmt : IntrinsicEditor
{
public:
  TargetStmt(VectorizeInfo& info, const SimdType& targetType);

  // splice the result!
  std::list<Stmt*>& generate(BinaryOperator *sourceNode, unsigned index); 

  Expr* generateLoad(Expr* RHS, unsigned index);
  DeclRefExpr* getVar(Expr* RHS, unsigned index);

  // canonical type for unrolled expressions (size is always targetSize)
  typedef llvm::SmallVector<Expr*, 8> tUnrolledExprs;

  const SimdType& getTargetType() const { return m_TargetType; }
  unsigned getIndex() const { return m_Index; }

  void mergeStmts(TargetStmt& other) 
  { m_GeneratedStmts.splice(m_GeneratedStmts.end(), other.m_GeneratedStmts); }
  void addStmt(Stmt* S)  
  { m_GeneratedStmts.push_back(S); }

private:

  // load/store emitter
  Expr* emitStoreIntrinsic(Expr* LHS, DeclRefExpr* targetRHS);
  Expr* emitLoadIntrinsic(Expr* RHS);
  Expr* retrieveLoadStoreArg(Expr* Node);

  // expression splitting
  DeclRefExpr* splitEnsureVar(Expr* Node);
  DeclRefExpr* ensureVar(Expr* targetStmt);
  void getSplitSubExpr(Expr* Node, tUnrolledExprs& targetExprs);
  Expr* split_subexpressions(Expr* Node);
  Expr* split_without_tempvar(Expr* Node);

  FunctionDecl* getIntrinsic(Expr* Node, llvm::SmallVector<Expr*, 4>& intrinsicArgs);
  void emitLHS(Expr* sourceLHS, DeclRefExpr* targetRhs);
  Expr* unrollComplexRHS(Expr* RHS);
  void unrollComplexLHS(Expr* LHS, DeclRefExpr* targetRHS);

  const char* testFnName(const char* fnName, const char* fnFallback) const;

  VectorizeInfo& m_Info;
  SimdType m_TargetType;
  unsigned m_Index;
  const char* m_StoreFunctions[2][2]; // 1st index: not aligned/aligned
                                      // 2nd index: normal/non-temporal
  std::list<Stmt*>  m_GeneratedStmts;
  bool    m_bGatherAvail, m_bScatterAvail;

  friend class TargetScalarStmt;

};

//--------------------------------------------------------- 
class TargetScalarStmt : IntrinsicEditor
{
public:
  typedef TargetStmt::tUnrolledExprs tUnrolledExprs;

  TargetScalarStmt(VectorizeInfo& info, unsigned targetSize);
  TargetScalarStmt(TargetStmt& sourceStmt);

  void rollout(Stmt* Node, Stmt** rolloutTarget);
  void rollout(Stmt* Node, tUnrolledExprs& exprs);

private:
  Expr* insertStrideIndex(unsigned idx, Expr* strideExpr);
  Stmt* scalarizeTarget(Stmt* Node, StmtCloneMapping& mapping, unsigned currentIndex);

  VectorizeInfo& m_Info;
  unsigned m_Size;
  unsigned m_Index;
};

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  // SCOUT_CLANGADDONS_VECTORIZING_TARGETSTMT_H
