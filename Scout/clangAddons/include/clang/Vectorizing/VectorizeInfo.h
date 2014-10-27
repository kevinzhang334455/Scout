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

#ifndef SCOUT_CLANGADDONS_VECTORIZING_VECTORIZEINFO_H
#define SCOUT_CLANGADDONS_VECTORIZING_VECTORIZEINFO_H

#include "clang/Vectorizing/IntrinsicEditor.h"    
#include "clang/Vectorizing/ValueFlowAnalysis.h"    
//#include "clang/AST/APValue.h"    
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include <list>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/bimap/bimap.hpp>
#include <map>

//--------------------------------------------------------- 
namespace clang {

namespace ento { class MemRegion; }

//--------------------------------------------------------- 
namespace ASTProcessing {

// forwards
class DeclCollector;
class TargetStmt;

//--------------------------------------------------------- 
enum tExprResult { 
  /*must be first:*/CONSTANT, // expression or variable is considered constant in the loop
  NOT_ANALYZED,               // expression or variable is unknown in the loop
  STRIDE1,                    // expression or variable is stride1 and has integral type 
  STRIDE1_DEPENDENT           // expression or variable is dependent on stride1 (e.g. an array access) -> these expressions get vectorized
};

// some notes regarding tExprResult:
// the tExprResult of an rvalue always denotes the value yielded by that expression
// an lvalue (that is, a MemRegion) has two kinds of tExprResult: the MemRegion itself and the content of the MemRegion 
// (e.g. a variable has a constant MemRegion, but its content may be of different kind).
// only constant MemRegions can hold all kinds of content, all other regions always hold STRIDE1_DEPENDENT values
/* example:

float a[100];
int b[100];
struct S x;  // S has members
float y;
int j;
for (int i = 0; i < 100; ++i)  // i is stride1
{
  j = i + 1;   // MemRegion of j CONSTANT, content STRIDE1
  y = a[i];    // MemRegion of a[i] STRIDE1 -> content STRIDE1_DEPENDENT
  a[b[j]] = y; // MemRegion of b[i] STRIDE1 -> content STRIDE1_DEPENDENT 
                  -> MemRegion and content of a[b[j]] STRIDE1_DEPENDENT
}
*/

//--------------------------------------------------------- 
struct StridedExprResult
{
  tExprResult m_Result;
  Expr*       m_StrideExpr;
  bool        m_bInnerLoopVariant;
  bool operator==(tExprResult vglRes) const { return m_Result == vglRes; }
  bool operator!=(tExprResult vglRes) const { return !operator==(vglRes); }
  StridedExprResult(tExprResult r) : m_Result(r), m_StrideExpr(0), m_bInnerLoopVariant(0) {}
  StridedExprResult() : m_Result(NOT_ANALYZED), m_StrideExpr(0), m_bInnerLoopVariant(0) {}
  bool isAbsolutConstant() const { return m_Result == CONSTANT && !m_bInnerLoopVariant; }
  bool isRealStride1() const { return m_Result == STRIDE1 && m_StrideExpr == 0; }
  static StridedExprResult Stride1(Expr* E)
  {
    StridedExprResult exprResult(STRIDE1);
    exprResult.m_StrideExpr = E;
    return exprResult;
  }

  static StridedExprResult InnerLoopVariant()
  {
    StridedExprResult exprResult(CONSTANT);
    exprResult.m_bInnerLoopVariant = true;
    return exprResult;
  }

  static StridedExprResult LHStoRHS(const StridedExprResult& lhsRes)
  {
    StridedExprResult exprResult(lhsRes);
    if (exprResult.m_Result == STRIDE1)
    {
      exprResult.m_Result = STRIDE1_DEPENDENT; 
    }
    return exprResult;
  }
};

//--------------------------------------------------------- 
enum tExprAction { 
  NO_CHANGE,      // expression don't change (e.g. paren expr, assignments)
  ROLLOUT,        // expression can't be vectorized and thus is rolled out
  VECTORIZE       // expression is vectorized 
}; 


//--------------------------------------------------------- 
class TopLevelStmt
{
private:
  enum kind { eVectorizeAssign, eUnrollAssign, eUnrollStmt };

  TopLevelStmt(Stmt* S) : m_Kind(eUnrollStmt), m_UnrollStmt(S) {}
  TopLevelStmt(BinaryOperator* BO, kind k) : m_Kind(k), m_Assign(BO) {}

