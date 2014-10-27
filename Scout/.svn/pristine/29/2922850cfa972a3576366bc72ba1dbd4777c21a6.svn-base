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

#include "clang/Vectorizing/TargetStmt.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    

#include <list>

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

//--------------------------------------------------------- 
TargetStmt::TargetStmt(VectorizeInfo& info, const SimdType& targetType) :
  IntrinsicEditor(info),
  m_Info(info),
  m_TargetType(targetType),
  m_Index(0)
{
  m_bGatherAvail = hasIntrinsicBuiltin("gather", m_TargetType);
  m_bScatterAvail = hasIntrinsicBuiltin("scatter", m_TargetType);

  m_StoreFunctions[0][0] = "store_unaligned";
  m_StoreFunctions[1][0] = testFnName("store_aligned", m_StoreFunctions[0][0]);
  m_StoreFunctions[0][1] = testFnName("store_nt_packed_unaligned", m_StoreFunctions[0][0]);
  m_StoreFunctions[1][1] = testFnName("store_nt_packed_aligned", 
    // if store_nt_packed_aligned doesn't exist, store_nt_packed_unaligned
    // is preferred over store_aligned, if available:
    m_StoreFunctions[0][1] != m_StoreFunctions[0][0] ? m_StoreFunctions[0][1] : 
                                                       m_StoreFunctions[1][0]);
}

//--------------------------------------------------------- 
const char* TargetStmt::testFnName(const char* fnName, const char* fnFallback) const
{
  return hasIntrinsicBuiltin(fnName, m_TargetType) ? fnName : fnFallback;
}

//--------------------------------------------------------- 
Expr* TargetStmt::retrieveLoadStoreArg(Expr* Node)
{
  if (!m_Info.isStreamable(Node, m_TargetType))
  {
    return 0;
  }
  Expr* argNode = getOnlySimplifiedLoc(Node);
  if (argNode == 0)
  {
    // this is possible, if e.g. it is a non-promotion cast (int <-> float)
    // in that case don't load streamed 
    return 0;
  }
  if (m_Index > 0)
  {
    argNode = Add_(argNode, Int_(m_Index * m_TargetType.getVectorSize()));
  }
  return argNode;
}

//--------------------------------------------------------- 
Expr* TargetStmt::emitLoadIntrinsic(Expr* RHS)
{
  Expr* argNode = retrieveLoadStoreArg(RHS);
  if (argNode != 0)
  {
    const char* fnName = m_Info.isAligned(RHS) ? 
      "load_aligned" : "load_unaligned";
    Expr* intrinsicArgs[1] = { argNode };
    argNode = IntrinsicCall_(fnName, intrinsicArgs, m_TargetType);
    m_Info.m_Statistics.numLoadOps++;
  }
  else if (m_bGatherAvail)
  {
    Expr* indexExpr;
    argNode = m_Info.retrieveGatherScatterArgs(RHS, indexExpr, *this);
    if (argNode != 0)
    {
      Expr* intrinsicArgs[2] = { argNode, indexExpr };
      argNode = IntrinsicCall_("gather", intrinsicArgs, m_TargetType);
    }
  }
  return argNode;
}

//--------------------------------------------------------- 
Expr* TargetStmt::emitStoreIntrinsic(Expr* LHS, DeclRefExpr* targetRHS)
{
  LHS = stripParenCasts(LHS);
  Expr* argNode = retrieveLoadStoreArg(LHS);
  if (argNode != 0)
  {
    int alignIdx = m_Info.isAligned(LHS) ? 1 : 0;
    int ntIdx = m_Info.isNonTemporal(LHS) ? 1 : 0;
    Expr* intrinsicArgs[2] = { argNode, targetRHS };
    argNode = IntrinsicCall_(m_StoreFunctions[alignIdx][ntIdx], intrinsicArgs, m_TargetType);
    m_Info.m_Statistics.numStoreOps++;
  }
  else if (m_bScatterAvail)
  {
    Expr* indexExpr;
    argNode = m_Info.retrieveGatherScatterArgs(LHS, indexExpr, *this);
    if (argNode != 0)
    {
      Expr* intrinsicArgs[3] = { argNode, indexExpr, targetRHS };
      argNode = IntrinsicCall_("scatter", intrinsicArgs, m_TargetType);
    }
  }
  return argNode;
}

//--------------------------------------------------------- 
Expr* TargetStmt::generateLoad(Expr* RHS, unsigned index)
{
  m_Index = index;
  RHS = stripParenCasts(RHS);
  Expr* result = emitLoadIntrinsic(RHS);
  return result == 0 ? unrollComplexRHS(RHS) : result;
}

//--------------------------------------------------------- 
DeclRefExpr* TargetStmt::getVar(Expr* RHS, unsigned index)
{
  m_Index = index;
  return splitEnsureVar(RHS);
}

