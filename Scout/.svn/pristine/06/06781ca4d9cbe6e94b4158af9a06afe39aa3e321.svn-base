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

#include "clang/Vectorizing/ValueFlowAnalysis.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/ASTProcessing/Toolbox.h"

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

class MyStoreManager : public StoreManager {

  typedef llvm::ImmutableMap<const MemRegion*,SVal> BindingsTy;
  BindingsTy::Factory VBFactory;

  static inline BindingsTy GetBindings(Store store) {
    return BindingsTy(static_cast<const BindingsTy::TreeTy*>(store));
  }

public:
  MyStoreManager(ProgramStateManager& mgr)
    : StoreManager(mgr), VBFactory(mgr.getAllocator()) {}

  ~MyStoreManager() {}

  virtual SVal getBinding(Store store, Loc loc, QualType T = QualType());
  virtual StoreRef Bind(Store store, Loc loc, SVal val);
  virtual StoreRef getInitialStore(const LocationContext *InitLoc)
  { return StoreRef(VBFactory.getEmptyMap().getRoot(), *this); }

  virtual void iterBindings(Store store, BindingsHandler& f) 
  { assert(0); }

  virtual StoreRef killBinding(Store store, Loc L)
  { assert(0); return StoreRef(store, *this); }

  virtual StoreRef bindCompoundLiteral(Store store,
                                    const CompoundLiteralExpr* cl,
                                    const LocationContext *LC, SVal v)
  { assert(0); return StoreRef(store, *this); }

  virtual SVal ArrayToPointer(Loc Array, QualType ElementTy)
  { assert(0); return UnknownVal(); }

  virtual StoreRef removeDeadBindings(Store store, const StackFrameContext *LCtx,
                                      SymbolReaper& SymReaper) 
  { assert(0); return StoreRef(store, *this); }

  virtual bool includedInBindings(Store store,
                                  const MemRegion *region) const
  { assert(0); return false; }

  virtual bool scanReachableSymbols(Store S, const MemRegion *R,
                                    ScanReachableSymbols &Visitor) 
  { assert(0); return false; }

  virtual StoreRef BindDecl(Store store, const VarRegion *VR, SVal initVal) 
  { assert(0); return StoreRef(store, *this); }

  virtual StoreRef BindDeclWithNoInit(Store store, const VarRegion *VR) 
  { assert(0); return StoreRef(store, *this); }

  virtual StoreRef invalidateRegions(Store store,
                                     ArrayRef<SVal> Values,
                                     const Expr *E, unsigned Count,
                                     const LocationContext *LCtx,
                                     const CallEvent *Call,
                                     InvalidatedSymbols &IS,
                                     RegionAndSymbolInvalidationTraits &ITraits,
                                     InvalidatedRegions *InvalidatedTopLevel,
                                     InvalidatedRegions *Invalidated)
  { assert(0); return StoreRef(store, *this); }

  virtual SVal evalDerivedToBase(SVal derived, QualType basePtrType) 
  { assert(0); return derived; }

  virtual SVal evalDynamicCast(SVal base, QualType derivedPtrType,
                                 bool &Failed)
  { assert(0); return base; }
  
  virtual void print(Store store, llvm::raw_ostream& Out,
                     const char* nl, const char *sep) 
  { assert(0); }
};

StoreManager *CreateMyStoreManager(ProgramStateManager& StMgr)
{
  return new MyStoreManager(StMgr);
}

//--------------------------------------------------------- 
SVal MyStoreManager::getBinding(Store store, Loc loc, QualType T)
{
  const MemRegion* R = loc.getAsRegion();
  if (R)
  {
    BindingsTy B = GetBindings(store);
    BindingsTy::data_type *Val = B.lookup(R);
    return Val ? *Val : UnknownVal();
  }
  assert(0);
  return UnknownVal();
}