  kind m_Kind;
  SimdType  m_VectorType;
  union {
    BinaryOperator* m_Assign;
    Stmt*           m_UnrollStmt;
  };
  
public:
  static TopLevelStmt vectorize(BinaryOperator* BO) 
  {
    return TopLevelStmt(BO, eVectorizeAssign);
  }

  static TopLevelStmt forceUnroll(BinaryOperator* BO) 
  {
    return TopLevelStmt(BO, eUnrollAssign);
  }
  
  static TopLevelStmt unroll(Stmt* S) 
  {
    return TopLevelStmt(S);
  }

  BinaryOperator* getAssign() const { return m_Kind == eUnrollStmt ? 0 : m_Assign; } 
  Stmt* getUnrollStmt() const { return m_Kind == eUnrollStmt ? m_UnrollStmt : 0; } 
  bool isForcedUnroll() const { return m_Kind == eUnrollAssign; }
  void setVectorType(const SimdType& vt) { m_VectorType = vt; }
  const SimdType& getVectorType() const { return m_VectorType; }
};

//--------------------------------------------------------- 
struct VectorizeInfo : IntrinsicEditor
{
  ValueFlowCollector  m_ValueFlowCollector;
  ValueFlowContext    m_ValueFlowContext;

  // this could be things like a[i] but also *(a+i):
  std::list<Expr*>  m_AlignedArraySubscripts;
  unsigned m_VectorSize;
  unsigned m_VectorByteAlignment;

  // contains all top level loop statements to be vectorized:
  typedef llvm::SmallVector<TopLevelStmt, 8> tVectExpressions;
  tVectExpressions m_AllExprs;

  // contains all generated loop invariant expressions of vectorized variables
  // to be inserted before the loop:
  struct tInvariantAssign
  {
    std::string   m_ExprAsString;
    VarDecl*      m_TempVar;
    SimdType      m_Type;
    mutable Expr* m_RHSExpr;
    tInvariantAssign() : m_TempVar(0), m_RHSExpr(0){}
  };

  typedef boost::multi_index::multi_index_container<tInvariantAssign,
    boost::multi_index::indexed_by<
      boost::multi_index::ordered_non_unique<boost::multi_index::member<tInvariantAssign,std::string,&tInvariantAssign::m_ExprAsString> >,
      boost::multi_index::ordered_unique<boost::multi_index::member<tInvariantAssign,VarDecl*,&tInvariantAssign::m_TempVar> > 
    > > tLoopInvariantAssigns;

  tLoopInvariantAssigns m_LoopInvariantAssigns;

  // gather/scatter support
  enum tGSDistanceInitKind { SPLATTED, UNIFORM };

  typedef std::pair<SimdType, std::string> tTypedDistance;

  struct tGSIndexAssign
  {
    DeclStmt*     m_TempScalarVar;
    DeclStmt*     m_TempIndexVar; // maybe 0 if get_uniform_gs_index does not exist
    Expr*         m_IndexExpr;
  };

  typedef std::map<tTypedDistance, tGSIndexAssign> tGSIndexAssigns;
  tGSIndexAssigns m_LocalGSIndexAssigns;


  struct tTypedIndex
  {
    const ValueDecl*  m_TempVar;
    QualType          m_RegionType;
    
    tTypedIndex(const ValueDecl* VD, const QualType& RT) :
      m_TempVar(VD), m_RegionType(RT) {}
  
    bool operator<(const tTypedIndex& other) const
    {
      return m_TempVar < other.m_TempVar || 
             (m_TempVar == other.m_TempVar && m_RegionType.getTypePtr() < other.m_RegionType.getTypePtr());
    }
  };

  typedef std::map<tTypedIndex, DeclStmt*> tGSIndexVars;
  tGSIndexVars m_GSIndexVars;

  typedef std::map<std::pair<Expr*, unsigned>, DeclRefExpr*> tGSIndexExprs;
  tGSIndexExprs m_GSIndexExprs;



  // end of gather/scatter support

  // contains all top level loop scalar invariant expressions 
  // to be inserted before the loop and either removed or aliased in the body:
  llvm::SmallVector<Stmt*, 8> m_ScalarLoopInvariantExprs;

  // all introduced vectorized variables:
  std::list<DeclStmt*>  m_TempVars;

  // all introduced other variables:
  std::list<DeclStmt*>  m_NonVectorizedTempVars;

  // vectorized constants
  struct tSimdConst
  {
    SimdType      m_Type;
    APValue       m_Value;
  
