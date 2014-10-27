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

#include "clang/Vectorizing/VectorizeInfo.h"    
#include "clang/Vectorizing/TargetStmt.h"    
#include "clang/Vectorizing/ExpressionMatch.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/DeclCollector.h"    
#include <sstream>

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

//--------------------------------------------------------- 
std::string VectorizeInfo::getOriginalName(const Expr* E)
{
  std::string exprName, result;
  getAstAsString(*this, E, exprName);
  bool bWasUnderscore = false;
  result.reserve(exprName.size());
  for (unsigned i = 0; i < exprName.size(); ++i)
  {
    if ((exprName[i] >= '0' && exprName[i] <= '9') ||
        (exprName[i] >= 'a' && exprName[i] <= 'z') ||
        (exprName[i] >= 'A' && exprName[i] <= 'Z'))
    {
      bWasUnderscore = false;
      result.push_back(exprName[i]);
    }
    else if (!bWasUnderscore)
    {
      bWasUnderscore = true;
      if (i > 0)  // prevent leading underscores
      {
        result.push_back('_');
      }
    }
  }
  return result;
}

//--------------------------------------------------------- 
VectorizeInfo::VectorizeInfo(IntrinsicEditor& editor, 
                             tFunctionGlobalAssigns& globalAssigns) : 
  IntrinsicEditor(editor),
  m_ValueFlowCollector(Ctx(), &getFnDecl()),
  m_ValueFlowContext(m_ValueFlowCollector),
  m_VectorSize(0),
  m_VectorByteAlignment(0),
  m_GlobalAssigns(globalAssigns),
  m_NonTemporalExpr(0)
{
}

//--------------------------------------------------------- 
QualType VectorizeInfo::getTypeForRegion(Expr* Node)
{
  return Node->getType().getDesugaredType(Ctx())
                        .getNonReferenceType()
                        .getUnqualifiedType();
}

//--------------------------------------------------------- 
QualType VectorizeInfo::getTypeForRegion(const MemRegion* memRegion, Expr* Node)
{
  QualType result;
  if (const TypedValueRegion* TR = dyn_cast<TypedValueRegion>(memRegion))
  {
    result = TR->getDesugaredValueType(Ctx());
  }
  else
  {
    // commented out for defensive style
    // however I'm fairly sure that we never encounter untyped regions here
    // assert(isa<CodeTextRegion>(memRegion));
    result = Node->getType().getDesugaredType(Ctx());
  }
  return result.getNonReferenceType()
               .getUnqualifiedType();
}

//--------------------------------------------------------- 
bool VectorizeInfo::analyzeForInc(Expr* Node, 
                                  llvm::DenseSet<const MemRegion*>& incVars, 
                                  const char*& errMsg, bool bInnerLoop)
{
  if (ParenExpr* PE = dyn_cast<ParenExpr>(Node))
  {
    return analyzeForInc(PE->getSubExpr(), incVars, errMsg, bInnerLoop);
  }

  if (BinaryOperator* BO = dyn_cast<BinaryOperator>(Node))
  {
    switch (BO->getOpcode())
    {
    case BO_Comma:
      if (bInnerLoop)
      {
        errMsg = "composite increment not allowed in inner loop";  
        return false;
      }
      return analyzeForInc(BO->getLHS(), incVars, errMsg, bInnerLoop) &&
             analyzeForInc(BO->getRHS(), incVars, errMsg, bInnerLoop);

    case BO_AddAssign:
    case BO_SubAssign:
    {
      const MemRegion* incVar = m_ValueFlowContext.getLValueMemRegion(BO->getLHS());
      if (!incVar)
      {
        errMsg = "memory region for for-increment variable not recognized";  
        return false;
      }
      if (!incVars.insert(incVar).second)
      {
        errMsg = "incremented variable referenced twice in for-increment";  
        return false;
      }
      QualType type = getTypeForRegion(BO->getLHS());
      if (type->getAs<BuiltinType>() == 0)
      {
        errMsg = "incremented variable has complex type";  
        return false;
      }
      //we expect a constant stride expression
      Expr* strideExpr = BO->getOpcode() == BO_SubAssign ? 
        UnaryOp_(BO->getRHS(), UO_Minus) : BO->getRHS();

      createLiveMemRegion(incVar, CONSTANT, BO, bInnerLoop ? 
                            StridedExprResult::InnerLoopVariant() : 
                            StridedExprResult::Stride1(strideExpr));
      /*if (BO->getRHS()->isIntegerConstantExpr(Ctx()))
      {
        m_AnalyzedVars.insert(tSsaMemRegion(0, incVar, StridedExprResult::Stride1(BO->getRHS())));
      }
      else
      {
        //doesn't work yet, because we must introduce an indirect index array for this   
        m_AnalyzedVars.insert(tSsaMemRegion(0, incVar, STRIDE1_DEPENDENT));
      }*/
      return true;
    }

    default:
      errMsg = "binary for-increment expression not supported";  
      return false;
    }
  }

  if (UnaryOperator* UO = dyn_cast<UnaryOperator>(Node))
  {
    StridedExprResult incResult(STRIDE1);
    switch (UO->getOpcode())
    {
    case UO_PostDec:
    case UO_PreDec:
      incResult.m_StrideExpr = Int_(-1);
      // fall through
    case UO_PreInc:
    case UO_PostInc:
    {
      const MemRegion* incVar = m_ValueFlowContext.getLValueMemRegion(UO->getSubExpr());
      if (!incVar)
      {
        errMsg = "memory region for for-increment variable not recognized";  
        return false;
      }
      if (!incVars.insert(incVar).second)
      {
        errMsg = "incremented variable referenced twice in for-increment";  
        return false;
      }
      QualType type = getTypeForRegion(UO->getSubExpr());
      if (type->getAs<BuiltinType>() == 0)
      {
        errMsg = "incremented variable has complex type";  
        return false;
      }
      createLiveMemRegion(incVar, CONSTANT, UO, bInnerLoop ? 
                            StridedExprResult::InnerLoopVariant() : incResult);
      return true;
    }
    default:
      errMsg = "unary for-increment expression not supported";  
      return false;
    }
  }

  errMsg = "for-increment expression not supported";  
  return false;
}

//--------------------------------------------------------- 
bool VectorizeInfo::analyzeForStmt(ForStmt* Node, const char*& errMsg, bool bInnerLoop)
{
  BinaryOperator* condCmp = dyn_cast_or_null<BinaryOperator>(Node->getCond());
  if (condCmp == 0 || 
      ((!condCmp->isRelationalOp()) && condCmp->getOpcode() != BO_NE))
  {
    errMsg = "for-condition not fortran-like (must be x rel expr)";  
    return false;
  }
  if (!condCmp->getLHS()->getType()->isIntegerType())
  {
    errMsg = "for-condition variable is not an integral type";  
    return false;
  }
  const MemRegion* condVar = m_ValueFlowContext.getLValueMemRegion(condCmp->getLHS());
  if (!condVar)
  {
    errMsg = "memory region for for-condition variable not recognized";  
    return false;
  }

  llvm::DenseSet<const MemRegion*> incVars;
  if (!analyzeForInc(Node->getInc(), incVars, errMsg, bInnerLoop))
  {
    return false;
  }
  if (!incVars.count(condVar))
  {
    errMsg = "no reference to condition variable in increment stmt";  
    return false;
  }

  if (bInnerLoop)
  {
    BinaryOperator* BO;
    if ((BO = dyn_cast_or_null<BinaryOperator>(Node->getInit())) == 0 || 
        BO->getOpcode() != BO_Assign)
    {
      errMsg = "init expression for inner loop not a simple assignment";  
      return false;
    }

    if (m_ValueFlowContext.getLValueMemRegion(BO->getLHS()) != condVar)
    {
      errMsg = "inner loop init and condition don't refer the same memory region";
      return false;
    }
  }
  return true;
}

//--------------------------------------------------------- 
VectorizeInfo::tStatistics::tStatistics() :
  numLoadOps(0),
  numStoreOps(0),
  numPackedOps(0)
{}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getConstVar(const APValue& value, const SimdType& simdType)
{
  if (simdType.isScalar())
  {
    return createScalarConst(value, simdType.getBuiltinType());
  }

  tSimdConst simdConst(simdType, value);
  tTypedConstAssigns& constAssigns = m_GlobalAssigns.m_ConstAssigns;
  tTypedConstAssigns::left_const_iterator i = constAssigns.left.find(simdConst);
  VarDecl* constDecl;
  if (i == constAssigns.left.end())
  {
    constDecl = VarDecl_(SimdType_(simdType));
    constAssigns.insert(tTypedConstAssigns::value_type(simdConst, constDecl));
  }
  else
  {
    constDecl = i->second;
  }
  return DeclRef_(constDecl);
}

