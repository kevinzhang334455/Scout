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

#ifndef SCOUT_ASTPROCESSING_INTRINSICCOLLECTOR_H
#define SCOUT_ASTPROCESSING_INTRINSICCOLLECTOR_H

#include "clang/ASTProcessing/StmtCollector.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class ASTImporter;
class CompilerInstance;

namespace ASTProcessing {

//--------------------------------------------------------- 
class Configuration;
class SimdType;

//--------------------------------------------------------- 
class IntrinsicCollector : public StmtCollector
{
public:
  IntrinsicCollector(CompilerInstance& compiler, const ArtificialIdentifierPolicy& identifierPolicy,  
                     FunctionDecl& fnContext, Configuration& config);
  ~IntrinsicCollector(); // outlined

  FunctionDecl* getIntrinsicFn(const char* pFnName, SimdType baseType);
  FunctionDecl* getIndexedIntrinsicFn(const char* pFnName, SimdType baseType, unsigned index);
  FunctionDecl* getIntrinsicFn(FunctionDecl* FD, SimdType baseType);
  FunctionDecl* getIntrinsicExpr(Expr* E, SimdType baseType, llvm::SmallVector<Expr*, 4>& subExprs);
  FunctionDecl* getConvertFn(SimdType sourceType, SimdType targetType, unsigned index);
  FunctionDecl* getGlobalFn(const char* pFnName);

  QualType getSimdType(SimdType baseType) const;
  QualType getGSIndexType(SimdType baseType) const;
  unsigned getVectorSize(SimdType baseType) const;
  unsigned getVectorByteAlignment(SimdType baseType) const;
  bool isIntrinsicFn(FunctionDecl* fnDecl) const;
  bool hasIntrinsicBuiltin(const char* pName, SimdType baseType) const;
  SimdType getSimdType(BuiltinType::Kind baseType, unsigned exclMaxSize);
  SimdType retrieveGSIndexType(const SimdType& baseType);

private:
  Configuration& m_Configuration;

  llvm::OwningPtr<ASTImporter> m_Importer;

  typedef llvm::DenseMap<FunctionDecl*, llvm::StringRef> tEmittedIntrinsics;
  tEmittedIntrinsics m_EmittedIntrinsics;

  class IntrinsicPrinter;
  friend class IntrinsicPrinter;

  ASTContext& getCtx() const;
  virtual bool hasNoSideEffects(const FunctionDecl* FD) const;
  virtual bool inlineable(const FunctionDecl* FD) const;

  enum kSideEffectCacheValue { UNKNOWN = 0, HAS_INTRINSIC, HAS_NO_INTRINSIC };
  typedef llvm::DenseMap<const FunctionDecl*, kSideEffectCacheValue> tSideEffectCache;
  mutable tSideEffectCache m_SideEffectCache;
};

//--------------------------------------------------------- inlines
inline ASTContext& IntrinsicCollector::getCtx() const
{
  return Ctx;
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_ASTPROCESSING_INTRINSICCOLLECTOR_H
