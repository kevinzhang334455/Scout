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

#ifndef SCOUT_CLANGADDONS_VECTORIZING_VALUEFLOWANALYSIS_H
#define SCOUT_CLANGADDONS_VECTORIZING_VALUEFLOWANALYSIS_H

#include "clang/StaticAnalyzer/Core/PathSensitive/SVals.h"    
#include "clang/StaticAnalyzer/Core/PathSensitive/MemRegion.h"    
#include "clang/StaticAnalyzer/Core/PathSensitive/AnalysisManager.h"    
#include "clang/StaticAnalyzer/Core/PathSensitive/ProgramState.h"    
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"    
#include "clang/AST/StmtGraphTraits.h"

//--------------------------------------------------------- 
namespace clang {

using namespace ento;

namespace ASTProcessing {

//--------------------------------------------------------- 
struct ValueFlowCollector
{
  llvm::BumpPtrAllocator  m_Alloc;
  LocationContextManager  m_LCManager;
  AnalysisDeclContext     m_AnalysisCtx;
  ProgramStateManager     m_StateManager;
  ProgramStateRef         m_CurrentState;

  ValueFlowCollector(ASTContext &ctx, FunctionDecl* D);
  const LocationContext* getLC();
private:
  ValueFlowCollector(const ValueFlowCollector&);
};

//--------------------------------------------------------- 
struct ValueFlowContext
{
  ValueFlowCollector& m_Collector;
  explicit ValueFlowContext(ValueFlowCollector& c) : m_Collector(c) {}

  const MemRegion* getLValueMemRegion(Expr* Node);
  SVal getLValueMemRegionAsSVal(Expr* Node);
  SVal getRValueSVal(Expr* Node);
  void bindValue(const MemRegion* memId, SVal value);
  const LocationContext* getLC();
  const VarRegion* getMemRegion(VarDecl* VD);

  SVal getSVal(const MemRegion* memId) { return m_Collector.m_CurrentState->getSVal(memId); }
  MemRegionManager& m_MRManager() { return m_Collector.m_StateManager.getRegionManager(); }
  ASTContext& Ctx() { return m_Collector.m_StateManager.getContext(); }
  SymbolManager& m_SymbolManager() { return m_Collector.m_StateManager.getSymbolManager(); }
  SValBuilder& getValueManager() { return m_Collector.m_StateManager.getSValBuilder(); }
  BasicValueFactory& m_ValueFactory() { return getValueManager().getBasicValueFactory(); }
};


//--------------------------------------------------------- 
// lvalue_iterator traverses over all referenced MemRegions in a expr
class MemRegion_iterator : private ValueFlowContext
{
  typedef llvm::df_iterator<Stmt*> IteratorTy;
  llvm::df_iterator<Stmt*>  m_Iterator;
  const MemRegion*          m_CurrentRegion;

  void toNext()
  {
    const MemRegion* nextRegion = 0;
    while (m_Iterator != IteratorTy::end(0) && 
           ((!isa<Expr>(*m_Iterator)) ||
            (nextRegion = getLValueMemRegion(cast<Expr>(*m_Iterator))) == 0 || 
            //m_CurrentRegion == nextRegion ||
            isa<CodeTextRegion>(nextRegion)))
    {
      ++m_Iterator;
    } 
    m_CurrentRegion = nextRegion;
  }

public:
  typedef Expr* pointer;
  typedef MemRegion_iterator _Self;

  explicit MemRegion_iterator(Stmt* start, ValueFlowContext& ctx) : 
    ValueFlowContext(ctx),
    m_Iterator(llvm::df_iterator<Stmt*>::begin(start)),
    m_CurrentRegion(0) {
    toNext();
  }

  MemRegion_iterator(ValueFlowContext& ctx) :     
    ValueFlowContext(ctx),
    m_Iterator(IteratorTy::end(0)),
    m_CurrentRegion(0) {}

  inline bool operator==(const _Self& x) const {
    return m_Iterator == x.m_Iterator;
  }

  inline bool operator!=(const _Self& x) const { return !operator==(x); }

  inline const MemRegion* region() const { 
    return m_CurrentRegion;
  }

  inline pointer operator*() const {
    return cast<Expr>(*m_Iterator);
  }

  inline pointer operator->() const { return operator*(); }

  inline _Self& operator++() {   // Preincrement
    ++m_Iterator;
    toNext();
    return *this;
  }

  inline _Self& skipChildren() { 
    m_Iterator.skipChildren();
    toNext();
    return *this;
  }
};



//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_VECTORIZING_VALUEFLOWANALYSIS_H