//--------------------------------------------------------- 
void VectorizeInfo::markAllIndexIndirect()
{
  m_AlignedArraySubscripts.empty();
  m_Stride1ArrayAccesses.clear();
  for (tExprResults::iterator i = m_AnalyzedExprs.begin(),
       e = m_AnalyzedExprs.end(); i != e; ++i)
  {
    if (i->second == STRIDE1)
    {
      i->second = STRIDE1_DEPENDENT;
    }
  }

  for (tVariableResults::iterator i = m_AnalyzedVars.begin(),
       e = m_AnalyzedVars.end(); i != e; ++i)
  {
    if (i->second.m_LHSResult == STRIDE1)
    {
      i->second.m_LHSResult = STRIDE1_DEPENDENT;
    }
    for (tUseSequence::iterator j = i->second.m_UseSequence.begin(),
         e2 = i->second.m_UseSequence.end(); j != e2; ++j)
    {
      if (j->m_Result == STRIDE1)
      {
        j->m_Result = STRIDE1_DEPENDENT;
      }
    }
  }
}


//--------------------------------------------------------- 
DeclStmt* VectorizeInfo::getScalarLoopInvariantVar(Expr* initializer, QualType type)
{
  std::string result;
  getAstAsString(*this, initializer, result);
  std::map<std::string, DeclStmt*>::const_iterator i = 
    m_ScalarLoopInvariantVarInitializer.find(result);
  if (i != m_ScalarLoopInvariantVarInitializer.end())
  {
    return i->second;
  }
  DeclStmt* tmpVar = TmpVar_(type);
  m_NonVectorizedTempVars.push_back(tmpVar);
  m_ScalarLoopInvariantExprs.push_back(Assign_(DeclRef_(tmpVar), initializer));
  m_ScalarLoopInvariantVarInitializer[result] = tmpVar;
  return tmpVar;
}