    tSimdConst(const SimdType& t, const APValue& v) :
      m_Type(t), m_Value(v) {}
  
    bool operator<(const tSimdConst& other) const
    {
      return m_Type < other.m_Type || 
             (m_Type == other.m_Type && isValueLess(other.m_Value));
    }

    bool isValueLess(const APValue& other) const;
  };


  typedef boost::bimaps::bimap<tSimdConst, VarDecl*> tTypedConstAssigns;

  struct tFunctionGlobalAssigns : IntrinsicEditor
  {
    tFunctionGlobalAssigns(IntrinsicEditor& e) : IntrinsicEditor(e) {}
    void generateFunctionGlobalAssigns(DeclCollector& collector,
      llvm::SmallVector<Stmt*, 32>& topLevelStmts);

    tTypedConstAssigns m_ConstAssigns;
    tGSIndexAssigns    m_GSUniformIndexAssigns, m_GSSplattedIndexAssigns;

  };

  tFunctionGlobalAssigns& m_GlobalAssigns;

  // an optimization for expanding temporaries in the condition 
  // of the loop alignment addition:
  llvm::DenseMap<const MemRegion*, Expr*>  m_LocalVarWrites;

  //--------------------------------------------------------- 
  struct tStatistics
  {
    unsigned  numLoadOps;       // scout_load_(un)aligned, SSE: _mm_load(u)_ps
    unsigned  numStoreOps;      // scout_store_(un)aligned, SSE: _mm_store(u)_ps
    unsigned  numPackedOps;     // scout_add/sub/mul/div/neg/sqrt/abs
    tStatistics();
  };
  mutable tStatistics m_Statistics;

  //--------------------------------------------------------- 
  VectorizeInfo(IntrinsicEditor& editor, tFunctionGlobalAssigns& globalAssigns);
  bool analyzeForStmt(ForStmt* Node, const char*& errMsg, bool bInnerLoop);

  // call after m_Stride1ArrayAccesses is populated
  // returns true, if all remaining vectorizing infos could be initialized
  bool postInit(unsigned uForcedVectorSize, bool bIncludeGSTypes = false);
  void initExplicitAlignment(Expr* alignExpr);
  void initMostDirectArrayAccess();

  const SimdType& getTargetType(BinaryOperator* BO) const;

  bool isAligned(Expr* D) const;
  bool isVectorizedType(const QualType& Type) const;
  bool isNonTemporal(Expr* D);
  void initExplicitNonTemporal(Expr* alignExpr);

  const MemRegion* getVarDecl(Expr* Node);

  Expr* insertLoopInvariantVar(Expr* Node, const SimdType& targeType);
  void markLoopInvariantVarsUsed(const std::list<Stmt*>& exprs);

  // creates a scout_extract call unless a loop invariant variable or 
  // constant gets extracted, which is directly referred
  Expr* createScalarExtract(DeclRefExpr* varRef, const SimdType& sourceType, unsigned idx);

  // marks a possible current live region with id memRegion as dead and
  // then creates a new live region for memRegion tagged with LHS:
  bool createLiveMemRegion(const MemRegion* memRegion, StridedExprResult lhsResult, Expr* LHS, StridedExprResult result, llvm::PointerIntPair<Stmt*, 1> branchStmt = (llvm::PointerIntPair<Stmt*, 1>()));
  void createLiveMemRegion(const VarRegion* memRegion, VarDecl* VD, StridedExprResult result, llvm::PointerIntPair<Stmt*, 1> branchStmt);

  // SSA support:
  //void startBranch();
  //void nextBranch();
  //void mergeAllBranches();

  // returns the mem region result for the current live region which is referred
  // by any expression E
  // if there is no mem region result it iscreated and initialized with CONSTANT
  StridedExprResult getLiveMemRegionResult(const MemRegion* memRegion, StridedExprResult lhsRes, Expr* RHS);

  // targetAlias can be 0:
  DeclRefExpr* getVectorizedVarLHS(const Expr* Node, int index, DeclRefExpr* aliasVar);  

  struct tUnrolledLHSResult
  {
    Expr*         m_Assign;    
    Expr*         m_TempVarExtract;
  };

  tUnrolledLHSResult getVectorizedVarForUnrollLHS(const Expr* Node, Expr* targetRHS, Stmt* unrolledStmt, unsigned index);
  Expr* getVectorizedVarRHS(Expr* Node, TargetStmt& target);
  Expr* getBoundVectorizedVar(Expr* Node, int index);