//--------------------------------------------------------- 
std::list<Stmt*>& TargetStmt::generate(BinaryOperator* sourceNode, unsigned index)
{
  m_Index = index;
  Expr* sourceLHS = sourceNode->getLHS();
  Expr* targetStmt = split_without_tempvar(sourceNode->getRHS());
  DeclRefExpr* rhsRef = dyn_cast<DeclRefExpr>(targetStmt);
  DeclRefExpr* lhsRef = m_Info.getVectorizedVarLHS(sourceLHS, m_Index, rhsRef);
  if (rhsRef == 0 || lhsRef->getDecl() != rhsRef->getDecl())
  {
    m_GeneratedStmts.push_back(Assign_(lhsRef, targetStmt));
  }
  if (m_Info.isLive(sourceLHS))
  {
    emitLHS(sourceLHS, lhsRef);
  }
  return m_GeneratedStmts;
}

//--------------------------------------------------------- 
DeclRefExpr* TargetStmt::splitEnsureVar(Expr* Node)
{
  return ensureVar(split_without_tempvar(Node));
}

//--------------------------------------------------------- 
DeclRefExpr* TargetStmt::ensureVar(Expr* targetStmt)
{
  if (isa<DeclRefExpr>(targetStmt))
  {
    return cast<DeclRefExpr>(targetStmt);
  }

  DeclStmt* vectVarDecl = m_Info.getVectTempVar(m_TargetType);
  m_GeneratedStmts.push_back(Assign_(DeclRef_(vectVarDecl), targetStmt));
  return DeclRef_(vectVarDecl);
}


//--------------------------------------------------------- 
void TargetStmt::getSplitSubExpr(Expr* Node, tUnrolledExprs& targetExprs)
{
  targetExprs.resize(m_TargetType.getVectorSize());
  switch (m_Info.getAnalyzedExprResult(Node).m_Result)  
  {
    case CONSTANT:
    {
      Expr* E = m_Info.getSimplifiedScalar(Node, m_TargetType.getBuiltinType());
      for (unsigned i = 0; i < m_TargetType.getVectorSize(); ++i)
      {
        targetExprs[i] = i == 0 ? E : Clone_(E);
      }
      break;
    }
    case STRIDE1:
      TargetScalarStmt(*this).rollout(Node, targetExprs);
      break;
    case STRIDE1_DEPENDENT:
    // FIXME: we can do a better and simplified handling by testing type 
    // conversions and then rollout and split sub expressions again, if the
    // result týpe of Node does not match the target type scalar
    {
      DeclRefExpr* vectVar = splitEnsureVar(Node);
      for (unsigned i = 0; i < m_TargetType.getVectorSize(); ++i)
      {
        targetExprs[i] = Extract_(DeclRef_(vectVar->getDecl()), i + m_Index, m_TargetType);
      }
      break;
    }
    default:
      assert(0 && "unknown result");
      break;
  }
}

//--------------------------------------------------------- 
Expr* TargetStmt::split_subexpressions(Expr* Node)
{
  tUnrolledExprs setExprs(m_TargetType.getVectorSize());
  if (CallExpr* CE = dyn_cast<CallExpr>(Node))
  {
    std::vector<tUnrolledExprs> subExprs(CE->getNumArgs());
    for (unsigned i = 0; i < CE->getNumArgs(); ++i)
    {
      getSplitSubExpr(CE->getArg(i), subExprs[i]);
    }
    for (unsigned i = 0; i < m_TargetType.getVectorSize(); ++i)
    {
      CallExpr* targetExpr = Clone_(CE);
      setExprs[i] = targetExpr;
      for (unsigned j = 0; j < CE->getNumArgs(); ++j)
      {
        targetExpr->setArg(j, subExprs[j][i]);
      }
    }
  }
  else
  {
    Stmt::child_iterator i = Node->child_begin(), e = Node->child_end();
    std::vector<tUnrolledExprs> subExprs;
    if (isa<ConditionalOperator>(Node))
    {
      subExprs.push_back(tUnrolledExprs());
      TargetScalarStmt(*this).rollout(*i, subExprs.back());

      // further special handling or ++i? 
      // with ++i the conditional branches both are vectorized evaluated 
      // -> problem if the condition prevents exceptions (e.g. a != 0 ? 1/a : 0)
      // with return the conditional branches are scalar computed on demand only
      ++i; 
      // we use the aggressive variant, because it is consistent with cases, 
      // where a blend-op (e.g. sse4) exists and it is also possible to 
      // circumvent potential problems by using if else instead of ?:
    }
    for (; i != e; ++i)
    {
      subExprs.push_back(tUnrolledExprs());
      getSplitSubExpr(cast<Expr>(*i), subExprs.back());
    }
    for (unsigned idx = 0; idx < m_TargetType.getVectorSize(); ++idx)
    {
      setExprs[idx] = Clone_(Node);
      unsigned j = 0;
      for (Stmt::child_iterator i = setExprs[idx]->child_begin(), 
           e = setExprs[idx]->child_end(); i != e; ++i, ++j)
      {
        *i = subExprs[j][idx];
      }
    }
  }
  return IntrinsicCall_("set", setExprs, m_TargetType);
}