//--------------------------------------------------------- 
bool VectorizeInfo::tSimdConst::isValueLess(const APValue& other) const
{
  bool ownIsInt = m_Value.isInt();
  bool otherIsInt = other.isInt();
  if (ownIsInt != otherIsInt)
  {
    return otherIsInt;
  }
  if (ownIsInt)
  {
    if (m_Value.getInt().isUnsigned() != other.getInt().isUnsigned())
    {
      return other.getInt().isUnsigned();
    }
    else
    { 
      return m_Value.getInt().getLimitedValue() < 
             other.getInt().getLimitedValue();
    }
  }
  else
  {
    if (&m_Value.getFloat().getSemantics() != &other.getFloat().getSemantics())
    {
      return &m_Value.getFloat().getSemantics() < &other.getFloat().getSemantics();
    }
    return m_Value.getFloat().compare(other.getFloat()) == llvm::APFloat::cmpLessThan;
  }
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::createScalarExtract(DeclRefExpr* varRef, const SimdType& sourceType, unsigned idx)
{
  VarDecl* VD = dyn_cast<VarDecl>(varRef->getDecl());
  if (VD != 0)
  {
    tTypedConstAssigns::right_const_iterator ci = m_GlobalAssigns.m_ConstAssigns.right.find(VD);
    if (ci != m_GlobalAssigns.m_ConstAssigns.right.end())
    {
      return createScalarConst(ci->second.m_Value, ci->second.m_Type.getBuiltinType());
    }

    typedef tLoopInvariantAssigns::nth_index<1>::type tLoopInvariantAssignsByVar;
    tLoopInvariantAssignsByVar::iterator i = m_LoopInvariantAssigns.get<1>().find(VD);
    if (i != m_LoopInvariantAssigns.get<1>().end())
    {
      DeclRefExpr* invariantScalarVar = dyn_cast<DeclRefExpr>(i->m_RHSExpr);
      if (invariantScalarVar == 0)
      {
        DeclStmt* tmpVar = getScalarLoopInvariantVar(i->m_RHSExpr, BuiltinTy_(i->m_Type.getBuiltinType()));
        invariantScalarVar = DeclRef_(tmpVar);
        i->m_RHSExpr = invariantScalarVar;
      }
      return DeclRef_(cast<VarDecl>(invariantScalarVar->getDecl()));
    }
  }
  return Extract_(varRef, idx, sourceType);
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getSimplifiedScalar(Expr* Node, BuiltinType::Kind scalarType)
{
  if (hasSideEffects(*this, Node))
  {
    return 0;
  }

  StridedExprResult result = getAnalyzedExprResult(Node);
  QualType T (BuiltinTy_(scalarType));
  if (getAnalyzedExprResult(Node).m_Result == CONSTANT)  
  {
    APValue Result;
    if (EvaluateAsRValue_(Node, Result))
    {
      if (Result.isInt())
      {
        return T->getAs<BuiltinType>()->isFloatingPoint() ? 
          Float_(llvm::APFloat(double(Result.getInt().getLimitedValue())), T) :
          Int_(Result.getInt(), T);
      }
      else if (Result.isFloat())
      {
        return Float_(Result.getFloat(), T);
      }
    }
    return isSimple(Node) ? 
      Clone_(Node) : 
      DeclRef_(getScalarLoopInvariantVar(Node, T));
  }
  return 0;
}

//--------------------------------------------------------- 
void VectorizeInfo::moveConstAssigns()
{
  for (tVectExpressions::iterator i = m_AllExprs.begin(); i != m_AllExprs.end();)
  {
    BinaryOperator* Node = i->getAssign();
    if (Node && getAnalyzedExprResult(Node).isAbsolutConstant())
    {
      const tSsaInfo& memRegionInfo = getSsaMemRegion(Node->getLHS());
      if (memRegionInfo.first->second.m_UseSequence.size() == 1)
      {
        m_ScalarLoopInvariantExprs.push_back(Node);
        i = m_AllExprs.erase(i);
        removeStmt(Node);
        continue;  
      }
    }
    ++i;
  }
}

//--------------------------------------------------------- 
const VectorizeInfo::tReduction VectorizeInfo::s_AllReductionOps[] = {
  { "a=a+b",      tReduction::eZero },
  { "a=a-b",      tReduction::eZero },
  { "a=b+a",      tReduction::eZero },
  { "a=a*b",      tReduction::eOne },
  { "a=b*a",      tReduction::eOne },
  { "a=a<b?a:b",  tReduction::eSubArg1 }, // what about permutations?
  { "a=a<=b?a:b", tReduction::eSubArg1 },
  { "a=a>b?a:b",  tReduction::eSubArg1 },
  { "a=a>=b?a:b", tReduction::eSubArg1 },
  { 0, tReduction::eNone }
};

//--------------------------------------------------------- 
Expr* VectorizeInfo::getReductionInit(const BinaryOperator* BO, std::list<Expr*>& secondaryExprs)
{
  static std::vector<std::string> s_ExprString; 
  if (s_ExprString.empty())
  {
    for (size_t i = 0; s_AllReductionOps[i].m_Expression != 0; ++i)
    {
      s_ExprString.push_back(buildInternalExpressionString(s_AllReductionOps[i].m_Expression));
    }
  }

  for (size_t i = 0, e = s_ExprString.size(); i < e; ++i)
  {
    tAllSubExpressions subExprs;
    if (matchExpressionString(s_ExprString[i], BO, subExprs, *this))
    {
      assert(subExprs.size() == 2);
      secondaryExprs.swap(subExprs[1]);
      switch (s_AllReductionOps[i].m_UnitElement)
      {
        case tReduction::eZero:
          return Int_(0);
        case tReduction::eOne:
          return Int_(1);
        case tReduction::eSubArg1:
          return subExprs[0].back();
      }
    }
  }
  return 0;
}

//--------------------------------------------------------- 
const BinaryOperator* VectorizeInfo::testReductionReads(const Expr* writeExpr, 
  const std::list<const Expr*>& readExprs)
{
  const BinaryOperator* BO;
  if ((BO = dyn_cast_or_null<BinaryOperator>(getParentIgnore(writeExpr, IG_Paren|IG_ImpCasts))) == 0 ||
      BO->getOpcode() != BO_Assign)
  {
    Warn(writeExpr, "not vectorized: unsupported reduction-operation");
    return NULL;
  }
  for (std::list<const Expr*>::const_iterator i = readExprs.begin(), 
       e = readExprs.end(); i != e; ++i)
  {
    if (!isChildOf(*i, BO))
    {
      Warn(writeExpr, "not vectorized: irreducible read/write pattern");
      return NULL;
    }
  }
  return BO;
}

//--------------------------------------------------------- 
void VectorizeInfo::markExprsDependent(Expr* E)
{
  while (E != 0)
  {
    tExprResults::iterator i = m_AnalyzedExprs.find(E);
    if (i != m_AnalyzedExprs.end())
    {
      i->second = STRIDE1_DEPENDENT;
    }
    E = dyn_cast<Expr>(getParent(E));
  }
}

//--------------------------------------------------------- 
bool VectorizeInfo::markPreconditionWritesDependent(tUseSequence& useSequence, const MemRegion* memId)
{
  for (tUseSequence::iterator i = useSequence.begin(), e = --useSequence.end();
       i != e; ++i)
  {
    tUseSequence::iterator next = i;
    ++next;
    if (i->m_Result == CONSTANT && next->m_BranchStmt.getPointer() != 0)
    {
      const Expr* WE = i->m_Write.dyn_cast<const Expr*>();
      if (WE == 0)
      {
        const VarDecl* VD = i->m_Write.dyn_cast<const VarDecl*>();
        if (VD != 0)
        {
          WE = m_LocalVarWrites[memId];
          assert(WE != 0 && "uninitialized var declaration");
        }
      }

      if (WE != 0 && !i->m_ReadExprs.empty())
      {
        Warn(WE, "not vectorized: pre-conditional written variable used before condition");
        return false;
      }
      i->m_Result = STRIDE1_DEPENDENT;
      markExprsDependent(const_cast<Expr*>(WE));
    }
  }
  return true;
}

//--------------------------------------------------------- 
void VectorizeInfo::warnDangerousWrites(const tUseSequence& useSequence) 
{
  if (useSequence.size() == 1 && useSequence.back().m_ReadExprs.empty())
  {
    return; // OK, only one write
  }

  for (tUseSequence::const_iterator i = useSequence.begin(), e = useSequence.end();
       i != e; ++i)
  {
    const Expr* E = i->m_Write.get<const Expr*>(); // LHS != CONSTANT -> never a VarDecl
    if (E != 0)  
    {
      Note(E, "warning: vectorized handling of non-uniformly strided memory location");
    }
  }
}

//--------------------------------------------------------- 
bool VectorizeInfo::analyzeMemAccessPatterns()
{
  for (tVariableResults::iterator i = m_AnalyzedVars.begin(), 
       e = m_AnalyzedVars.end(); i != e; ++i)
  {
    const StridedExprResult& lhsResult = i->second.m_LHSResult;
    tUseSequence& useSequence = i->second.m_UseSequence;
    if (lhsResult == CONSTANT &&  // include inner loop strided lhs?
        useSequence.size() > 1)
    {
      if (useSequence.front().m_Write.isNull())
      {
        // write after read detected!
        const Expr* writeExpr = useSequence.back().m_Write.get<const Expr*>();
        if (useSequence.size() > 2 || 
            (!useSequence.back().m_ReadExprs.empty()))
        {
          Warn(writeExpr, "not vectorized: irreducible read/write pattern");
          return false;
        }
        if (useSequence.back().m_Result == CONSTANT)
        {
          Warn(writeExpr, "not vectorized: reducible read/write pattern, but loop-invariant operand (optimizeable?)");
          return false;
        }

        const BinaryOperator* BO = testReductionReads(writeExpr, useSequence.front().m_ReadExprs);
        if (BO == 0)
        {
          return false;
        }
        i->second.m_ReductionInitElement = getReductionInit(BO, i->second.m_ReductionSecondaryExprs);
        if (i->second.m_ReductionInitElement == 0)
        {
          Warn(writeExpr, "not vectorized: reducible read/write pattern, but unsupported operation");
          return false;
        }
      }
      if (!markPreconditionWritesDependent(useSequence, i->first))
      {
        return false;
      }
    }
  }
  return true;
}

//--------------------------------------------------------- 
bool VectorizeInfo::postInit(unsigned uForcedVectorSize, bool bIncludeGSTypes)
{
  if (!analyzeMemAccessPatterns())
  {
    return false;
  }

  moveConstAssigns();
  SimdType result = initVectorSize(bIncludeGSTypes);
  if (!result.isValid())
  {
    return false;
  }

  m_VectorSize = result.getVectorSize();
  if (uForcedVectorSize > 0)
  {
    if (uForcedVectorSize < m_VectorSize)
    {
      //Warn("not vectorized: forced vector size too small for vector type");
      return false;
    }
    if (uForcedVectorSize % m_VectorSize != 0)
    {
      //Warn("not vectorized: forced vector size incompatible to vector type");
      return false;
    }
    m_VectorSize = uForcedVectorSize;
  }
  m_VectorByteAlignment = getVectorByteAlignment(result);
  return true;
}

//--------------------------------------------------------- 
bool VectorizeInfo::mergeVectorSize(SimdType& result, const SimdType& simdType)
{ 
  m_SimdTypes[simdType.getBuiltinType()] = simdType;
  unsigned vs = simdType.getVectorSize();
  if (vs > result.getVectorSize())
  {
    if (result.getVectorSize() != 0 && vs % result.getVectorSize() != 0)
    {
      return false;
    }
    result = simdType;
  }
  return true;
}

//--------------------------------------------------------- 
SimdType VectorizeInfo::initVectorSize(bool bIncludeGSTypes)
{ 
  m_VectorSize = 1;
  SimdType result;
  for (tVectExpressions::iterator i = m_AllExprs.begin(), 
       e = m_AllExprs.end(); i != e; ++i)
  {
    BinaryOperator* BO = i->getAssign();
    if (BO != 0)
    {
      StridedExprResult ER = getAnalyzedExprResult(BO);
      if (ER != STRIDE1)
      {
        const BuiltinType *BT = stripParenCasts(BO->getLHS())->getType()->getAs<BuiltinType>();
        if (BT != 0)
        {
          SimdType simdType = retrieveSimdType(BT->getKind());
          if (simdType.isValid())
          {
            i->setVectorType(simdType);
            if (!mergeVectorSize(result, simdType))
            {
              Warn(BO, "not vectorized: incompatible mixed vector types");
              return SimdType();
            }

            if (bIncludeGSTypes)
            {
              SimdType gsType = retrieveGSIndexType(simdType);
              if (gsType.isValid() && !mergeVectorSize(result, gsType))
              {
                Warn(BO, "not vectorized: incompatible mixed vector types");
                return SimdType();
              }
            }

            const tSsaMemRegion& ssaRegion = getSsaMemRegion(BO->getLHS()).first->second;
            if (!i->isForcedUnroll() && ssaRegion.m_LHSResult == STRIDE1_DEPENDENT)
            {
              warnDangerousWrites(ssaRegion.m_UseSequence);
            }
            continue;
          }
        }
        if (ER != CONSTANT)
        {
          *i = TopLevelStmt::forceUnroll(BO);
        }
      }
    }
  }
  return result;
}

//--------------------------------------------------------- 
const SimdType& VectorizeInfo::getTargetType(BinaryOperator* BO) const
{
  const BuiltinType *BT = stripParenCasts(BO->getLHS())->getType()->getAs<BuiltinType>();
  tSimdTypes::const_iterator i = m_SimdTypes.find(BT->getKind());
  assert(i != m_SimdTypes.end());
  return i->second; 
}

//--------------------------------------------------------- 
void VectorizeInfo::initMostDirectArrayAccess()
{
  //TODO: replace by other means in order to get rid of m_Stride1ArrayAccesses:
  for (tArrayAccesses::iterator i = m_Stride1ArrayAccesses.begin(), 
       e = m_Stride1ArrayAccesses.end(); i != e; ++i)
  {
    if (i->second.size() > m_AlignedArraySubscripts.size() &&
        isVectorizedType(i->second.front()->getType()))
    {
      m_AlignedArraySubscripts.swap(i->second);
    }
  }
}

//--------------------------------------------------------- 
bool VectorizeInfo::isSubRegionOfAlignExpr(const MemRegion* memRegion, Expr* E)
{
  BinaryOperator* CE;
  const TypedValueRegion* alignedRegion;
  if ((CE = dyn_cast<BinaryOperator>(E)) != 0 && 
      CE->getOpcode() == BO_Comma)
  {
    return isSubRegionOfAlignExpr(memRegion, CE->getLHS()) ||
           isSubRegionOfAlignExpr(memRegion, CE->getRHS());
  }
  const MemRegion* exprRegion = m_ValueFlowContext.getLValueMemRegion(E);
  if ((alignedRegion = dyn_cast<TypedValueRegion>(exprRegion)) != 0 &&
      alignedRegion->getValueType()->isPointerType())
  {
    // deref pointers implicitely:
    exprRegion = m_ValueFlowContext.getSVal(exprRegion).getAsRegion();
  }    
  const SubRegion* SR = dyn_cast<SubRegion>(memRegion);
  return exprRegion != 0 && SR != 0 && 
         (SR == exprRegion || SR->isSubRegionOf(exprRegion));
}

//--------------------------------------------------------- 
void VectorizeInfo::initExplicitAlignment(Expr* alignExpr)
{
  assert(alignExpr != 0);
  for (tArrayAccesses::iterator i = m_Stride1ArrayAccesses.begin(), 
       e = m_Stride1ArrayAccesses.end(); i != e; ++i)
  {
    if (isSubRegionOfAlignExpr(i->first, alignExpr))
    {
      m_AlignedArraySubscripts.splice(m_AlignedArraySubscripts.end(), i->second);
    }
  }
}

//--------------------------------------------------------- 
bool VectorizeInfo::isNonTemporal(Expr* D)
{
  if (m_NonTemporalExpr == 0)
  {
    return false;
  }
  return isSubRegionOfAlignExpr(getSsaMemRegion(D).first->first, m_NonTemporalExpr);
}

//--------------------------------------------------------- 
void VectorizeInfo::initExplicitNonTemporal(Expr* ntExpr)
{
  m_NonTemporalExpr = ntExpr;
}

//--------------------------------------------------------- 
bool VectorizeInfo::isVectorizedType(const QualType& Type) const
{
  const BuiltinType *BT = Type->getAs<BuiltinType>();
  return BT != 0 && m_SimdTypes.find(BT->getKind()) != m_SimdTypes.end();
}

//--------------------------------------------------------- 
const SimdType* VectorizeInfo::getSimdType(const tSsaMemRegion& memRegion)
{
  const BuiltinType* BT = memRegion.m_ScalarType->getAs<BuiltinType>();
  if (BT != 0)  
  {
    tSimdTypes::const_iterator i = m_SimdTypes.find(BT->getKind());
    if (i != m_SimdTypes.end())
    {
      return &i->second;
    }
  }
  return 0;
}

//--------------------------------------------------------- 
const MemRegion* VectorizeInfo::getVarDecl(Expr* Node)
{
  return m_ValueFlowContext.getLValueMemRegion(Node);
}

//--------------------------------------------------------- 
bool VectorizeInfo::isAligned(Expr* Node) const
{
// TODO: we should introduce array accesses which are aligned anyway
  return std::find(m_AlignedArraySubscripts.begin(), 
                   m_AlignedArraySubscripts.end(), Node) != 
         m_AlignedArraySubscripts.end();
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::insertLoopInvariantVar(Expr* Node, const SimdType& targeType)
{
  std::string nodeStr;
  getAstAsString(*this, Node, nodeStr);
  return insertLoopInvariantVar(Node, nodeStr, targeType);
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::insertLoopInvariantVar(Expr* Node, 
  const std::string& nodeStr, const SimdType& targeType)
{
  typedef tLoopInvariantAssigns::nth_index<0>::type tLoopInvariantAssignsByString;
  typedef tLoopInvariantAssignsByString::iterator tIt;

  std::pair<tIt, tIt> range = m_LoopInvariantAssigns.get<0>().equal_range(nodeStr);
  for (tIt i = range.first; i != range.second; ++i)
  {
    if (targeType == i->m_Type)
    {
      return DeclRef_(i->m_TempVar);
    }
  }

  tInvariantAssign preLoopAssign;
  preLoopAssign.m_ExprAsString = nodeStr;
  preLoopAssign.m_TempVar = VarDecl_(SimdType_(targeType));
  preLoopAssign.m_Type = targeType;
  preLoopAssign.m_RHSExpr = Node;
  m_LoopInvariantAssigns.insert(preLoopAssign);
  Decl* D = preLoopAssign.m_TempVar;
  m_TempVars.push_back(DeclStmt_(&D, 1));
  return DeclRef_(preLoopAssign.m_TempVar);
}

//--------------------------------------------------------- 
DeclStmt* VectorizeInfo::getVectTempVar(const SimdType& targetType)
{
  DeclStmt* vectVarDecl = TmpVar_(SimdType_(targetType));
  m_TempVars.push_back(vectVarDecl);
  return vectVarDecl;
}

//--------------------------------------------------------- 
VectorizeInfo::tSsaMemRegion& VectorizeInfo::ensureSimdVar(const Expr* LHS)
{
  tSsaMemRegion& memRegion = getSsaMemRegion(LHS).first->second;
  assert(memRegion.m_BoundTempArray == 0 && "should not be vectorized");
  
  if (memRegion.m_BoundTempVar.empty())
  {
    createTempSimdVar(memRegion);
  }
  return memRegion;
} 

//--------------------------------------------------------- 
bool VectorizeInfo::needsWrite(const tSsaInfo& info) const
{
  tUseSequence::const_iterator usePoint = info.second;
  assert(usePoint->m_BranchStmt.getPointer() == 0);
  if (!usePoint->m_ReadExprs.empty())
  {
    return true;
  }
  if ((++usePoint) != info.first->second.m_UseSequence.end() &&
      usePoint->m_BranchStmt.getPointer() != 0)
  {
    return true;
  }
  return false;
}

//--------------------------------------------------------- 
DeclRefExpr* VectorizeInfo::getVectorizedVarLHS(const Expr* Node, int index, DeclRefExpr* aliasVar)
{
  if (aliasVar != 0 && !needsWrite(getSsaMemRegion(Node)))
  {
    // if that lhs Node is never read afterwards, then don't create or
    // touch a temporary if there is already one (aliasVar)
    return aliasVar;
  }

  tSsaMemRegion& memRegion = ensureSimdVar(Node);
  return DeclRef_(memRegion.m_BoundTempVar[index]);
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::createScalarToBoundVar(const tSsaMemRegion& memRegion, unsigned index)
{
  if (!memRegion.m_BoundTempVar.empty())
  {
    unsigned sourceVS = memRegion.m_BoundType.getVectorSize();
    return Extract_(DeclRef_(memRegion.m_BoundTempVar[index / sourceVS]), 
                    index % sourceVS, memRegion.m_BoundType);
  }
  if (memRegion.m_BoundTempArray != 0)
  {
    return ArraySubscript_(DeclRef_(memRegion.m_BoundTempArray), Int_(index));
  }
  return 0;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::createScalarAssignToBoundVar(const tSsaMemRegion& memRegion, Expr* RHS, unsigned index)
{
  if (!memRegion.m_BoundTempVar.empty())
  {
    unsigned sourceVS = memRegion.m_BoundType.getVectorSize();
    return Insert_(DeclRef_(memRegion.m_BoundTempVar[index / sourceVS]), RHS,
                   index % sourceVS, memRegion.m_BoundType);
  }
  if (memRegion.m_BoundTempArray != 0)
  {
    return Assign_(
      ArraySubscript_(DeclRef_(memRegion.m_BoundTempArray), Int_(index)),
      RHS);
  }
  return 0;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::createConvert(const tSsaMemRegion& memRegion, TargetStmt& target)
{
  const SimdType& targetType = target.getTargetType();
  unsigned index = target.getIndex();

  assert(!targetType.isScalar());
  assert(!memRegion.m_BoundTempVar.empty());
  if (targetType == memRegion.m_BoundType)
  {
    return DeclRef_(memRegion.m_BoundTempVar[index]);
  }

  unsigned targetVS = targetType.getVectorSize();
  unsigned scalarIndex = index * targetVS;
  unsigned sourceVS = memRegion.m_BoundType.getVectorSize();
  unsigned convertFnIndex = targetVS > sourceVS ? 0 : index % (sourceVS / targetVS);

  FunctionDecl* FD = getConvertFn(memRegion.m_BoundType, targetType, convertFnIndex);
  if (FD != 0)
  {
    unsigned numArgs = std::max(1u, targetVS / sourceVS);
    llvm::SmallVector<Expr*, 4> tempRefs(numArgs);
    unsigned startIdx = (scalarIndex / sourceVS);
    for (unsigned i = 0; i < numArgs; ++i)
    {
      tempRefs[i] = DeclRef_(memRegion.m_BoundTempVar[startIdx + i]);
    }
    return Call_(FD, &tempRefs[0], numArgs);
  }

  llvm::SmallVector<Expr*, 4> extractExprs(targetVS);
  for (unsigned i = scalarIndex, e = scalarIndex + targetVS; i < e; ++i)
  {
    extractExprs[i-scalarIndex] = Extract_(
      DeclRef_(memRegion.m_BoundTempVar[i / sourceVS]), 
      i % sourceVS, 
      memRegion.m_BoundType);
  }
  return IntrinsicCall_("set", extractExprs, targetType);
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::createExprToBoundVar(const tSsaMemRegion& memRegion, TargetStmt& target)
{
  const SimdType& targetType = target.getTargetType();
  unsigned index = target.getIndex();
  if (targetType.isScalar())
  {
    return createScalarToBoundVar(memRegion, index);
  }

  if (!memRegion.m_BoundTempVar.empty())
  {
    return createConvert(memRegion, target);
  }
  if (memRegion.m_BoundTempArray != 0)
  {
    unsigned targetVS = targetType.getVectorSize();
    unsigned scalarIndex = index * targetVS;
    llvm::SmallVector<Expr*, 4> extractExprs(targetVS);
    for (unsigned i = 0, e = targetVS; i < e; ++i)
    {
      extractExprs[i] = ArraySubscript_(DeclRef_(memRegion.m_BoundTempArray), Int_(scalarIndex+i));
    }
    return IntrinsicCall_("set", extractExprs, targetType);
  }
  return 0;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getBoundVectorizedVar(Expr* Node, int index)
{
  Node = stripParenCasts(Node);
  if (m_ExprToSsaMap.count(Node))
  {
    const tSsaInfo& memRegionInfo = getSsaMemRegion(Node);
    const tMemRegionUsage::tWrite& write = memRegionInfo.second->m_Write;
    if (memRegionInfo.second->m_Result == STRIDE1_DEPENDENT &&
        (write.isNull() || 
         (write.dyn_cast<const Expr*>() != 0 &&
          stripParenCasts(const_cast<Expr*>(write.get<const Expr*>())) != Node
         )))
    {
      // 1st clause: CONSTANT and STRIDE1 vars use the original var in  
      // the rollout regardless of introduced vectorized or arrayized vars
      // 2nd, 3rd clause: don't bind writing nodes
      return createScalarToBoundVar(memRegionInfo.first->second, index);
    }  
  }
  return 0;
}

//--------------------------------------------------------- 
bool VectorizeInfo::isRegionUsedOutside(const tSsaMemRegion& memRegion, Stmt* S) const
{
  for (tExprToSsaMap::const_iterator i = m_ExprToSsaMap.begin(), 
       e = m_ExprToSsaMap.end(); i != e; ++i)
  {
    if (&(i->second.first->second) == &memRegion && !isChildOf(i->first, S))
    {
      return true;
    }
  }
  return false;
}


//--------------------------------------------------------- 
DeclStmt* VectorizeInfo::getTempVarDecl(QualType T, const tMemRegionUsage::tWrite& Node)
{
  if (const Expr* E = Node.dyn_cast<const Expr*>())
  {
    return TmpVar_(T, 0, getOriginalName(E));
  }
  else if (const VarDecl* VD = Node.dyn_cast<const VarDecl*>())
  {
    return TmpVar_(T, 0, VD);
  }
  else
  {
    return TmpVar_(T);
  }
}

//--------------------------------------------------------- 
void VectorizeInfo::initTempSimdVar(tSsaMemRegion& memRegion)
{
  const SimdType* targetType = getSimdType(memRegion);
  assert(targetType);
  assert(m_VectorSize >= targetType->getVectorSize());
  int numVars = m_VectorSize / targetType->getVectorSize();
  memRegion.m_BoundTempVar.resize(numVars);
  memRegion.m_BoundType = *targetType;
}

//--------------------------------------------------------- 
void VectorizeInfo::createTempSimdVar(tSsaMemRegion& memRegion)
{
  initTempSimdVar(memRegion);
  for (unsigned i = 0; i < memRegion.m_BoundTempVar.size(); ++i)
  {
    DeclStmt* vectVarDecl = getTempVarDecl(SimdType_(memRegion.m_BoundType), 
                                           memRegion.m_UseSequence.back().m_Write);
    m_TempVars.push_back(vectVarDecl);
    memRegion.m_BoundTempVar[i] = cast<VarDecl>(vectVarDecl->getSingleDecl());
  }
}

//--------------------------------------------------------- 
void VectorizeInfo::createSplattedTempSimdVar(tSsaMemRegion& memRegion)
{
  initTempSimdVar(memRegion);
  DeclStmt* vectVarDecl = getTempVarDecl(SimdType_(memRegion.m_BoundType), 
                                          memRegion.m_UseSequence.back().m_Write);
  m_TempVars.push_back(vectVarDecl);
  VarDecl* VD = cast<VarDecl>(vectVarDecl->getSingleDecl());
  for (unsigned i = 0; i < memRegion.m_BoundTempVar.size(); ++i)
  {
    memRegion.m_BoundTempVar[i] = VD;
  }
}

//--------------------------------------------------------- 
void VectorizeInfo::createTempVar(tSsaMemRegion& memRegion)
{
  assert(memRegion.m_BoundTempVar.empty() && memRegion.m_BoundTempArray == 0);
  if (getSimdType(memRegion) == 0)
  {
    QualType arrayType = Ctx().getConstantArrayType(memRegion.m_ScalarType, llvm::APInt(32, m_VectorSize), ArrayType::Normal, 0);
    memRegion.m_BoundTempArray = getTempVarDecl(arrayType, memRegion.m_UseSequence.back().m_Write);
    m_NonVectorizedTempVars.push_back(memRegion.m_BoundTempArray);
  }
  else
  {
    createTempSimdVar(memRegion);
  }
}

//--------------------------------------------------------- 
VectorizeInfo::tUnrolledLHSResult VectorizeInfo::getVectorizedVarForUnrollLHS(const Expr* Node, Expr* targetRHS, Stmt* unrolledStmt, unsigned index)
{
  tUnrolledLHSResult result = {};
  const tSsaInfo& memRegionInfo = getSsaMemRegion(Node);
  if (memRegionInfo.second->m_Result != STRIDE1_DEPENDENT)
  {
    return result;
  }

  tSsaMemRegion& memRegion = memRegionInfo.first->second;
  const StridedExprResult& lValueResult = memRegion.m_LHSResult;
  if (memRegion.m_BoundTempVar.empty() && 
      memRegion.m_BoundTempArray == 0)
  {
    if (lValueResult != CONSTANT)
    {
      return result;  
    }

    if (!isRegionUsedOutside(memRegion, unrolledStmt))
    {
      return result;
    }

    createTempVar(memRegion);
  }

  if (lValueResult != CONSTANT && isLiveMemRegion(memRegionInfo))
  {
    result.m_TempVarExtract = createScalarToBoundVar(memRegion, index); 
  }
  result.m_Assign = createScalarAssignToBoundVar(memRegion, targetRHS, index);
  return result;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getGSIndex(Expr* E, const SimdType& targetType, tGSIndexAssigns& gsIndexAssigns, tGSDistanceInitKind k)
{
  std::string nodeStr;
  getAstAsString(*this, E, nodeStr);
  tTypedDistance key(targetType, nodeStr);  
  tGSIndexAssigns::iterator i = gsIndexAssigns.find(key);
  if (i == gsIndexAssigns.end())
  {
    tGSIndexAssign value = { 0, 0, E };
    if (!isSimple(E))
    {
      value.m_TempScalarVar = TmpVar_(Ctx().UnsignedIntTy);
    }
    if (k == SPLATTED)
    {
      value.m_TempIndexVar = TmpVar_(SimdType_(targetType));
    }
    else if (hasIntrinsicBuiltin("get_uniform_gs_index", targetType))
    {
      value.m_TempIndexVar = TmpVar_(GSIndexType_(targetType));
    }
    i = gsIndexAssigns.insert(tGSIndexAssigns::value_type(key, value)).first;
  }
  const tGSIndexAssign& gsAssigns = i->second;

  return gsAssigns.m_TempIndexVar != 0 ? DeclRef_(gsAssigns.m_TempIndexVar) :
         (gsAssigns.m_TempScalarVar != 0 ? DeclRef_(gsAssigns.m_TempScalarVar) :
          Clone_(gsAssigns.m_IndexExpr));
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getDistanceExpr(Expr* stride, const QualType& regionType, const SimdType& targetType)
{
  Expr* typeSize = Sizeof_(regionType.getCanonicalType().getUnqualifiedType());
  return stride != 0 ? 
    getGSIndex(BinaryOp_(Clone_(stride), typeSize, BO_Mul), targetType, m_LocalGSIndexAssigns, UNIFORM) : 
    getGSIndex(typeSize, targetType, m_GlobalAssigns.m_GSUniformIndexAssigns, UNIFORM);
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getDistanceExpr(Expr* stride, Expr* lValueExpr, const SimdType& targetType)
{
  const TypedValueRegion* MR = dyn_cast_or_null<TypedValueRegion>
    (m_ValueFlowContext.getRValueSVal(lValueExpr).getAsRegion());
  return MR != 0 ? getDistanceExpr(stride, MR->getValueType(), targetType) : 0;
}  

//--------------------------------------------------------- 
Expr* VectorizeInfo::getIndirectDistanceExpr(Expr* Node, const QualType& regionType, TargetStmt& target)
{
  const SimdType& targetType = target.getTargetType();
  SimdType gsType = retrieveGSIndexType(targetType);
  if (!gsType.isValid())
  {
    Warn(Node, "no configuration for indirect gather/scatter");
    return 0;
  }

  unsigned targetVS = targetType.getVectorSize();
  unsigned sourceVS = gsType.getVectorSize();
  if (targetVS > sourceVS)
  {
    Warn(Node, "configuration error: gather/scatter index type smaller than data type");
    return 0;
  }

  if (m_VectorSize < sourceVS || m_VectorSize % sourceVS != 0)
  {
    Warn(Node, "gather/scatter type not vectorized. Coding advice: store the index in a variable");
    return 0;
  }
  FunctionDecl* ConvertFN = 0;
  unsigned index = target.getIndex();
  if (targetVS < sourceVS)
  {
    SimdType gsFittingType = retrieveSimdType(gsType.getBuiltinType(), targetVS+1);
    if ((!gsFittingType.isValid()) || 
        targetVS != gsFittingType.getVectorSize())
    {
      Warn(Node, "gather/scatter type not vectorized: fitting vectorized index type needed");
      return 0;
    }
    unsigned convertFnIndex = targetVS > sourceVS ? 0 : index % (sourceVS / targetVS);
    ConvertFN = getConvertFn(gsType, gsFittingType, convertFnIndex);
    if (ConvertFN == 0)
    {
      Warn(Node, "gather/scatter type not vectorized: conversion function needed");
      return 0;
    }
    index = (index * targetVS) / sourceVS;
  }

  DeclRefExpr*& var = m_GSIndexExprs[tGSIndexExprs::key_type(Node, index)];
  if (var == 0)
  {
    TargetStmt gsIndexExpr(*this, gsType);
    var = gsIndexExpr.getVar(Node, index);
    // generates superfluous statements if there is no multiplication avail: 
    target.mergeStmts(gsIndexExpr); 
  }
  tTypedIndex indexVarKey(var->getDecl(), regionType);
  tGSIndexVars::const_iterator i = m_GSIndexVars.find(indexVarKey);
  if (i == m_GSIndexVars.end())
  {
    llvm::SmallVector<Expr*, 4> intrinsicArgs;
    Expr* typeSize = getGSIndex(Sizeof_(regionType), gsType, m_GlobalAssigns.m_GSSplattedIndexAssigns, SPLATTED);
    FunctionDecl* FD = getIntrinsicExpr(BinaryOp_(var, typeSize, BO_Mul), gsType, intrinsicArgs);
    if (FD == 0)
    {
      Warn(Node, "multiplication not configured for indirect gather/scatter");
      return 0;
    }
    DeclStmt* tmp = getVectTempVar(gsType);
    target.addStmt(Assign_(DeclRef_(tmp), Call_(FD, &intrinsicArgs[0], intrinsicArgs.size())));
    i = m_GSIndexVars.insert(tGSIndexVars::value_type(indexVarKey, tmp)).first;
  }
  Expr* result = DeclRef_(i->second);
  if (ConvertFN != 0)
  {
    Expr* intrinsicArgs[1] = { result };
    return Call_(ConvertFN, intrinsicArgs);
  }
  return result;
}

//--------------------------------------------------------- 
VectorizeInfo::GSDistanceResult VectorizeInfo::visitDistanceExpr(Expr* Node, TargetStmt& target)
{
  Node = Node->IgnoreParenCasts();
  MemberExpr* ME;
  UnaryOperator* UO;
  GSDistanceResult result;
  const SimdType& targetType = target.getTargetType();
  if ((ME = dyn_cast<MemberExpr>(Node)) != 0)
  {
    if (ME->isArrow())
    {
      // catch cases like (p+i)->m:
      StridedExprResult rValue = getAnalyzedExprResult(ME->getBase());
      if (rValue == STRIDE1)
      {
        result.m_IndexExpr = getDistanceExpr(rValue.m_StrideExpr, ME->getBase(), targetType);
      }
    }
    else
    {
      result = visitDistanceExpr(ME->getBase(), target);
      if (result.m_AddressExpr != 0)
      {
        result.m_AddressExpr = MemberPoint_(result.m_AddressExpr, cast<FieldDecl>(ME->getMemberDecl()));
      }
    }
  }
  if ((UO = dyn_cast<UnaryOperator>(Node)) != 0 && UO->getOpcode() == UO_Deref)
  {
    StridedExprResult rValue = getAnalyzedExprResult(UO->getSubExpr());
    if (rValue == STRIDE1)
    {
      result.m_IndexExpr = getDistanceExpr(rValue.m_StrideExpr, UO->getSubExpr(), targetType);
    }
  }
  if (ArraySubscriptExpr* ASE = dyn_cast<ArraySubscriptExpr>(Node)) 
  {
    Expr* indexNode = ASE->getIdx();
    StridedExprResult indexValue = getAnalyzedExprResult(indexNode);
    if (indexValue == CONSTANT)
    {
      // catch cases like a[i][0]:
      result = visitDistanceExpr(ASE->getBase(), target);
      if (result.m_AddressExpr != 0)
      {
        result.m_AddressExpr = ArraySubscript_(result.m_AddressExpr, Clone_(indexNode));
      }
    }
    if (indexValue == STRIDE1 || indexValue == STRIDE1_DEPENDENT) 
    {
      StridedExprResult baseValue = getAnalyzedExprResult(ASE->getBase());
      const TypedValueRegion* MR = dyn_cast_or_null<TypedValueRegion>
        (m_ValueFlowContext.getLValueMemRegion(ASE));
      if (baseValue == CONSTANT && MR != 0)
      {
        if (indexValue == STRIDE1)
        {
          result.m_IndexExpr = getDistanceExpr(indexValue.m_StrideExpr, MR->getValueType(), targetType);
        }
        else
        {
          result.m_IndexExpr = getIndirectDistanceExpr(indexNode, MR->getValueType(), target);
          if (result.m_IndexExpr != 0)
          {
            result.m_AddressExpr = ArraySubscript_(Clone_(ASE->getBase()), Int_(0));
          }
        }
      }
    }
  }
  return result;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::retrieveGatherScatterArgs(Expr* Node, Expr*& indexExpr, TargetStmt& target)
{
  const SimdType& targetType = target.getTargetType();
  const BuiltinType *BT = Node->getType()->getAs<BuiltinType>();
  if (BT == 0 || BT->getKind() != targetType.getBuiltinType())
  {
    return 0;
  }
  // distance in bytes:
  StridedExprResult lValResult = getLValueResult(Node);
  if (lValResult == STRIDE1)
  {
    indexExpr = getDistanceExpr(lValResult.m_StrideExpr, Node->getType(), targetType);
    return getSimplifiedIndexedLoc(Node, targetType, lValResult.m_StrideExpr, target.getIndex());
  }
  if (lValResult == STRIDE1_DEPENDENT)
  {
    GSDistanceResult result = visitDistanceExpr(Node, target);
    indexExpr = result.m_IndexExpr;
    if (indexExpr != 0)
    {
      if (result.m_AddressExpr != 0)
      {
        return getSimplifiedLoc(result.m_AddressExpr);
      }
      else
      {
        // haven't tested yet:
        return getSimplifiedIndexedLoc(Node, targetType, 0, target.getIndex());
      }
    }
  }
  return 0;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getSimplifiedIndexedLoc(Expr* Node, const SimdType& targetType, Expr* strideExpr, unsigned index)
{
  Expr* argNode = getSimplifiedLoc(Node);
  if (index > 0)
  {
    Expr* idxAdd = Int_(index * targetType.getVectorSize());
    if (strideExpr != 0)
    {
      idxAdd = BinaryOp_(idxAdd, Clone_(strideExpr), BO_Mul);
    }
    argNode = Add_(argNode, idxAdd);
  }
  return argNode;
}

//--------------------------------------------------------- 
bool VectorizeInfo::isStreamable(Expr* Node, const SimdType& targetType) const
{
  const BuiltinType *BT = Node->getType()->getAs<BuiltinType>();
  return BT != 0 &&
         BT->getKind() == targetType.getBuiltinType() && 
         getLValueResult(Node).isRealStride1();
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::evaluateConst(Expr* Node, const SimdType& targetType)
{
  APValue Result;
  if (EvaluateAsRValue_(Node, Result) && (Result.isInt() || Result.isFloat()))
  {
    return getConstVar(Result, targetType);
  }
  return 0;
}

//--------------------------------------------------------- 
bool VectorizeInfo::isTempLoadNeeded(tSsaMemRegion& rhsRegion) const
{
  size_t uNumReads = 0;
  for (tUseSequence::const_iterator i = rhsRegion.m_UseSequence.begin(), 
       e = rhsRegion.m_UseSequence.end(); i != e && uNumReads <= 1; ++i)
  {
    uNumReads += i->m_ReadExprs.size();
  }
  // there is more than one read to the rhs, 
  return uNumReads > 1;
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::getVectorizedVarRHS(Expr* Node, TargetStmt& target)
{
  if (m_ValueFlowContext.getLValueMemRegion(Node) == 0)
  {
    return evaluateConst(Node, target.getTargetType());
  }

  if (hasSideEffects(*this, Node))
  {
    //assert(0 && "FIXME");
    // ensure that Node is always executed targetType.m_VectorSize times
  }

  const tSsaInfo& memRegionInfo = getSsaMemRegion(Node);
  tSsaMemRegion& memRegion = memRegionInfo.first->second;
  switch (memRegionInfo.second->m_Result.m_Result)  
  {
    case CONSTANT: 
      if (memRegion.m_ReductionInitElement == 0)
      {
        Expr* result = createExprToBoundVar(memRegion, target);
        if (result == 0)
        {
          const SimdType& targetType = target.getTargetType();
          if (memRegionInfo.second->m_Result.m_bInnerLoopVariant)
          {
            if (targetType.isScalar())
            {
              result = Node;
            }
            else 
            {
              result = Broadcast_(Node, targetType);
              if (m_VectorSize > targetType.getVectorSize())
              {
                createSplattedTempSimdVar(memRegion);
                target.addStmt(Assign_(DeclRef_(memRegion.m_BoundTempVar[0]), result));
                result = DeclRef_(memRegion.m_BoundTempVar[0]);
              }
            }
          }
          else
          {
            result = insertLoopInvariantVar(Node, targetType);
          }
        }
        return result;
      }
      else
      {
        // special case: read the reduction var -> only create the array
        if (memRegion.m_BoundTempVar.empty() && 
            memRegion.m_BoundTempArray == 0)
        {
          createTempVar(memRegion);
        }
        return createExprToBoundVar(memRegion, target);
      }

    case STRIDE1:
      // TEST_IT: handle the case equivalent to STRIDE1_DEPENDENT
    case STRIDE1_DEPENDENT:
      if (memRegion.m_BoundTempVar.empty() && 
          memRegion.m_BoundTempArray == 0)
      {
        const SimdType* vectorType = getSimdType(memRegion);
        if (vectorType != 0)
        {
          int numVars = m_VectorSize / vectorType->getVectorSize();
          TargetStmt loadStmt(*this, *vectorType);
          const SimdType& targetType = target.getTargetType();
          if (numVars < 2 && 
              *vectorType == targetType && 
              !isTempLoadNeeded(memRegion))
          {
            Expr* result = loadStmt.generateLoad(Node, target.getIndex());
            target.mergeStmts(loadStmt);
            return result;
          }
          llvm::SmallVector<Expr*, 8> rhsExprs(numVars);
          for (int i = 0, e = numVars; i != e; ++i)
          {
            // generateLoad must come before createTempVar 
            // in order to avoid self-reference:
            rhsExprs[i] = loadStmt.generateLoad(Node, i);
          }
          target.mergeStmts(loadStmt);
          createTempSimdVar(memRegion);
          for (int i = 0, e = memRegion.m_BoundTempVar.size(); i != e; ++i)
          {
            target.addStmt(Assign_(DeclRef_(memRegion.m_BoundTempVar[i]), rhsExprs[i]));
          }
        }
        else
        {
          TargetScalarStmt::tUnrolledExprs rhsExprs;
          TargetScalarStmt(*this, m_VectorSize).rollout(Node, rhsExprs);
          createTempVar(memRegion);
          for (int i = 0, e = m_VectorSize; i != e; ++i)
          {
            target.addStmt(createScalarAssignToBoundVar(memRegion, rhsExprs[i], i));
          }
        }
      }
      return createExprToBoundVar(memRegion, target);

    case NOT_ANALYZED:
    default:               
      assert(0 && "unexpected mem region content");
      return 0;
  }
}

//--------------------------------------------------------- 
bool VectorizeInfo::createLiveMemRegion(const MemRegion* memRegion, StridedExprResult lhsResult, Expr* LHS, StridedExprResult result, llvm::PointerIntPair<Stmt*, 1> branchStmt)
{
  assert(LHS != 0);
  tVariableResults::iterator i = m_AnalyzedVars.find(memRegion);
  if (i == m_AnalyzedVars.end())
  {
    i = m_AnalyzedVars.insert(tVariableResults::value_type(memRegion, tSsaMemRegion(getTypeForRegion(memRegion, LHS), lhsResult))).first;
  }
  else if (i->second.m_LHSResult != lhsResult.m_Result)
  {
    return false;
  }
  if (lhsResult.isRealStride1())
  {
    m_Stride1ArrayAccesses[memRegion].push_back(LHS);
  }
  tUseSequence& useSequence = i->second.m_UseSequence;
  useSequence.push_back(tMemRegionUsage(LHS, result, branchStmt));
  m_ExprToSsaMap[stripParenCasts(LHS)] = tSsaInfo(&*i, --useSequence.end());
  return true;
}

//--------------------------------------------------------- 
void VectorizeInfo::createLiveMemRegion(const VarRegion* memRegion, VarDecl* VD, StridedExprResult result, llvm::PointerIntPair<Stmt*, 1> branchStmt)
{
  assert(VD != 0);
  QualType regionType = memRegion->getDesugaredValueType(Ctx())
    .getNonReferenceType()
    .getUnqualifiedType();

  // TODO: get a unique memRegion, if the same var is declared twice (in several compounds):
  tVariableResults::iterator i = m_AnalyzedVars.insert(tVariableResults::value_type(memRegion, tSsaMemRegion(regionType, CONSTANT))).first;
  tUseSequence& useSequence = i->second.m_UseSequence;
  useSequence.push_back(tMemRegionUsage(VD, result, branchStmt));
}

//--------------------------------------------------------- 
StridedExprResult VectorizeInfo::getLiveMemRegionResult(const MemRegion* memRegion, StridedExprResult lhsRes, Expr* RHS)
{
  RHS = stripParenCasts(RHS);
  tVariableResults::iterator i = m_AnalyzedVars.find(memRegion);
  if (i == m_AnalyzedVars.end())
  {
    i = m_AnalyzedVars.insert(tVariableResults::value_type(memRegion, tSsaMemRegion(getTypeForRegion(memRegion, RHS), lhsRes))).first;
  }
  else if (i->second.m_LHSResult != lhsRes.m_Result)
  {
    return NOT_ANALYZED;
  }
  tUseSequence& useSequence = i->second.m_UseSequence;
  if (useSequence.empty())
  {
    useSequence.push_back(tMemRegionUsage(StridedExprResult::LHStoRHS(lhsRes)));
  }
  if (lhsRes.isRealStride1())
  {
    m_Stride1ArrayAccesses[memRegion].push_back(RHS);
  }
  useSequence.back().m_ReadExprs.push_back(RHS);
  m_ExprToSsaMap[RHS] = tSsaInfo(&*i, --useSequence.end());
  return useSequence.back().m_Result;
}

//--------------------------------------------------------- 
bool VectorizeInfo::isLiveMemRegion(const tSsaInfo& memRegionInfo) const
{
  tUseSequence::const_iterator 
    curr_write = memRegionInfo.second,
    last_write = --memRegionInfo.first->second.m_UseSequence.end();
  llvm::PointerIntPair<Stmt*, 1> currentBranch = curr_write->m_BranchStmt;
  llvm::PointerIntPair<Stmt*, 1> lastBranch(0, 0);
  while (curr_write != last_write)
  {
    ++curr_write;
    const llvm::PointerIntPair<Stmt*, 1>& B = curr_write->m_BranchStmt;
    if (B == currentBranch ||  // subsequent write in the same branch (incl. global writes)
        B.getPointer() == 0)   // subsequent unconditional write 
    {
      return false;
    }
    if (lastBranch.getPointer() == B.getPointer() &&
        lastBranch.getInt() != B.getInt())
    {
      //two leafs of a branch after the current write perform a write:
      return false;
    }
    lastBranch = B; // pointer always != 0
  }
  return true;
}
/*
//--------------------------------------------------------- 
void VectorizeInfo::startBranch()
{
  assert(m_BranchedVarsStart.empty() && "call mergeAllBranches before!");
  m_BranchedVarsStart = m_AnalyzedVars;
}

//--------------------------------------------------------- 
void VectorizeInfo::nextBranch()
{
  m_BranchedVarsStack.push_back(tVariableResults());
  std::swap(m_BranchedVarsStack.last(), m_AnalyzedVars);
  m_AnalyzedVars = m_BranchedVarsStart;
}

//--------------------------------------------------------- 
void VectorizeInfo::mergeAllBranches()
{
  m_BranchedVarsStack.clear();
  m_BranchedVarsStart.clear();
}
*/
//--------------------------------------------------------- 
const VectorizeInfo::tSsaInfo& VectorizeInfo::getSsaMemRegion(const Expr* E) const
{
  E = stripParenCasts(const_cast<Expr*>(E));
  assert(m_ExprToSsaMap.count(E));
  return m_ExprToSsaMap.find(E)->second;
}

//--------------------------------------------------------- 
void VectorizeInfo::setAnalyzedExprResult(Expr* E, StridedExprResult result, bool expectExistence)
{
  Expr* Node = stripParenCasts(E);
  // this assertion is difficult to ensure as there may sub-expressions of
  // E (if E is a ParenExpr) already processed:
  //assert((m_AnalyzedExprs.count(Node) != 0) == expectExistence);
  m_AnalyzedExprs[Node] = result;
}

//--------------------------------------------------------- 
StridedExprResult VectorizeInfo::getAnalyzedExprResult(Expr* E) const
{
  Expr* Node = stripParenCasts(E);
  assert(m_AnalyzedExprs.count(Node));
  return m_AnalyzedExprs.lookup(Node);
}

//--------------------------------------------------------- 
const StridedExprResult& VectorizeInfo::getSimplifiedStride1Expr(Expr* E) 
{
  // FIXME: remove the hack!
  static const StridedExprResult dummy(CONSTANT);
  Expr* Node = stripParenCasts(E);
  if (!m_AnalyzedExprs.count(Node))
  {
    return dummy;
  }

  StridedExprResult& stride = m_AnalyzedExprs[Node];
  if (stride.m_Result == STRIDE1 && 
      stride.m_StrideExpr != 0 && 
      !isSimple(stride.m_StrideExpr))
  {
    stride.m_StrideExpr = DeclRef_(getScalarLoopInvariantVar(stride.m_StrideExpr, Ctx().IntTy));
  }
  return stride;
}

//--------------------------------------------------------- 
StridedExprResult VectorizeInfo::getLValueResult(Expr* Node) const
{
  return getSsaMemRegion(Node).first->second.m_LHSResult;
}

//--------------------------------------------------------- 
bool VectorizeInfo::isLive(Expr* LHS) 
{
  const tSsaInfo& memRegionInfo = getSsaMemRegion(LHS);
  return memRegionInfo.first->second.m_LHSResult != CONSTANT &&
         isLiveMemRegion(memRegionInfo);
}

//--------------------------------------------------------- 
Expr* VectorizeInfo::createReductionRHS(Expr* origRhs, Expr* extractExpr, 
                                        const std::list<Expr*>& secondaryExprs)
{
  StmtCloneMapping mapping;
  Expr* RHS = Clone_(origRhs, &mapping);
  for (std::list<Expr*>::const_iterator i = secondaryExprs.begin(), 
       e = secondaryExprs.end(); i != e; ++i)
  {
    assert(*i != origRhs);
    Expr* origAccess = const_cast<Expr*>(*i);
    assert(mapping.m_StmtMapping.count(origAccess));
    replaceStatement(mapping.m_StmtMapping[origAccess], 
                     i == secondaryExprs.begin() ? extractExpr : Clone_(extractExpr));
  }
  return RHS;
}

//--------------------------------------------------------- 
void VectorizeInfo::generateReductionPrePost(const tSsaMemRegion& reductionRegion,
                                             llvm::SmallVector<Stmt*, 32>& preStmts, 
                                             llvm::SmallVector<Stmt*, 32>& postStmts) 
{
  Expr* lhs = const_cast<Expr*>(reductionRegion.m_UseSequence.back().m_Write.get<const Expr*>());
  assert(lhs != 0);
  const BinaryOperator* BO = dyn_cast<BinaryOperator>(getParentIgnore(lhs, IG_Paren|IG_ImpCasts));
  assert(BO != 0 && BO->getOpcode() == BO_Assign);

  if (!reductionRegion.m_BoundTempVar.empty())
  {
    BuiltinType::Kind type = reductionRegion.m_BoundType.getBuiltinType();
    llvm::SmallVector<Stmt*, 4> reductionStmts(reductionRegion.m_BoundTempVar.size());
    DeclStmt* indexVar = TmpVar_(Ctx().UnsignedIntTy);
    m_NonVectorizedTempVars.push_back(indexVar);

    for (unsigned i = 0, e = reductionRegion.m_BoundTempVar.size(); i != e; ++i)
    {
      preStmts.push_back(Assign_(DeclRef_(reductionRegion.m_BoundTempVar[i]), 
                                 Splat_(Clone_(reductionRegion.m_ReductionInitElement), 
                                        reductionRegion.m_BoundType)));

      Expr* extractArgs[2] = { DeclRef_(reductionRegion.m_BoundTempVar[i]), DeclRef_(indexVar) };
      reductionStmts[i] = 
        Assign_(Clone_(lhs), 
                createReductionRHS(
                  BO->getRHS(),
                  IntrinsicCall_("extract", extractArgs, reductionRegion.m_BoundType),
                  reductionRegion.m_ReductionSecondaryExprs));

    }

    postStmts.push_back(
      For_(
        Assign_(DeclRef_(indexVar), UInt_(0)), 
        BinaryOp_(DeclRef_(indexVar), 
                  UInt_(reductionRegion.m_BoundType.getVectorSize()), 
                  BO_LT),
        UnaryOp_(DeclRef_(indexVar), UO_PreInc),
        Compound_(&reductionStmts[0], reductionStmts.size())));
  }
  else if (reductionRegion.m_BoundTempArray != 0)
  {
    DeclStmt* indexVar = TmpVar_(Ctx().UnsignedIntTy);
    m_NonVectorizedTempVars.push_back(indexVar);
    Stmt* initAssign[1] = { Assign_(
      ArraySubscript_(DeclRef_(reductionRegion.m_BoundTempArray), DeclRef_(indexVar)),
      Clone_(reductionRegion.m_ReductionInitElement)) };

    Stmt* reduction[1] = { Assign_(
      Clone_(lhs), 
      createReductionRHS(
        BO->getRHS(),
        ArraySubscript_(DeclRef_(reductionRegion.m_BoundTempArray), DeclRef_(indexVar)),
        reductionRegion.m_ReductionSecondaryExprs)) };

    preStmts.push_back(
      For_(
        Assign_(DeclRef_(indexVar), UInt_(0)), 
        BinaryOp_(DeclRef_(indexVar), UInt_(m_VectorSize), BO_LT),
        UnaryOp_(DeclRef_(indexVar), UO_PreInc),
        Compound_(initAssign)));

    postStmts.push_back(
      For_(
        Assign_(DeclRef_(indexVar), UInt_(0)), 
        BinaryOp_(DeclRef_(indexVar), UInt_(m_VectorSize), BO_LT),
        UnaryOp_(DeclRef_(indexVar), UO_PreInc),
        Compound_(reduction)));
  }
}

//--------------------------------------------------------- 
void VectorizeInfo::generateLoopEnclosingStmts(DeclCollector& collector,
  llvm::SmallVector<Stmt*, 32>& preStmts, llvm::SmallVector<Stmt*, 32>& postStmts) 
{

  preStmts.append(m_ScalarLoopInvariantExprs.begin(), 
                  m_ScalarLoopInvariantExprs.end());

  for (tLoopInvariantAssigns::const_iterator i = m_LoopInvariantAssigns.begin(),
       e = m_LoopInvariantAssigns.end(); i != e; ++i)
  {
    preStmts.push_back(Assign_(DeclRef_(i->m_TempVar), 
                               Broadcast_(i->m_RHSExpr, i->m_Type)));
  }
  
  generateGSAssigns(m_LocalGSIndexAssigns, preStmts, collector, *this, UNIFORM);

  addOptionalGlobalCall("vectorized_loop_epilog", postStmts);

  for (tVariableResults::const_iterator i = m_AnalyzedVars.begin(), 
       e = m_AnalyzedVars.end(); i != e; ++i)
  {
    if (i->second.m_ReductionInitElement != 0)
    {
      generateReductionPrePost(i->second, preStmts, postStmts);
    }
  }
  addOptionalGlobalCall("vectorized_loop_prolog", preStmts);
}

//--------------------------------------------------------- 
void VectorizeInfo::generateGSAssigns(const tGSIndexAssigns& gsAssigns, 
  llvm::SmallVector<Stmt*, 32>& stmts, DeclCollector& collector, 
  IntrinsicEditor& e, tGSDistanceInitKind k)
{
  for (tGSIndexAssigns::const_iterator i = gsAssigns.begin(), 
       eI = gsAssigns.end(); i != eI; ++i)
  {
    Expr* arg[1];
    if (i->second.m_TempScalarVar != 0)
    {
      stmts.push_back(e.Assign_(e.DeclRef_(i->second.m_TempScalarVar), 
                                e.Clone_(i->second.m_IndexExpr)));
      arg[0] = e.DeclRef_(i->second.m_TempScalarVar);
      collector.collect(i->second.m_TempScalarVar);
    }
    else
    {
      arg[0] = e.Clone_(i->second.m_IndexExpr);
    }
    if (i->second.m_TempIndexVar != 0)
    {
      stmts.push_back(e.Assign_(e.DeclRef_(i->second.m_TempIndexVar), 
        e.IntrinsicCall_(k == SPLATTED ? "splat" : "get_uniform_gs_index", arg, i->first.first)));
      collector.collect(i->second.m_TempIndexVar);
    }
  }
}

//--------------------------------------------------------- 
void VectorizeInfo::tFunctionGlobalAssigns::generateFunctionGlobalAssigns(
  DeclCollector& collector, llvm::SmallVector<Stmt*, 32>& topLevelStmts)
{
  for (tTypedConstAssigns::left_const_iterator i2 = m_ConstAssigns.left.begin(),
       e2 = m_ConstAssigns.left.end(); i2 != e2; ++i2)
  {
    topLevelStmts.push_back(Assign_(
      DeclRef_(i2->second), 
      Splat_(createScalarConst(i2->first.m_Value, 
                               i2->first.m_Type.getBuiltinType()), 
             i2->first.m_Type)));
    collector.collect(i2->second);    
  }
  VectorizeInfo::generateGSAssigns(m_GSUniformIndexAssigns, topLevelStmts, 
                                   collector, *this, UNIFORM);
  VectorizeInfo::generateGSAssigns(m_GSSplattedIndexAssigns, topLevelStmts, 
                                   collector, *this, SPLATTED);
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

