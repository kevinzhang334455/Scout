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

#include "clang/Vectorizing/Analysis.h"    
#include "clang/Vectorizing/VectorizeInfo.h"    
#include "clang/ASTProcessing/AttachedPragmas.h" 
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/AST/StmtVisitor.h"    
#include "../lib/Sema/TreeTransform.h"    

//--------------------------------------------------------- 
namespace clang {

struct CompoundAssignTransform : TreeTransform<CompoundAssignTransform>
{
  CompoundAssignTransform (Sema& s) : TreeTransform<CompoundAssignTransform>(s) {}

  bool AlwaysRebuild() { return true; }

  ExprResult TransformCompoundAssignOperator(CompoundAssignOperator *E) 
  {
#define OPASSIGN_TO_OP( OPCODE ) \
  case BO_ ## OPCODE ## Assign: \
    binOpc = BO_ ## OPCODE; break;

    BinaryOperator::Opcode binOpc;
    switch (E->getOpcode())
    {
      OPASSIGN_TO_OP(Add)
      OPASSIGN_TO_OP(Sub)
      OPASSIGN_TO_OP(Mul)
      OPASSIGN_TO_OP(Div)
      OPASSIGN_TO_OP(Rem)
      OPASSIGN_TO_OP(Shl)
      OPASSIGN_TO_OP(Shr)
      OPASSIGN_TO_OP(And)
      OPASSIGN_TO_OP(Or)
      OPASSIGN_TO_OP(Xor)
      default:
        assert(0 && "unknown compound op");
        return ExprResult(true);
    }
#undef OPASSIGN_TO_OP
      
    ExprResult lhsClone = TransformExpr(E->getLHS());
    if (lhsClone.isInvalid())
    {
      assert(0);
      return lhsClone;
    }
    ExprResult rhs = RebuildBinaryOperator(E->getOperatorLoc(), binOpc, lhsClone.get(), E->getRHS());
    if (rhs.isInvalid())
    {
      assert(0);
      return rhs;
    }
    return RebuildBinaryOperator(E->getOperatorLoc(), BO_Assign, E->getLHS(), rhs.get());
  }
};



namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
template<class T>
struct AnalyzeExprResult : StmtVisitor<T, StridedExprResult>, StmtEditor
{
  ConditionalOperator* m_ConstantNode;

  AnalyzeExprResult(StmtEditor& e) : StmtEditor(e), m_ConstantNode(0) {}

  void addStride(StridedExprResult& result, Expr* addExpr)
  {
    if (result.m_StrideExpr == 0)
    {
      result.m_StrideExpr = addExpr == 0 ? 
        Int_(2) : 
        BinaryOp_(addExpr, Int_(1), BO_Add);
    }
    else
    {
      result.m_StrideExpr = BinaryOp_(result.m_StrideExpr, 
        addExpr == 0 ? Int_(1) : addExpr, BO_Add);
    }
  }


  StridedExprResult mergeConstRes(StridedExprResult lhsRes, 
                                  const StridedExprResult& rhsRes)
  {
    if (lhsRes.m_Result == rhsRes.m_Result)
    {
      lhsRes.m_bInnerLoopVariant |= rhsRes.m_bInnerLoopVariant;
      return lhsRes;
    }
    return lhsRes == CONSTANT ? rhsRes : lhsRes;
  }


  StridedExprResult subVisit(Stmt* Node)
  {
    return static_cast<T*>(this)->Visit(Node);
  }

  StridedExprResult VisitStmt(Stmt* Node)
  {
    Warn(Node, "not vectorized: unsupported expression");  
    return NOT_ANALYZED;
  }