//--------------------------------------------------------- 
Expr* TargetStmt::split_without_tempvar(Expr* Node)
{
  Node = stripParenCasts(Node);
  if (Expr* vecRef = m_Info.getVectorizedVarRHS(Node, *this))
  {
    return vecRef;
  }

  llvm::SmallVector<Expr*, 4> intrinsicArgs;
  if (FunctionDecl* FD = getIntrinsic(Node, intrinsicArgs))
  {
    for (unsigned i = 0, e = intrinsicArgs.size(); i != e; ++i)
    {
      intrinsicArgs[i] = splitEnsureVar(intrinsicArgs[i]);
    }
    m_Info.m_Statistics.numPackedOps++;
    return Call_(FD, &intrinsicArgs[0], intrinsicArgs.size());
  }
  return split_subexpressions(Node);
}

//--------------------------------------------------------- 
FunctionDecl* TargetStmt::getIntrinsic(Expr* Node, llvm::SmallVector<Expr*, 4>& intrinsicArgs)
{
  FunctionDecl* FD = 0;
  if (CallExpr* CE = dyn_cast<CallExpr>(Node))
  {
    DeclRefExpr* fnDecl;
    if ((fnDecl = dyn_cast<DeclRefExpr>(CE->getCallee()->IgnoreParenCasts())) != 0 && 
        (FD = dyn_cast<FunctionDecl>(fnDecl->getDecl())) != 0 && 
        (FD = getIntrinsicFn(FD, m_TargetType)) != 0)
    {
      // TODO: support vectorized functions with heterogenous args?
      for (CallExpr::arg_iterator i = CE->arg_begin(), e = CE->arg_end(); 
           i != e; ++i)
      {
        intrinsicArgs.push_back(*i);
      }
    }
  }
  else
  {
    FD = getIntrinsicExpr(Node, m_TargetType, intrinsicArgs);
  }
  return FD;
}

//--------------------------------------------------------- 
void TargetStmt::emitLHS(Expr* sourceLHS, DeclRefExpr* targetRhs)
{
  Expr* intrinsicExpr = emitStoreIntrinsic(sourceLHS, targetRhs);
  if (intrinsicExpr != 0)
  {
    m_GeneratedStmts.push_back(intrinsicExpr);
  }
  else
  {
    unrollComplexLHS(sourceLHS, targetRhs);
  }
}

//--------------------------------------------------------- 
Expr* TargetStmt::unrollComplexRHS(Expr* RHS)
{
  // vectorizes scalar expr 
  // to
  // scout_set(expr_{scout_extract(var, index)}, 
  //           expr_{scout_extract(var, index+1)}, ..., 
  //           expr_{scout_extract(var, index+n)}); 
  // all vectorized vars are replaced with scout_extract, n is targetType.m_VectorSize

  llvm::SmallVector<Expr*, 8> setExprs;
  TargetScalarStmt(*this).rollout(RHS, setExprs);
  return IntrinsicCall_("set", setExprs, m_TargetType);
}

//--------------------------------------------------------- 
void TargetStmt::unrollComplexLHS(Expr* LHS, DeclRefExpr* targetRHS)
{
  // unrolls expr = var; 
  // to
  // expr_{0} = scout_extract(var, m_Index);
  // expr_{1} = scout_extract(var, m_Index + 1); aso.
  // expr_{n} = scout_extract(var, m_Index + n); n is m_TargetType.m_VectorSize
  bool bUseNtStore = m_Info.isNonTemporal(LHS) && 
                     hasIntrinsicBuiltin("store_nt_scalar", m_TargetType);
  llvm::SmallVector<Expr*, 8> setExprs;
  TargetScalarStmt(*this).rollout(LHS, setExprs);
  for (unsigned i = 0; i < setExprs.size(); ++i)
  {
    DeclRefExpr* RHS = i == 0 ? targetRHS : Clone_(targetRHS);
    Expr* storeExpr = bUseNtStore ? 
      cast<Expr>(StoreNtScalar_(getSimplifiedLoc(setExprs[i]), RHS, i, m_TargetType)) :
      cast<Expr>(Assign_(setExprs[i], m_Info.createScalarExtract(RHS, m_TargetType, i)));
    m_GeneratedStmts.push_back(storeExpr);
  }
}

//--------------------------------------------------------- 
TargetScalarStmt::TargetScalarStmt(VectorizeInfo& info, unsigned targetSize) :
  IntrinsicEditor(info),
  m_Info(info),
  m_Size(targetSize),
  m_Index(0)
{}

