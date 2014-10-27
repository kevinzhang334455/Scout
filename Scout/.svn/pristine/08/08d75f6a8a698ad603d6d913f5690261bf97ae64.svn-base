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

#ifndef SCOUT_CLANGADDONS_INTRINSICEDITOR_H
#define SCOUT_CLANGADDONS_INTRINSICEDITOR_H

#include "clang/ASTProcessing/StmtEditor.h"
#include "clang/AST/Type.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards

namespace ASTProcessing {

class IntrinsicCollector;

//--------------------------------------------------------- 
class SimdType 
{ 
  BuiltinType::Kind m_BuiltinType; 
  unsigned          m_VectorSize; 
  const void*       m_OpaqueConfigEntry;

public:
  SimdType() : m_VectorSize(0) {}
  static SimdType scalar(BuiltinType::Kind k);
  static SimdType scalar(const SimdType& src) { return scalar(src.m_BuiltinType); }
  BuiltinType::Kind getBuiltinType() const { return m_BuiltinType; }
  unsigned getVectorSize() const { return m_VectorSize; }
  const void* getPtr() const { return m_OpaqueConfigEntry; }
  bool isValid() const { return m_VectorSize > 0; }
  bool isScalar() const { return m_VectorSize == 1; }

  bool operator<(const SimdType& other) const
  {
    return m_BuiltinType < other.m_BuiltinType || 
           (m_BuiltinType == other.m_BuiltinType && 
            m_VectorSize < other.m_VectorSize);
  }

  bool operator==(const SimdType& other) const
  {
    return m_BuiltinType == other.m_BuiltinType &&
            m_VectorSize == other.m_VectorSize;
  }

  friend class IntrinsicCollector;
};

//--------------------------------------------------------- 
class IntrinsicEditor : public StmtEditor
{
  IntrinsicCollector& getIntrinsicCollector() const;

protected:
  enum { MaxVectorSize = 0x7fffffff };
  SimdType retrieveSimdType(BuiltinType::Kind baseType, unsigned exclMaxSize = MaxVectorSize);
  SimdType retrieveGSIndexType(const SimdType& baseType);

public:
  explicit IntrinsicEditor(IntrinsicCollector& collector);

  QualType SimdType_(SimdType baseType);  
  QualType GSIndexType_(SimdType baseType);

  // built-in intrinsics
  CallExpr* IntrinsicCall_(const char* pName, Expr **args, unsigned numargs, SimdType baseType);
  template<unsigned N>
  CallExpr* IntrinsicCall_(const char* pName, Expr *(&args)[N], SimdType baseType);
  template<unsigned N>
  CallExpr* IntrinsicCall_(const char* pName, llvm::SmallVector<Expr*, N>& subExprs, SimdType baseType);
  FunctionDecl* getIntrinsicFn(FunctionDecl* fnDecl, SimdType baseType);
  FunctionDecl* getIntrinsicExpr(Expr* E, SimdType baseType, llvm::SmallVector<Expr*, 4>& subExprs);
  FunctionDecl* getConvertFn(SimdType sourceType, SimdType targetType, unsigned index);

  CallExpr* Extract_(DeclRefExpr* vectVarRef, unsigned index, SimdType baseType);
  CallExpr* Insert_(DeclRefExpr* vectVarRef, Expr* RHS_Node, 
                    unsigned index, const SimdType& baseType);
  CallExpr* Splat_(Expr* E, const SimdType& baseType);
  CallExpr* Broadcast_(Expr* E, const SimdType& baseType);
  CallExpr* StoreNtScalar_(Expr* targetLoc, Expr* RHS, unsigned index, const SimdType& baseType);

  void addOptionalGlobalCall(const char* pFnName, llvm::SmallVector<Stmt*, 32>& stmts);

  bool hasIntrinsicBuiltin(const char* pName, SimdType baseType) const;
  bool isIntrinsicFn(FunctionDecl* fnDecl) const;
  Expr* getSimplifiedLoc(Expr* E);
  Expr* getOnlySimplifiedLoc(Expr* E);
  
  unsigned getVectorByteAlignment(SimdType baseType) const;
};


//--------------------------------------------------------- inlines
template<unsigned N>
inline CallExpr* IntrinsicEditor::IntrinsicCall_(const char* pName, 
  Expr *(&args)[N], SimdType baseType)
{
  return IntrinsicCall_(pName, args, N, baseType);
}

//--------------------------------------------------------- 
template<unsigned N>
inline CallExpr* IntrinsicEditor::IntrinsicCall_(const char* pName, 
    llvm::SmallVector<Expr*, N>& subExprs, SimdType baseType)
{
  return IntrinsicCall_(pName, &subExprs[0], subExprs.size(), baseType);
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_INTRINSICEDITOR_H