  DeclStmt* getVectTempVar(const SimdType& targetType);

  void setAnalyzedExprResult(Expr* E, StridedExprResult result, bool expectExistence = false);  
  StridedExprResult getAnalyzedExprResult(Expr* E) const;
  StridedExprResult getLValueResult(Expr* Node) const;

  void markAllIndexIndirect();

  bool isLive(Expr* LHS);

  bool isLoadShuffleable(Expr* originalLoadRHS);
  void saveLoadForShuffle(CallExpr* vectAssign);
  void shuffleLoads();

  Expr* getConstVar(const APValue& value, const SimdType& simdType);


  bool isStreamable(Expr* Node, const SimdType& targetType) const;
  Expr* retrieveGatherScatterArgs(Expr* Node, Expr*& indexExpr, TargetStmt& target);  

  Expr* getSimplifiedScalar(Expr* Node, BuiltinType::Kind scalarType);
  DeclStmt* getScalarLoopInvariantVar(Expr* initializer, QualType type);
  const StridedExprResult& getSimplifiedStride1Expr(Expr* E);

  void moveConstAssigns();
  void generateLoopEnclosingStmts(DeclCollector& collector,
                                  llvm::SmallVector<Stmt*, 32>& preStmts, 
                                  llvm::SmallVector<Stmt*, 32>& postStmts);
private:
  VectorizeInfo(const VectorizeInfo&);  // don't implement

  Expr* m_NonTemporalExpr;

  typedef llvm::DenseMap<const MemRegion*, std::list<Expr*> > tArrayAccesses;
  tArrayAccesses m_Stride1ArrayAccesses;

  typedef std::map<BuiltinType::Kind, SimdType> tSimdTypes;
  tSimdTypes  m_SimdTypes;

  std::map<Expr*, APValue>  m_EvaluableExpressions;

  // the results of all analyzed expressions:
  typedef llvm::DenseMap<Expr*, StridedExprResult> tExprResults;
  tExprResults m_AnalyzedExprs;

  Expr* insertLoopInvariantVar(Expr* Node, const std::string& nodeStr, const SimdType& targeType);
  Expr* evaluateConst(Expr* Node, const SimdType& targetType);

  bool mergeVectorSize(SimdType& result, const SimdType& simdType);
  SimdType initVectorSize(bool bIncludeGSTypes);

  std::map<std::string, DeclStmt*> m_ScalarLoopInvariantVarInitializer;


  struct tMemRegionUsage
  {
    typedef llvm::PointerUnion<const Expr*, const VarDecl*> tWrite;
    StridedExprResult       m_Result;
    tWrite                  m_Write;
    std::list<const Expr*>  m_ReadExprs;
    llvm::PointerIntPair<Stmt*, 1>  m_BranchStmt;

    template<class T>
    explicit tMemRegionUsage(const T* t, StridedExprResult r, llvm::PointerIntPair<Stmt*, 1> branchStmt) :
      m_Result(r), m_Write(t), m_BranchStmt(branchStmt) {}

    explicit tMemRegionUsage(StridedExprResult rhsResult) :
      m_Result(rhsResult) {}
  };  
  typedef std::list<tMemRegionUsage> tUseSequence;


  struct tSsaMemRegion
  {
    tUseSequence        m_UseSequence;
    QualType            m_ScalarType;   
    StridedExprResult   m_LHSResult;
    SimdType            m_BoundType;  // valid, if !m_BoundTempVar.empty()
    llvm::SmallVector<VarDecl*, 4>  m_BoundTempVar;
    DeclStmt*           m_BoundTempArray;
    Expr*               m_ReductionInitElement;
    std::list<Expr*>    m_ReductionSecondaryExprs;
    explicit tSsaMemRegion(QualType scalarType, const StridedExprResult& lhsResult) :
      m_ScalarType(scalarType), m_LHSResult(lhsResult), m_BoundTempArray(0),  
      m_ReductionInitElement(0)
    {}
  };  


  typedef std::map<const MemRegion*, tSsaMemRegion> tVariableResults;
  typedef std::pair<tVariableResults::value_type*, tUseSequence::const_iterator> tSsaInfo;
  typedef llvm::DenseMap<const Expr*, tSsaInfo> tExprToSsaMap;

  tVariableResults  m_AnalyzedVars;
  tExprToSsaMap m_ExprToSsaMap;
 
  //tVariableResults  m_BranchedVarsStart;