//--------------------------------------------------------- 
StoreRef MyStoreManager::Bind(Store store, Loc loc, SVal V)
{
  const MemRegion* R = loc.getAsRegion();
  if (R)
  {
    BindingsTy B = GetBindings(store);
    return StoreRef(V.isUnknown()
      ? VBFactory.remove(B, R).getRoot()
      : VBFactory.add(B, R, V).getRoot(), *this);
  }
  assert(0);
  return StoreRef(store, *this);
}

//--------------------------------------------------------- 
struct MyConstraintManager : ConstraintManager
{
  virtual ProgramStateRef assume(ProgramStateRef state, DefinedSVal Cond,
                                bool Assumption) { return state; }

  virtual const llvm::APSInt* getSymVal(ProgramStateRef state,
                                        SymbolRef sym) const { return 0; }

  virtual bool isEqual(ProgramStateRef state, SymbolRef sym,
                       const llvm::APSInt& V) const { return false; }

  virtual ProgramStateRef removeDeadBindings(ProgramStateRef state,
                                            SymbolReaper& SymReaper) { return state; }

  virtual void print(ProgramStateRef state, llvm::raw_ostream& Out,
                     const char* nl, const char *sep) {}

  virtual bool canReasonAbout(SVal X) const { return false; }
};

//--------------------------------------------------------- 
ConstraintManager *CreateMyConstraintManager(ProgramStateManager&, SubEngine*)
{
  return new MyConstraintManager();
}

//--------------------------------------------------------- 
struct GetRValueSVal : StmtVisitor<GetRValueSVal, SVal>, ValueFlowContext
{
  GetRValueSVal(ValueFlowContext& vf) : ValueFlowContext(vf) {}

  SVal VisitUnaryAddrOf(UnaryOperator* Node)
  {
    return getLValueMemRegionAsSVal(Node->getSubExpr());
  }

  SVal VisitCallExpr(CallExpr* Node)
  {
    const FunctionDecl *FD = findFunctionDecl(Node);
    if (FD == NULL) // || findAttachedPragma(FD, "function", "dummy") == 0)
    {
      return VisitExpr(Node);
    }
    const SymExpr* result = getRValueSVal(Node->getCallee()).getAsSymbolicExpression();
    if (result == 0)
    {
      return VisitExpr(Node);
    }

    for (CallExpr::arg_iterator i = Node->arg_begin(), e = Node->arg_end(); i != e; ++i)
    {
      SVal argRes = getRValueSVal(*i);
      Optional<nonloc::ConcreteInt> ci = argRes.getAs<nonloc::ConcreteInt>();
      if (ci.hasValue())
      {
        result = m_SymbolManager().getSymIntExpr(result, BO_Comma,
          ci->getValue(), Node->getType());
      } 
      else
      {
        const SymExpr* argSym = argRes.getAsSymbolicExpression();
        if (argSym == 0)
        {
          return VisitExpr(Node);
        }
        result = m_SymbolManager().getSymSymExpr(result, BO_Comma,
                                                 argSym, Node->getType());
      }
    }
    return nonloc::SymbolVal(result);
  }