//--------------------------------------------------------- 
TargetScalarStmt::TargetScalarStmt(TargetStmt& sourceStmt) :
  IntrinsicEditor(sourceStmt),
  m_Info(sourceStmt.m_Info),
  m_Size(sourceStmt.m_TargetType.getVectorSize()),
  m_Index(sourceStmt.m_Index * sourceStmt.m_TargetType.getVectorSize())
{}

//--------------------------------------------------------- 
Expr* TargetScalarStmt::insertStrideIndex(unsigned idx, Expr* strideExpr)
{
  if (strideExpr == 0)
  {
    return Int_(idx);
  }
  return idx == 1 ? strideExpr : Paren_(BinaryOp_(strideExpr, Int_(idx), BO_Mul)); 
}  


//--------------------------------------------------------- 
Stmt* TargetScalarStmt::scalarizeTarget(Stmt* Node, StmtCloneMapping& mapping, unsigned idx)
{
  Stmt* targetStmt = mapping.m_StmtMapping[Node];
  BinaryOperator* BO;
  stmt_iterator<Expr> i = stmt_ibegin(Node), 
                      e = stmt_iend(Node); 
  while (i != e)
  {
    Expr* sourceNode = *i;
    if ((BO = dyn_cast<BinaryOperator>(sourceNode)) && BO->isAssignmentOp())
    {
      i.skipChildren();

      BinaryOperator* targetBO = cast<BinaryOperator>(mapping.m_StmtMapping[BO]);
      targetBO->setRHS(cast<Expr>(scalarizeTarget(BO->getRHS(), mapping, idx)));
      VectorizeInfo::tUnrolledLHSResult tempVectVar = m_Info.getVectorizedVarForUnrollLHS(BO->getLHS(), targetBO->getRHS(), Node, idx);
      if (tempVectVar.m_Assign != 0)
      {
        Expr* newExpr = tempVectVar.m_Assign;
        if (tempVectVar.m_TempVarExtract != 0)
        {
          Expr* scalaredLHS = cast<Expr>(scalarizeTarget(BO->getLHS(), mapping, idx));
          
          // this transforms "LHS = RHS" to 
          // "(insert(TEMP, RHS, idx), LHS = extract(TEMP, idx))" or
          // "(TEMP[idx] = RHS, LHS = TEMP[idx])"
          // which eventually leads to the same expression result
          newExpr = Paren_(BinaryOp_(
            newExpr, 
            Assign_(scalaredLHS, tempVectVar.m_TempVarExtract), 
            BO_Comma));
        }

        if (sourceNode == Node)
        {
          return newExpr;
        }

        replaceStatement(targetBO, newExpr);
      }
      else
      {
        targetBO->setLHS(cast<Expr>(scalarizeTarget(BO->getLHS(), mapping, idx)));
      }
      continue;
    }
    Expr* vecRef = m_Info.getBoundVectorizedVar(sourceNode, idx);
    if (vecRef != 0)
    {
      if (sourceNode == Node)
      {
        return vecRef;
      }

      i.skipChildren();
      replaceStatement(mapping.m_StmtMapping[sourceNode], vecRef);
      continue;
    }
    StridedExprResult stride = m_Info.getSimplifiedStride1Expr(sourceNode);
    if (stride.m_Result == STRIDE1)
    {
      i.skipChildren();
      if (idx > 0)
      {
        Expr* clonedExpr = cast<Expr>(mapping.m_StmtMapping[sourceNode]);
        Expr* stridedExpr = Paren_(Add_(clonedExpr, insertStrideIndex(idx, stride.m_StrideExpr)));
        if (sourceNode == Node)
        {
          return fold(stridedExpr, *this);
        }

        Stmt* P = getParent(clonedExpr);
        assert(P != 0);
        Stmt::child_iterator replacePoint = std::find(P->child_begin(), P->child_end(), clonedExpr);
        assert(replacePoint != P->child_end());

        *replacePoint = fold(stridedExpr, *this);
        updateParentMap(P);
      }
      continue;
    }
    ++i;
  }
  return targetStmt;
}

//--------------------------------------------------------- 
void TargetScalarStmt::rollout(Stmt* Node, llvm::SmallVector<Expr*, 8>& exprs)
{
  exprs.resize(m_Size);
  rollout(Node, (Stmt**)(&exprs[0]));
}

//--------------------------------------------------------- 
void TargetScalarStmt::rollout(Stmt* Node, Stmt** rolloutTarget)
{
  for (unsigned idx = 0; idx < m_Size; ++idx)
  {
    StmtCloneMapping mapping;
    cloneStmtTree(*this, Node, &mapping);
    rolloutTarget[idx] = scalarizeTarget(Node, mapping, m_Index + idx);
  }
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