  bool isLiveMemRegion(const tSsaInfo& memRegionInfo) const;
  bool isSubRegionOfAlignExpr(const MemRegion* memRegion, Expr* E);

  // returns true, if any expression referring to memRegion is not a child of S:
  bool isRegionUsedOutside(const tSsaMemRegion& memRegion, Stmt* S) const;

  // true, if the write denoted by info must be written to the temporary:
  bool needsWrite(const tSsaInfo& info) const;

  void initTempSimdVar(tSsaMemRegion& memRegion);
  Expr* createScalarToBoundVar(const tSsaMemRegion& memRegion, unsigned index);
  Expr* createScalarAssignToBoundVar(const tSsaMemRegion& memRegion, Expr* RHS, unsigned index);
  const SimdType* getSimdType(const tSsaMemRegion& memRegion);
  QualType getTypeForRegion(Expr* Node);
  QualType getTypeForRegion(const MemRegion* memRegion, Expr* Node);

  const tSsaInfo& getSsaMemRegion(const Expr* E) const;

  bool analyzeMemAccessPatterns();
  BinaryOperatorKind getReductionOp(const Expr* LHS, const Expr* RHS);
  bool markPreconditionWritesDependent(tUseSequence& useSequence, const MemRegion* memId);
  void markExprsDependent(Expr* E);
  void warnDangerousWrites(const tUseSequence& useSequence);


  Expr* createExprToBoundVar(const tSsaMemRegion& memRegion, TargetStmt& target);
  Expr* createConvert(const tSsaMemRegion& memRegion, TargetStmt& target);

  DeclStmt* getTempVarDecl(QualType T, const tMemRegionUsage::tWrite& Node);
  void createTempVar(tSsaMemRegion& memRegion);
  void createTempSimdVar(tSsaMemRegion& memRegion);
  void createSplattedTempSimdVar(tSsaMemRegion& memRegion);
  tSsaMemRegion& ensureSimdVar(const Expr* LHS);
  bool isTempLoadNeeded(tSsaMemRegion& rhsRegion) const;

  std::string getOriginalName(const Expr* E);

  bool analyzeForInc(Expr* Node, llvm::DenseSet<const MemRegion*>& incVars, 
                     const char*& errMsg, bool bInnerLoop);
  void generateReductionPrePost(const tSsaMemRegion& reductionRegion,
                                llvm::SmallVector<Stmt*, 32>& preStmts, 
                                llvm::SmallVector<Stmt*, 32>& postStmts);
  Expr* createReductionRHS(Expr* origRhs, Expr* extractExpr, const std::list<Expr*>& secondaryExprs);

  // reduction stuff:
  struct tReduction
  {
    enum tUnit { eNone, eZero, eOne, eSubArg1 };

    const char* m_Expression;
    tUnit       m_UnitElement;
  };

  static const tReduction s_AllReductionOps[]; 
  const BinaryOperator* testReductionReads(const Expr* writeExpr, const std::list<const Expr*>& readExprs);
  Expr* getReductionInit(const BinaryOperator* BO, std::list<Expr*>& secondaryExprs);
  // end of reduction stuff


  // gather/scatter stuff:
  struct GSDistanceResult 
  { 
    Expr *m_IndexExpr, *m_AddressExpr;
    GSDistanceResult() : m_IndexExpr(0), m_AddressExpr(0) {}
  };

  Expr* getDistanceExpr(Expr* Node, const QualType& regionType, const SimdType& targetType);
  Expr* getDistanceExpr(Expr* stride, Expr* lValueExpr, const SimdType& targetType);
  GSDistanceResult visitDistanceExpr(Expr* Node, TargetStmt& target);
  Expr* getGSIndex(Expr* E, const SimdType& targetType, 
                   tGSIndexAssigns& gsIndexAssigns, tGSDistanceInitKind k);
  Expr* getIndirectDistanceExpr(Expr* indexExpr, const QualType& regionType, TargetStmt& target);
  Expr* getSimplifiedIndexedLoc(Expr* Node, const SimdType& targetType, Expr* strideExpr, unsigned index);

  static void generateGSAssigns(const tGSIndexAssigns& gsAssigns, 
    llvm::SmallVector<Stmt*, 32>& stmts, DeclCollector& collector, IntrinsicEditor& e, tGSDistanceInitKind k);
  // end of gather/scatter stuff
};



//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_VECTORIZING_VECTORIZEINFO_H