  StridedExprResult VisitBinAdd(BinaryOperator* Node)
  {
    StridedExprResult lhsRes = subVisit(Node->getLHS());
    if (lhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    StridedExprResult rhsRes = subVisit(Node->getRHS());
    if (rhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    if (lhsRes == CONSTANT || rhsRes == CONSTANT)
    {
      return mergeConstRes(lhsRes, rhsRes);
    }

    if (lhsRes == STRIDE1 && rhsRes == STRIDE1)
    {
      addStride(lhsRes, rhsRes.m_StrideExpr);
      return lhsRes;
    }

    return STRIDE1_DEPENDENT;
  }

  StridedExprResult VisitBinSub(BinaryOperator* Node)
  {
    StridedExprResult lhsRes = subVisit(Node->getLHS());
    if (lhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    StridedExprResult rhsRes = subVisit(Node->getRHS());
    if (rhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    if (rhsRes == STRIDE1)
    {
      rhsRes.m_StrideExpr = rhsRes.m_StrideExpr == 0 ? 
          Int_(-1) : UnaryOp_(rhsRes.m_StrideExpr, UO_Minus);
    }      

    if (lhsRes == CONSTANT || rhsRes == CONSTANT)
    {
      return mergeConstRes(lhsRes, rhsRes);
    }

    if (lhsRes == STRIDE1 && rhsRes == STRIDE1)
    {
      addStride(lhsRes, rhsRes.m_StrideExpr);
      return lhsRes;
    }

    return STRIDE1_DEPENDENT;
  }

  StridedExprResult VisitBinMul(BinaryOperator* Node)
  {
    StridedExprResult lhsRes = subVisit(Node->getLHS());
    if (lhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    StridedExprResult rhsRes = subVisit(Node->getRHS());
    if (rhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    if (lhsRes == CONSTANT && rhsRes == CONSTANT)
    {
      lhsRes.m_bInnerLoopVariant |= rhsRes.m_bInnerLoopVariant;
      return lhsRes;
    }

    if (lhsRes == CONSTANT && rhsRes == STRIDE1)
    {
      rhsRes.m_StrideExpr = rhsRes.m_StrideExpr == 0 ? 
        Node->getLHS() : 
        BinaryOp_(Node->getLHS(), rhsRes.m_StrideExpr, BO_Mul); 
      return rhsRes;
    }

    if (lhsRes == STRIDE1 && rhsRes == CONSTANT)
    {
      lhsRes.m_StrideExpr = lhsRes.m_StrideExpr == 0 ? 
        Node->getRHS() : 
        BinaryOp_(Node->getRHS(), lhsRes.m_StrideExpr, BO_Mul); 
      return lhsRes;
    }

    return STRIDE1_DEPENDENT;
  }

  StridedExprResult VisitBinComma(BinaryOperator* Node)
  {
    Warn(Node, "not vectorized: comma expression not supported yet");  
    return NOT_ANALYZED; //Visit(Node->getRHS());
  }

  StridedExprResult VisitCompoundAssignOperator(CompoundAssignOperator* Node)
  {
    Warn(Node, "not vectorized: compound assignment sub-expression not supported");  
    return NOT_ANALYZED;
  }

  StridedExprResult VisitBinaryOperator(BinaryOperator* Node)
  {
    if (Node->isAssignmentOp())
    {
      Warn(Node, "not vectorized: assignment sub-expression not supported");  
      return NOT_ANALYZED;
    }

    StridedExprResult lhsRes = subVisit(Node->getLHS());
    if (lhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    StridedExprResult rhsRes = subVisit(Node->getRHS());
    if (rhsRes == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }

    if (lhsRes == CONSTANT && rhsRes == CONSTANT)
    {
      lhsRes.m_bInnerLoopVariant |= rhsRes.m_bInnerLoopVariant;
      return lhsRes;
    }

    return STRIDE1_DEPENDENT;
  }

  StridedExprResult VisitUnaryPlus(UnaryOperator* Node)
  {
    return subVisit(Node->getSubExpr());
  }

  StridedExprResult VisitUnaryMinus(UnaryOperator* Node)
  {
    StridedExprResult result = subVisit(Node->getSubExpr());
    if (result == STRIDE1)
    {
      result.m_StrideExpr = result.m_StrideExpr == 0 ? 
        Int_(-1) : 
        UnaryOp_(result.m_StrideExpr, UO_Minus);
    }
    return result;
  }

  StridedExprResult VisitUnaryOperator(UnaryOperator* Node)
  {
    if (Node->isIncrementDecrementOp())
    {
      Warn(Node, "not vectorized: side effect not supported");  
      return NOT_ANALYZED;
      //return result == STRIDE1_DEPENDENT ? STRIDE1_DEPENDENT : NOT_ANALYZED;
    }
    StridedExprResult result = subVisit(Node->getSubExpr());
    return result == STRIDE1 ? STRIDE1_DEPENDENT : result;
  }

  StridedExprResult VisitIntegerLiteral(IntegerLiteral*)
  {
    return CONSTANT;
  }

  StridedExprResult VisitFloatingLiteral(FloatingLiteral*)
  {
    return CONSTANT;
  }

  StridedExprResult VisitCharacterLiteral(CharacterLiteral*)
  {
    return CONSTANT;
  }

  StridedExprResult VisitCallExpr(CallExpr* Node)
  {
    if (subVisit(Node->getCallee()) != CONSTANT)
    {
      Warn(Node, "not vectorized: variable function pointers not supported");  
      return NOT_ANALYZED;
    }

    const FunctionDecl *FD = findFunctionDecl(Node);
    bool dummy_fn = FD != NULL && findAttachedPragma(FD, "function", "dummy");
    // FIXME: enhance macro rewriting capabilites

    StridedExprResult result = CONSTANT;
    for (CallExpr::arg_iterator i = Node->arg_begin(), e = Node->arg_end(); i != e; ++i)
    {
      StridedExprResult argRes = subVisit(*i);
      if (argRes == NOT_ANALYZED)
      {
        return NOT_ANALYZED;
      }
      result.m_bInnerLoopVariant |= argRes.m_bInnerLoopVariant;
      if (argRes != CONSTANT)
      {
        result = dummy_fn ? argRes : STRIDE1_DEPENDENT;
      }
    }
    return result;
  }

  StridedExprResult VisitConditionalOperator(ConditionalOperator* Node)
  {
    StridedExprResult condResult, trueResult, falseResult;
    if ((condResult = subVisit(Node->getCond())) == NOT_ANALYZED || 
        (trueResult = subVisit(Node->getTrueExpr())) == NOT_ANALYZED || 
        (falseResult = subVisit(Node->getFalseExpr())) == NOT_ANALYZED)
    {
      return NOT_ANALYZED;
    }
    if (condResult.isAbsolutConstant())
    {
      if (trueResult.isAbsolutConstant() && falseResult.isAbsolutConstant())
      {
        return CONSTANT;
      }
      m_ConstantNode = Node;
    }

    // after a conditional operator a STRIDE1 expr is still never STRIDE1,
    // so we have to unroll anyway
    return STRIDE1_DEPENDENT;
  }

  StridedExprResult VisitCastExpr(CastExpr* Node)
  {
    StridedExprResult result = subVisit(Node->getSubExpr());
    // a promotion-only cast keeps the STRIDE1 kind
    return (isa<ImplicitCastExpr>(Node) || 
            isPromotionCast(Node) || 
            result != STRIDE1) ? result : STRIDE1_DEPENDENT;
  }

  StridedExprResult VisitParenExpr(ParenExpr* Node)
  {
    return subVisit(Node->getSubExpr());
  }
};

//--------------------------------------------------------- 
struct AnalyzeExprRValue;

//--------------------------------------------------------- 
struct AnalyzeExprLValue : AnalyzeExprResult<AnalyzeExprLValue>
{
  AnalyzeExprRValue&  m_AnalyseRValue;
public:
  AnalyzeExprLValue(AnalyzeExprRValue& analyser);

  StridedExprResult VisitMemberExpr(MemberExpr* Node);
  StridedExprResult VisitUnaryDeref(UnaryOperator* Node);
  StridedExprResult VisitArraySubscriptExpr(ArraySubscriptExpr* Node);
  StridedExprResult VisitDeclRefExpr(DeclRefExpr* Node);
};

//--------------------------------------------------------- 
enum tInnerLoopState { NO_INNER_LOOP, INNER_LOOP_VAR_INIT, INSIDE_INNER_LOOP };

//--------------------------------------------------------- 
struct AnalyzeExprRValue : AnalyzeExprResult<AnalyzeExprRValue>, ValueFlowContext
{
private:
  typedef AnalyzeExprResult<AnalyzeExprRValue> tBase;

  AnalyzeExprLValue m_AnalyseLValue;
  VectorizeInfo&    m_Info;

  StridedExprResult getDerefedMemoryContent(Expr* Node, const MemRegion* memRegion);

public:
  AnalyzeExprRValue(VectorizeInfo& info);
  StridedExprResult analyseBinAssign(BinaryOperator* Node, tInnerLoopState innerLoopState, llvm::PointerIntPair<Stmt*, 1> branchStmt);
  StridedExprResult analyseVarDecl(VarDecl* VD, Expr* Init, tInnerLoopState innerLoopState, llvm::PointerIntPair<Stmt*, 1> branchStmt);

  StridedExprResult Visit(Stmt* Node);
  StridedExprResult VisitDeclRefExpr(DeclRefExpr*);
  StridedExprResult VisitUnaryAddrOf(UnaryOperator*);
};


//--------------------------------------------------------- 
AnalyzeExprLValue::AnalyzeExprLValue(AnalyzeExprRValue& analyser) : 
  AnalyzeExprResult<AnalyzeExprLValue>(analyser),
  m_AnalyseRValue(analyser)
{}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprLValue::VisitMemberExpr(MemberExpr* Node)
{
  StridedExprResult result = Node->isArrow() ? 
      m_AnalyseRValue.Visit(Node->getBase()) : Visit(Node->getBase());
  // catch cases like a[i].x, which has not a STRIDE1 lvalue:
  return result == STRIDE1 ? STRIDE1_DEPENDENT : result;
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprLValue::VisitUnaryDeref(UnaryOperator* Node)
{
  return m_AnalyseRValue.Visit(Node->getSubExpr());
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprLValue::VisitArraySubscriptExpr(ArraySubscriptExpr* Node)
{
  StridedExprResult lhsRes = m_AnalyseRValue.Visit(Node->getBase());
  if (lhsRes == NOT_ANALYZED)
  {
    return NOT_ANALYZED;
  }

  StridedExprResult rhsRes = m_AnalyseRValue.Visit(Node->getIdx());
  if (lhsRes == CONSTANT || rhsRes == NOT_ANALYZED)
  {
    rhsRes.m_bInnerLoopVariant |= lhsRes.m_bInnerLoopVariant;
    return rhsRes;  // STRIDE1 for a[i]
  }
  if (lhsRes == STRIDE1 && rhsRes == CONSTANT)
  {
    //lhsRes == STRIDE1, rhsRes == CONSTANT: a[i][0] -> lvalue is STRIDED
    // the proper stride was already added at the RValue.Visit(getBase()) call
    return lhsRes;
  }
  return STRIDE1_DEPENDENT;
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprLValue::VisitDeclRefExpr(DeclRefExpr*)
{
  return CONSTANT;
}

//--------------------------------------------------------- 
AnalyzeExprRValue::AnalyzeExprRValue(VectorizeInfo& info) :
  AnalyzeExprResult<AnalyzeExprRValue>(info), 
  ValueFlowContext(info.m_ValueFlowContext), 
  m_AnalyseLValue(*this),
  m_Info(info)
{}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprRValue::getDerefedMemoryContent(Expr* Node, const MemRegion* memRegion)
{
  assert(memRegion);
  StridedExprResult lhsRes = m_AnalyseLValue.Visit(Node);
  // same logic as in GetRValueSVal::VisitExpr: arrays don't have rvalues:
  QualType resultType = Node->IgnoreParenImpCasts()->getType();
  if (resultType->isConstantArrayType())
  {
    if (lhsRes == STRIDE1)
    {
      Expr* stride = Int_(StmtEditor::Ctx().getAsConstantArrayType(resultType)->getSize().getLimitedValue());
      lhsRes.m_StrideExpr = lhsRes.m_StrideExpr == 0 ? 
        stride : 
        fold(BinaryOp_(Paren_(lhsRes.m_StrideExpr), stride, BO_Mul), *this);
    }
    return lhsRes;
  }
  return m_Info.getLiveMemRegionResult(memRegion, lhsRes, Node);
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprRValue::VisitUnaryAddrOf(UnaryOperator* Node)
{
  // we need to shortcut this, because:
  // 1. &(a[i]) is stride1, not stride1_dependent
  // 2. *(&a) would otherwise analyze the content of a and 
  // maybe introduce a bogus read op
  return m_AnalyseLValue.Visit(Node->getSubExpr());
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprRValue::Visit(Stmt* Node)
{
  StridedExprResult result = NOT_ANALYZED;
  if (Expr* E = dyn_cast<Expr>(Node))
  {
    const MemRegion* memRegion = getLValueMemRegion(E);
    result = memRegion == 0 ? 
      tBase::Visit(E) : 
      getDerefedMemoryContent(E, memRegion);

    m_Info.setAnalyzedExprResult(E, result);
  }
  else
  {
    Warn(Node, "not vectorized: statement detected, but expression expected");  
  }
  return result;
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprRValue::VisitDeclRefExpr(DeclRefExpr* Node)
{
  if (isa<EnumConstantDecl>(Node->getDecl()) || 
      isa<FunctionDecl>(Node->getDecl()))
  {
    return CONSTANT;
  }
  Warn(Node, "not vectorized: DeclRef resolves neither to constant nor lvalue");  
  return NOT_ANALYZED;
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprRValue::analyseBinAssign(BinaryOperator* Node, tInnerLoopState innerLoopState, llvm::PointerIntPair<Stmt*, 1> branchStmt)
{
  StridedExprResult lhsRes = m_AnalyseLValue.Visit(Node->getLHS());
  const MemRegion* memId = getLValueMemRegion(Node->getLHS());
  if (memId == 0 || lhsRes == NOT_ANALYZED)
  {
    Warn(Node, "not vectorized: lhs expression not supported yet");  
    return NOT_ANALYZED;
  }

  if (lhsRes == CONSTANT && lhsRes.m_bInnerLoopVariant)
  {
    Warn(Node, "not vectorized: lhs expression depends on nested loop var only");  
    return NOT_ANALYZED;
  }

  bindValue(memId, getRValueSVal(Node->getRHS()));
  StridedExprResult result = Visit(Node->getRHS());
  if (result == NOT_ANALYZED)
  {
    return NOT_ANALYZED;
  }

  if (branchStmt.getPointer() != 0)
  {
    result = STRIDE1_DEPENDENT;
  }

  if (lhsRes != CONSTANT)
  {
    result = STRIDE1_DEPENDENT;
  }

  if (innerLoopState == INNER_LOOP_VAR_INIT)
  {
    if (result != CONSTANT)
    {
      Warn(Node, "not vectorized: nested loop variable not invariant to vectroization");  
      return NOT_ANALYZED;
    }
    result.m_bInnerLoopVariant = true;
  }

  if (!m_Info.createLiveMemRegion(memId, lhsRes, Node->getLHS(), result, branchStmt))
  {
    Warn(Node, "not vectorized: local variable carry dependency between iterations");  
    return NOT_ANALYZED;
  }

  m_Info.setAnalyzedExprResult(Node->getLHS(), result);
  m_Info.setAnalyzedExprResult(Node, result);
  m_Info.m_LocalVarWrites[memId] = Node->getRHS();
  return result;
}

//--------------------------------------------------------- 
StridedExprResult AnalyzeExprRValue::analyseVarDecl(VarDecl* VD, Expr* Init, tInnerLoopState innerLoopState, llvm::PointerIntPair<Stmt*, 1> branchStmt)
{
  StridedExprResult result(CONSTANT); 
  const VarRegion* memId = getMemRegion(VD);
  bindValue(memId, getRValueSVal(Init));
  result = Visit(Init);
  if (result == NOT_ANALYZED)
  {
    return NOT_ANALYZED;
  }

  if (branchStmt.getPointer() != 0)
  {
    result = STRIDE1_DEPENDENT;
  }

  if (innerLoopState == INNER_LOOP_VAR_INIT)
  {
    if (result != CONSTANT)
    {
      Warn(Init, "not vectorized: nested loop variable not invariant to vectorization");  
      return NOT_ANALYZED;
    }
    result.m_bInnerLoopVariant = true;
  }

  m_Info.createLiveMemRegion(memId, VD, result, branchStmt);
  m_Info.m_LocalVarWrites[memId] = Init;
  return result;
}

//--------------------------------------------------------- 
struct CollectorResult
{
  enum { NOT_VECTORIZABLE, DIVE_IN, DIVE_IN_LOOP, SKIP_CHILDREN, 
         SPLIT_INVARIANT_IF, TRANSFORM_GUARDING_IF, UNROLL_VARIANT_IF };

  CollectorResult(unsigned res) : m_Value(res), m_DiveInLoop(0) {}

  unsigned m_Value;
  union {
    Stmt*   m_InvariantIf;
    IfStmt* m_UnrollIf;
    IfStmt* m_GuardingIf;
    ForStmt*  m_DiveInLoop;
  };
  boost::logic::tribool m_StaticInvariantIfResult;
};

//--------------------------------------------------------- 
class VectorizeableExprsCollector : public StmtVisitor<VectorizeableExprsCollector, CollectorResult>, StmtEditor
{
  VectorizeInfo& m_Info;
  AnalyzeExprRValue m_Analyser;

public:
  llvm::PointerIntPair<Stmt*, 1>  m_BranchStmt;
  unsigned  m_InnerLoopNest;

  template<class T>
  CollectorResult InvariantIf(T* S)
  {
    CollectorResult result(CollectorResult::SPLIT_INVARIANT_IF);
    result.m_InvariantIf = S;
    result.m_StaticInvariantIfResult = boost::logic::indeterminate;
    Optional<nonloc::ConcreteInt> CI = m_Analyser.getRValueSVal(S->getCond()).template getAs<nonloc::ConcreteInt>();
    if (CI)
    {
      result.m_StaticInvariantIfResult = CI->getValue().getBoolValue();
    }
    return result;
  }

  static CollectorResult UnrollIf(IfStmt* S)
  {
    CollectorResult result(CollectorResult::UNROLL_VARIANT_IF);
    result.m_UnrollIf = S;
    return result;
  }

  static CollectorResult GuardingIf(IfStmt* S)
  {
    CollectorResult result(CollectorResult::TRANSFORM_GUARDING_IF);
    result.m_GuardingIf = S;
    return result;
  }

  static CollectorResult DiveInLoop(ForStmt* S)
  {
    CollectorResult result(CollectorResult::DIVE_IN_LOOP);
    result.m_DiveInLoop = S;
    return result;
  }



  VectorizeableExprsCollector(VectorizeInfo& info) :
    StmtEditor(info),
    m_Info(info),
    m_Analyser(info),
    m_InnerLoopNest(0)
  {}

  CollectorResult VisitBinAssign(BinaryOperator* Node)
  {
    StridedExprResult res = m_Analyser.analyseBinAssign(Node, 
      m_InnerLoopNest > 0 ? INSIDE_INNER_LOOP : NO_INNER_LOOP, m_BranchStmt);
    if (res == NOT_ANALYZED)
    {
      std::string result;
      getAstAsString(*this, Node, result);
      Warn(Node, "not vectorized: binary expression not analyzed (%0)") << result;  
      return CollectorResult::NOT_VECTORIZABLE;
    }
    if (m_BranchStmt.getPointer() != 0)
    {
      return CollectorResult::SKIP_CHILDREN;
    }

    const PragmaArgumentInfo* pragmaInfo = 
      m_Info.findAttachedPragma(Node, "loop", "vectorize");
    bool bUnrollMode = pragmaInfo != 0 && pragmaInfo->hasArgument("unroll");

    if ((!bUnrollMode) && m_Analyser.m_ConstantNode != 0)
    {
      return InvariantIf(m_Analyser.m_ConstantNode);
    }
    m_Info.m_AllExprs.push_back(bUnrollMode ? 
                                TopLevelStmt::forceUnroll(Node) : 
                                TopLevelStmt::vectorize(Node));

    return CollectorResult::SKIP_CHILDREN;
  }

  CollectorResult VisitCompoundAssignOperator(CompoundAssignOperator* Node)
  {
    Sema::ContextRAII raiiHolder(getSema(), &getFnDecl());
    ExprResult res = CompoundAssignTransform (getSema()).TransformCompoundAssignOperator(Node);
    if (res.isInvalid())
    {
      return NOT_ANALYZED;
    }
    BinaryOperator* transformedCompound = cast<BinaryOperator>(res.get());
    replaceStatement(Node, transformedCompound);
    return VisitBinAssign(transformedCompound);
  }
  
  CollectorResult VisitNullStmt(NullStmt*)
  {
    return CollectorResult::SKIP_CHILDREN;
  }

  CollectorResult VisitCompoundStmt(CompoundStmt*)
  {
    return CollectorResult::DIVE_IN;
  }

  CollectorResult VisitIfStmt(IfStmt* Node)
  {
    StridedExprResult res = m_Analyser.Visit(Node->getCond());
    if (m_BranchStmt.getPointer() != 0)
    {
      // TODO: analyse nested conditions, at the moment 
      // dive in leads to condition analysis, which fails
      Warn(Node, "not vectorized: nested conditions not supported");  
      return CollectorResult::NOT_VECTORIZABLE;
    }

    if (res.isAbsolutConstant())    
    {
      Warn(Node, "vectorization analysis detects invariant if: loop splitted");
      return InvariantIf(Node);
    }
    switch (res.m_Result)
    {
      case STRIDE1:
      case STRIDE1_DEPENDENT:
        if (m_Info.m_AllExprs.empty())
        {
          CompoundStmt* loopBody = dyn_cast<CompoundStmt>(getParent(Node));
          // TODO: remove these conditons step by step:
          if (loopBody != 0 && 
              dyn_cast<ForStmt>(getParent(loopBody)) != 0 &&
              (*loopBody->body_begin()) == Node &&
              Node->getElse() == 0)   // FIXME: reset the mem region access between if and else
          {
            return GuardingIf(Node);
          }
        }
        // fall through
      case CONSTANT:  // inner loop
        // gets unrolled:
        m_Info.m_AllExprs.push_back(TopLevelStmt::unroll(Node));
        return UnrollIf(Node);

      default:
        Warn(Node, "not vectorized: unsupported conditional control flow");  
        return CollectorResult::NOT_VECTORIZABLE;
    }
  }

  CollectorResult VisitForStmt(ForStmt* Node)
  {
    if (m_BranchStmt.getPointer() != 0)
    {
      // TODO: analyse nested for-stmts, at the moment 
      // dive in leads to condition analysis, which fails
      Warn(Node, "not vectorized: nested for-stmts in conditions not supported");  
      return CollectorResult::NOT_VECTORIZABLE;
    }

    const char* errMsg;
    if (m_Info.analyzeForStmt(Node, errMsg, true))
    {
      if (m_Analyser.Visit(dyn_cast<BinaryOperator>(Node->getInit())->getRHS()) == CONSTANT && 
          m_Analyser.Visit(Node->getCond()) == CONSTANT)
      {
        // analyzeForStmt has set var as a CONSTANT innerLoopVariant variable
        return DiveInLoop(Node);
      }
      Warn(Node, "not vectorized: inner loop initialization or condition not constant");
    }
    else
    {
      Warn(Node, errMsg);
    }
    return CollectorResult::NOT_VECTORIZABLE;
  }

  CollectorResult VisitDeclStmt(DeclStmt* Node)
  {
    Warn(Node, "not vectorized: TODO: inline declaration of variables");
    return CollectorResult::NOT_VECTORIZABLE;

    if (!Node->isSingleDecl())  
    {
      Warn(Node, "not vectorized: declaration groups not supported (TODO: break up before)");
      return CollectorResult::NOT_VECTORIZABLE;               
    }
    VarDecl* VD = dyn_cast<VarDecl>(Node->getSingleDecl());
    if (VD == 0)
    {
      Warn(Node, "not vectorized: declaration is not a variable");
      return CollectorResult::NOT_VECTORIZABLE;               
    }
    Expr* IE = VD->getInit();
    if (IE == 0)
    {
      // what should we do here: leave it, remove it?
      return CollectorResult::SKIP_CHILDREN;  
    }

    StridedExprResult res = m_Analyser.analyseVarDecl(VD, IE,
      m_InnerLoopNest > 0 ? INSIDE_INNER_LOOP : NO_INNER_LOOP, m_BranchStmt);

    if (res == NOT_ANALYZED)
    {
      Warn(Node, "not vectorized: variable declaration not analyzed (%0)") << VD->getName();  
      return CollectorResult::NOT_VECTORIZABLE;
    }
    if (m_BranchStmt.getPointer() != 0)
    {
      return CollectorResult::SKIP_CHILDREN;
    }
    const PragmaArgumentInfo* pragmaInfo = 
      m_Info.findAttachedPragma(Node, "loop", "vectorize");
    bool bUnrollMode = pragmaInfo != 0 && pragmaInfo->hasArgument("unroll");
    if ((!bUnrollMode) && m_Analyser.m_ConstantNode != 0)
    {
      return InvariantIf(m_Analyser.m_ConstantNode);
    }
/*
    m_Info.m_AllExprs.push_back(bUnrollMode ? 
                                TopLevelStmt::forceUnroll(Node) : 
                                TopLevelStmt::vectorize(Node));
*/
    return CollectorResult::SKIP_CHILDREN;
  }


#define NOT_VECTORIZED(CLASS, REASON)     \
CollectorResult Visit ## CLASS(CLASS *Node)      \
{                                         \
  Warn(Node, "not vectorized: " REASON);  \
  return CollectorResult::NOT_VECTORIZABLE;                \
}                 

  NOT_VECTORIZED(WhileStmt, "no inner loop")
  NOT_VECTORIZED(DoStmt, "no inner loop")
  NOT_VECTORIZED(BreakStmt, "unsuppported loop control flow (top-level break!)")
  NOT_VECTORIZED(ContinueStmt, "unsuppported loop control flow (top-level continue!)")
  NOT_VECTORIZED(SwitchStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(GotoStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(IndirectGotoStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(ReturnStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(CXXCatchStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(CXXTryStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(LabelStmt, "unsuppported loop control flow")
  NOT_VECTORIZED(Stmt, "unsuppported statement or expression")

#undef NOT_VECTORIZED
};


//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
inline AnalyzeResult AnalyzeResult::Vectorize(bool doIt)
{
  AnalyzeResult result = { doIt ? VECTORIZE : DONT_VECTORIZE };
  return result;
}

//--------------------------------------------------------- 
inline AnalyzeResult AnalyzeResult::SplitIf(Stmt* invariantIf, 
                                            boost::logic::tribool staticResult)
{
  AnalyzeResult result = { SPLIT_IF };
  result.m_InvariantIf = invariantIf;
  result.m_StaticInvariantIfResult = staticResult;
  return result;
}

//--------------------------------------------------------- 
inline AnalyzeResult AnalyzeResult::GuardingIf(IfStmt* guardingIf)
{
  AnalyzeResult result = { GUARDING_IF };
  result.m_GuardingIf = guardingIf;
  return result;
}

//--------------------------------------------------------- 
AnalyzeResult analyzeNode(Stmt* Node, VectorizeableExprsCollector& collector)
{
  for (llvm::df_iterator<Stmt*> i = llvm::df_begin(Node), 
       e = llvm::df_end(Node); i != e;)
  {
    CollectorResult result = collector.Visit(*i);
    switch (result.m_Value)
    {
    case CollectorResult::NOT_VECTORIZABLE:
      return AnalyzeResult::Vectorize(false);
    case CollectorResult::SPLIT_INVARIANT_IF:
      return AnalyzeResult::SplitIf(result.m_InvariantIf, result.m_StaticInvariantIfResult);  
    case CollectorResult::TRANSFORM_GUARDING_IF:
      return AnalyzeResult::GuardingIf(result.m_GuardingIf);

    case CollectorResult::UNROLL_VARIANT_IF:
      {
        i.skipChildren();
        VectorizeableExprsCollector ifCollector(collector);
        ifCollector.m_BranchStmt = llvm::PointerIntPair<Stmt*, 1>(result.m_UnrollIf, 1);
        if (analyzeNode(result.m_UnrollIf->getThen(), ifCollector).m_Result != AnalyzeResult::VECTORIZE)
        {
          return AnalyzeResult::Vectorize(false);
        }
        ifCollector.m_BranchStmt.setInt(0);
        if (result.m_UnrollIf->getElse() != 0 && 
            analyzeNode(result.m_UnrollIf->getElse(), ifCollector).m_Result != AnalyzeResult::VECTORIZE)
        {
          return AnalyzeResult::Vectorize(false);
        }
      }
      break;
    case CollectorResult::DIVE_IN_LOOP:
      {
        i.skipChildren();
        ++collector.m_InnerLoopNest;
        AnalyzeResult innerLoopRes = analyzeNode(result.m_DiveInLoop->getBody(), collector);
        --collector.m_InnerLoopNest;
        if (innerLoopRes.m_Result != AnalyzeResult::VECTORIZE)
        {
          return innerLoopRes.m_Result == AnalyzeResult::SPLIT_IF ? 
            innerLoopRes : 
            AnalyzeResult::Vectorize(false);
        }
      }
      break;
    case CollectorResult::SKIP_CHILDREN:
      i.skipChildren();
      break;
    case CollectorResult::DIVE_IN:
      ++i;
      break;
    default:
      assert(0);
    }
  }
  return AnalyzeResult::Vectorize(true);
}

//--------------------------------------------------------- 
AnalyzeResult analyzeBody(ForStmt* Node, VectorizeInfo& info)
{
  VectorizeableExprsCollector collector(info);
  AnalyzeResult result = analyzeNode(Node->getBody(), collector);
  switch (result.m_Result)
  {
    case AnalyzeResult::VECTORIZE:
    {
      break;    
    }

    case AnalyzeResult::GUARDING_IF:
    {
      AnalyzeResult thenResult = analyzeNode(result.m_GuardingIf->getThen(), collector);
      if (thenResult.m_Result == AnalyzeResult::SPLIT_IF)
      {
        return thenResult;
      }
      if (thenResult.m_Result != AnalyzeResult::VECTORIZE)
      {
        // this may be: for () { if(c1) if(c2) {} } 
        // TODO: merge to for () { if(c1 && c2) {} }
        return AnalyzeResult::Vectorize(false);
      }
      if (result.m_GuardingIf->getElse() != 0)
      {
        AnalyzeResult elseResult = analyzeNode(result.m_GuardingIf->getElse(), collector);
        if (elseResult.m_Result == AnalyzeResult::SPLIT_IF)
        {
          return elseResult;
        }
        if (elseResult.m_Result != AnalyzeResult::VECTORIZE)
        {
          return AnalyzeResult::Vectorize(false);
        }
      }
      info.markAllIndexIndirect();
      break;
    }

    default:
      break;
  }
  return result;
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