  SVal VisitBinaryOperator(BinaryOperator* Node)
  {
    QualType type = Node->getType();
    SVal lhsVal = getRValueSVal(Node->getLHS());
    SVal rhsVal = getRValueSVal(Node->getRHS());
    BinaryOperatorKind opc = Node->getOpcode();

    if (Loc::isLocType(type))
    {
      SVal result = getValueManager().evalBinOp(m_Collector.m_CurrentState, opc,
                                                lhsVal, rhsVal, type);
      return result.isUnknown() ? VisitExpr(Node) : result;
    }

    Optional<nonloc::ConcreteInt> lhsCI = lhsVal.getAs<nonloc::ConcreteInt>();
    Optional<nonloc::ConcreteInt> rhsCI = rhsVal.getAs<nonloc::ConcreteInt>();

    if (opc == BO_LAnd &&
        ((lhsCI && !lhsCI->getValue().getBoolValue()) ||
         (rhsCI && !rhsCI->getValue().getBoolValue())))
    {
      return getValueManager().makeTruthVal(false, Ctx().BoolTy);
    }
    if (opc == BO_LOr &&
        ((lhsCI && lhsCI->getValue().getBoolValue()) ||
         (rhsCI && rhsCI->getValue().getBoolValue())))
    {
      return getValueManager().makeTruthVal(true, Ctx().BoolTy);
    }

    if (rhsCI)
    {
      if (lhsCI)
      {
        SVal result = lhsCI->evalBinOp(getValueManager(), opc, rhsCI.getValue());
        if (!result.isUnknownOrUndef())
        {
          return result;
        }
      }
      else if (const SymExpr* lhsSym = lhsVal.getAsSymbolicExpression())
      {
        return nonloc::SymbolVal(m_SymbolManager().getSymIntExpr(lhsSym, opc, rhsCI->getValue(), type));
      }
    } 
    else if (const SymExpr* rhsSym = rhsVal.getAsSymbolicExpression())
    {
      if (const SymExpr* lhsSym = lhsVal.getAsSymbolicExpression())
      {
        return nonloc::SymbolVal(m_SymbolManager().getSymSymExpr(lhsSym, opc, rhsSym, type));
      }
      else if (lhsCI) 
      {
        switch (opc)
        {
        case BO_Mul: case BO_Add: case BO_EQ: case BO_NE:                 
        case BO_And: case BO_Xor: case BO_Or: case BO_LAnd: case BO_LOr:                       
          return nonloc::SymbolVal(m_SymbolManager().getSymIntExpr(rhsSym, opc, lhsCI->getValue(), type));

        default:
          break;
        }
      }
    }

    return VisitExpr(Node);
  }

  //SVal VisitUnaryOperator(UnaryOperator* Node)
  //{
  //  getRValueSVal(Node->getSubExpr());
  //  return VisitExpr(Node);
  //}


  SVal VisitExpr(Expr* Node)
  {
    llvm::APSInt idx;
    if (Node->isIntegerConstantExpr(idx, Ctx()))
    {
      idx.setIsSigned(true);
      return nonloc::ConcreteInt(m_ValueFactory().getValue(idx));
    }
    // introduce an unique symbol for an unsupported expression:
    QualType subTy = Node->getType();
    if (subTy->isArrayType() /*|| subTy->isFunctionType()*/)
    {
      // arrays as rvalues happens in int a[10]; int* p = a;
      // clangs analysis handles this cases in VisitCastExpr
      const MemRegion* memRegion = getLValueMemRegion(Node);
      return memRegion == 0 ? SVal(UnknownVal()) : SVal(loc::MemRegionVal(memRegion));
    }

    return getValueManager().conjureSymbolVal(0, Node, 0, Node->getType(), 0);
  }

  SVal VisitCastExpr(CastExpr* Node)
  {
    return getRValueSVal(Node->getSubExpr());
  }

  SVal VisitParenExpr(ParenExpr* Node)
  {
    return getRValueSVal(Node->getSubExpr());
  }
};


//--------------------------------------------------------- 
struct GetLValueMemRegion : StmtVisitor<GetLValueMemRegion, const MemRegion*>, ValueFlowContext
{
  GetLValueMemRegion(ValueFlowContext& vf) : ValueFlowContext(vf) {}


  const MemRegion* getDerefPtrRegion(Expr* Node)
  {
    SVal ptrValue = getRValueSVal(Node);
    return ptrValue.getAsRegion();
  }

  const MemRegion* VisitUnaryDeref(UnaryOperator* Node)
  {
    return getDerefPtrRegion(Node->getSubExpr());
  }

  const MemRegion* VisitMemberExpr(MemberExpr* Node)
  {
    FieldDecl* FD = dyn_cast<FieldDecl>(Node->getMemberDecl());
    const MemRegion* baseRegion = Node->isArrow() ?
      getDerefPtrRegion(Node->getBase()) : 
      Visit(Node->getBase());
    return (FD == 0 || baseRegion == 0) ? 0 : m_MRManager().getFieldRegion(FD, baseRegion);
  }

