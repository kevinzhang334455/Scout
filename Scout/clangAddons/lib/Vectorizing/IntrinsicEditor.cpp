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

#include "clang/Vectorizing/IntrinsicEditor.h"
#include "clang/Vectorizing/IntrinsicCollector.h"
#include <algorithm>

//--------------------------------------------------------- 
namespace clang {
  namespace ASTProcessing {

//--------------------------------------------------------- 
SimdType SimdType::scalar(BuiltinType::Kind k)
{
  SimdType result;
  result.m_BuiltinType = k;
  result.m_VectorSize = 1;
  result.m_OpaqueConfigEntry = 0;
  return result;
}

//--------------------------------------------------------- 
IntrinsicEditor::IntrinsicEditor(IntrinsicCollector& collector) :
  StmtEditor(collector)
{}

//--------------------------------------------------------- 
IntrinsicCollector& IntrinsicEditor::getIntrinsicCollector() const
{ 
  return static_cast<IntrinsicCollector&>(m_Collector);
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicEditor::getIntrinsicFn(FunctionDecl* fnDecl, SimdType baseType)
{
  return getIntrinsicCollector().getIntrinsicFn(fnDecl, baseType);
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicEditor::getConvertFn(SimdType sourceType, SimdType targetType, unsigned index)
{
  return getIntrinsicCollector().getConvertFn(sourceType, targetType, index);
}

//--------------------------------------------------------- 
bool IntrinsicEditor::isIntrinsicFn(FunctionDecl* fnDecl) const
{
  return getIntrinsicCollector().isIntrinsicFn(fnDecl);
}

//--------------------------------------------------------- 
bool IntrinsicEditor::hasIntrinsicBuiltin(const char* pName, SimdType targetType) const
{
  return getIntrinsicCollector().hasIntrinsicBuiltin(pName, targetType);
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicEditor::getIntrinsicExpr(Expr* E, 
  SimdType baseType, llvm::SmallVector<Expr*, 4>& subExprs)
{
  return getIntrinsicCollector().getIntrinsicExpr(E, baseType, subExprs);
}

//--------------------------------------------------------- 
void IntrinsicEditor::addOptionalGlobalCall(const char* pFnName, llvm::SmallVector<Stmt*, 32>& stmts)
{
  FunctionDecl* FD = getIntrinsicCollector().getGlobalFn(pFnName);
  if (FD != 0)
  {
    stmts.push_back(Call_(FD, 0, 0));
  }
}

//--------------------------------------------------------- 
CallExpr* IntrinsicEditor::IntrinsicCall_(const char* pName, Expr **args, 
                                          unsigned numargs, SimdType baseType)
{
  return Call_(getIntrinsicCollector().getIntrinsicFn(pName, baseType), args, numargs);
}

//--------------------------------------------------------- 
CallExpr* IntrinsicEditor::Extract_(DeclRefExpr* vectVarRef, unsigned index, SimdType baseType)
{
  Expr* intrinsicArgs[2] = { vectVarRef, Int_(index) };
  return IntrinsicCall_("extract", intrinsicArgs, baseType);
} 

//--------------------------------------------------------- 
CallExpr* IntrinsicEditor::Insert_(DeclRefExpr* vectVarRef, Expr* RHS_Node, 
                                   unsigned index, const SimdType& baseType)
{
  Expr* intrinsicArgs[3] = { vectVarRef, RHS_Node, Int_(index) };
  return IntrinsicCall_("insert", intrinsicArgs, baseType);
}

//--------------------------------------------------------- 
CallExpr* IntrinsicEditor::StoreNtScalar_(Expr* targetLoc, Expr* RHS, 
                                          unsigned index, const SimdType& baseType)
{
  if (FunctionDecl* FD = getIntrinsicCollector().
        getIndexedIntrinsicFn("store_nt_scalar", baseType, index))
  {
    Expr* intrinsicArgs[2] = { targetLoc, RHS };
    return Call_(FD, intrinsicArgs);
  }
  else
  {  
    Expr* intrinsicArgs[3] = { targetLoc, RHS, Int_(index) };
    return IntrinsicCall_("store_nt_scalar", intrinsicArgs, baseType);
  }
}

//--------------------------------------------------------- 
CallExpr* IntrinsicEditor::Splat_(Expr* E, const SimdType& baseType)
{
  Expr* intrinsicArgs[1] = { E };
  return IntrinsicCall_("splat", intrinsicArgs, baseType);
}

//--------------------------------------------------------- 
CallExpr* IntrinsicEditor::Broadcast_(Expr* E, const SimdType& baseType)
{
  FunctionDecl* FD = getIntrinsicCollector().getIntrinsicFn("broadcast", baseType);
  if (FD == 0)
  {
    return Splat_(E, baseType);
  }
  Expr* intrinsicArgs[1] = { getSimplifiedLoc(E) };
  return Call_(FD, intrinsicArgs);
}

//--------------------------------------------------------- 
Expr* IntrinsicEditor::getSimplifiedLoc(Expr* E)
{
  Expr* Loc = getOnlySimplifiedLoc(E);
  return Loc == 0 ? UnaryOp_(Paren_(Clone_(E)), UO_AddrOf) : Loc;
}

//--------------------------------------------------------- 
Expr* IntrinsicEditor::getOnlySimplifiedLoc(Expr* E)
{
  UnaryOperator* UO; 
  if (ArraySubscriptExpr* ASE = dyn_cast<ArraySubscriptExpr>(E))
  {
    Expr* Base = Clone_(ASE->getBase());
    return EvaluateNull_(ASE->getIdx()) ?
      Base :
      Add_(Base, Paren_(Clone_(ASE->getIdx())));
  }
  else if ((UO = dyn_cast<UnaryOperator>(E)) != 0 && 
           UO->getOpcode() == UO_Deref)
  {
    return Clone_(UO->getSubExpr());
  }
  return 0;
}

//--------------------------------------------------------- 
QualType IntrinsicEditor::SimdType_(SimdType baseType)
{
  return getIntrinsicCollector().getSimdType(baseType);
}

//--------------------------------------------------------- 
QualType IntrinsicEditor::GSIndexType_(SimdType baseType)
{
  return getIntrinsicCollector().getGSIndexType(baseType);
}

//--------------------------------------------------------- 
SimdType IntrinsicEditor::retrieveSimdType(BuiltinType::Kind baseType, unsigned exclMaxSize)
{
  return getIntrinsicCollector().getSimdType(baseType, exclMaxSize);
}

//--------------------------------------------------------- 
SimdType IntrinsicEditor::retrieveGSIndexType(const SimdType& baseType)
{
  return getIntrinsicCollector().retrieveGSIndexType(baseType);
}

//--------------------------------------------------------- 
unsigned IntrinsicEditor::getVectorByteAlignment(SimdType baseType) const
{
  return getIntrinsicCollector().getVectorByteAlignment(baseType);
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // ns clang