  const MemRegion* VisitDeclRefExpr(DeclRefExpr* Node)
  {
    Decl* D = Node->getDecl();
    if (VarDecl* VD = dyn_cast<VarDecl>(D))
    {
      return m_MRManager().getVarRegion(VD, getLC());
    }
    if (FunctionDecl *FD = dyn_cast<FunctionDecl>(D))
    {
      return m_MRManager().getFunctionTextRegion(FD);
    }
    return 0;
  }

  const MemRegion* VisitArraySubscriptExpr(ArraySubscriptExpr* Node)
  {
    const MemRegion* baseRegion = getDerefPtrRegion(Node->getBase()); 
    if (baseRegion == 0)
    {
      return 0;
    }

    SVal symbolIdx = getRValueSVal(Node->getIdx());
    //TODO: fix the cast
    return m_MRManager().getElementRegion(Node->getType().getUnqualifiedType(), symbolIdx.castAs<NonLoc>(), baseRegion, Ctx());
  }

  const MemRegion* VisitCastExpr(CastExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  const MemRegion* VisitParenExpr(ParenExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  const MemRegion* VisitStmt(Stmt* Node)
  {
    return 0;
  }
};


//--------------------------------------------------------- 
} // anon namespace 


//--------------------------------------------------------- 
ValueFlowCollector::ValueFlowCollector(ASTContext &Ctx, FunctionDecl* D) :
  m_StateManager(Ctx, CreateMyStoreManager, CreateMyConstraintManager, m_Alloc, 0),
  m_AnalysisCtx(0, D)  
{
  m_CurrentState = m_StateManager.getInitialState(getLC());
}

//--------------------------------------------------------- 
const LocationContext* ValueFlowCollector::getLC()
{
  return m_LCManager.getStackFrame(&m_AnalysisCtx, 0, 0, 0, 0);
}

//--------------------------------------------------------- 
const MemRegion* ValueFlowContext::getLValueMemRegion(Expr* Node)
{
  return GetLValueMemRegion(*this).Visit(Node);
}

//--------------------------------------------------------- 
SVal ValueFlowContext::getLValueMemRegionAsSVal(Expr* Node)
{
  const MemRegion* memRegion = getLValueMemRegion(Node);
  if (memRegion == 0)
  {
    // FIXME in GetLValueMemRegion: introduce symbolic region
    assert(0);
    return UnknownVal();
  }
  else
  {
    return loc::MemRegionVal(memRegion);
  }
}

//--------------------------------------------------------- 
SVal ValueFlowContext::getRValueSVal(Expr* Node)
{
  const MemRegion* memRegion = getLValueMemRegion(Node);
  if (memRegion == 0)
  {
    // this is a truly (unnamed) rvalue which never get bound to anything:
    return GetRValueSVal(*this).Visit(Node);
  }
  else
  {
    SVal val = getSVal(memRegion);
    if (!val.isUnknown())
    {
      return val;
    }
    SVal result = GetRValueSVal(*this).Visit(Node);
    m_Collector.m_CurrentState = m_Collector.m_CurrentState->bindLoc(loc::MemRegionVal(memRegion), result);
    return result;
  }
}

//--------------------------------------------------------- 
const LocationContext* ValueFlowContext::getLC()
{
  return m_Collector.getLC();
}

//--------------------------------------------------------- 
void ValueFlowContext::bindValue(const MemRegion* memId, SVal value)
{
  m_Collector.m_CurrentState = m_Collector.m_CurrentState->bindLoc(loc::MemRegionVal(memId), value);
}

//--------------------------------------------------------- 
const VarRegion* ValueFlowContext::getMemRegion(VarDecl* VD)
{
  return m_MRManager().getVarRegion(VD, getLC());
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

