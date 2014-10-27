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

#include "clang/Vectorizing/Vectorize.h"    
#include "clang/Vectorizing/Analysis.h"    
#include "clang/Vectorizing/IntrinsicEditor.h"    
#include "clang/Vectorizing/TargetStmt.h"

#include "clang/ASTProcessing/ModificationNoteBuilder.h"
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/ASTProcessing/DeclCollector.h" 
#include "clang/ASTProcessing/AttachedPragmas.h" 
#include "clang/ASTProcessing/Inline.h"    
#include "clang/ASTProcessing/LoopUnroll.h"    
#include "clang/ASTProcessing/InvariantIf.h"    
#include "clang/Vectorizing/VectorizeInfo.h"
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/DeclCXX.h"    
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"
#include <list>
#include <map>

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {


//--------------------------------------------------------- 
class Vectorizer : IntrinsicEditor
{
public:
  Vectorizer(IntrinsicEditor& editor) : 
    IntrinsicEditor(editor),
    m_GlobalAssigns(editor) {}
  bool vectorizeLoop(ForStmt* Node);
  void emitConstantAssigns();

private:
  void prepareSplitInvariantIf(VectorizeInfo& info, ForStmt* forNode);
  CompoundStmt* splitLoopAtEnd(VectorizeInfo& info, ForStmt* Node, bool columnVectorized);
  void alignLoopStart(VectorizeInfo& info, CompoundStmt* CS, ForStmt* Node);
  void emitLoopInvariantAssigns(CompoundStmt* CS, VectorizeInfo& info);

  Stmt* generateRestLoop(VectorizeInfo& info, Stmt* ifBody, VarDecl* stride1Var, 
                         DeclStmt* tmpArray, DeclStmt* tmpIndex);
  void replaceIndex(VectorizeInfo& info, Stmt* ifBody, 
                    VarDecl* stride1Var, DeclStmt* tmpArray);
  CompoundStmt* transformGuardingIf(VectorizeInfo& info, ForStmt* Node, 
                         IfStmt* guardingIf, VarDecl* stride1Var);
  Stmt* transformIfBody(VectorizeInfo& info, Stmt* ifBody, VarDecl* stride1Var, 
                        llvm::SmallVector<Stmt*, 6>& enclosingCS);
  CompoundStmt* appendIndexReset(Stmt* ifBody, DeclStmt* tmpIndex);
  Expr* getVectorizedForIncrement(VectorizeInfo& info, Expr* Node);

  bool vectorizeColumn(ForStmt* Node);

  enum eRemainderHandling { NO_REMAINDER, AUTO_SPLIT, COLUMN_SPLIT };

  struct tPragmaInfo
  {
    const PragmaArgumentInfo* m_PragmaInfo;
    eRemainderHandling        m_Remainder;
    Expr*                     m_CondRHS;
    tPragmaInfo(const PragmaArgumentInfo* info, bool isColumn) :
      m_PragmaInfo(info),
      m_Remainder(isColumn ? COLUMN_SPLIT : 
                             (info->hasArgument("noremainder") ? 
                              NO_REMAINDER : AUTO_SPLIT)),
      m_CondRHS(0)
    {}
  };

  bool vectorizePreparedLoop(ForStmt* Node, tPragmaInfo& pragmaInfo);
  Expr* getAndReplaceAlignedLoopEndExpr(VectorizeInfo& info, ForStmt* Node);


  CompoundStmt* binarizeExprs(VectorizeInfo& info)
  {
    CompoundStmt* result;
    for (VectorizeInfo::tVectExpressions::iterator i = info.m_AllExprs.begin(), 
         e = info.m_AllExprs.end(); i != e; ++i)
    {
      if (Stmt* S = i->getUnrollStmt())
      { 
        llvm::SmallVector<Stmt*, 8> setExprs(info.m_VectorSize);
        TargetScalarStmt(info, info.m_VectorSize).rollout(S, &setExprs[0]);
        replaceStatement(S, Compound_(&setExprs[0], setExprs.size()));
        continue;
      }

      BinaryOperator* Node = i->getAssign();
      assert(Node);
      if (!(i->getVectorType().isValid() || i->isForcedUnroll()))
      {
        continue;
      }

      Stmt* exprStmt = getStatementOfExpression(Node);
      CompoundStmt* CS = ensureCompoundParent(exprStmt);
      std::vector<Stmt*> compoundStmts(CS->child_begin(), CS->child_end());
      std::vector<Stmt*>::iterator insertPos = 
        std::find(compoundStmts.begin(), compoundStmts.end(), exprStmt);
      assert(insertPos != compoundStmts.end());
      if (i->isForcedUnroll())
      {
        TargetScalarStmt::tUnrolledExprs setExprs(info.m_VectorSize);
        TargetScalarStmt(info, info.m_VectorSize).rollout(Node, setExprs);
        if (Node == exprStmt)
        {
          insertPos = compoundStmts.erase(insertPos);
        }
        else
        {
          assert(0 && "unchecked so far");
          replaceStatement(Node, Node->getLHS());
        }
        compoundStmts.insert(insertPos, setExprs.begin(), setExprs.end());
      }
      else
      {
        std::list<Stmt*> tempAssigns;
        const SimdType& targetType = i->getVectorType();
        TargetStmt generator(info, targetType);
        for (unsigned index = 0, 
             endIndex = info.m_VectorSize / targetType.getVectorSize();
             index < endIndex; ++index)
        {
          tempAssigns.splice(tempAssigns.end(), generator.generate(Node, index));
        }

        if (Node == exprStmt)
        {
          insertPos = compoundStmts.erase(insertPos);
        }
        else
        {
          assert(0 && "unchecked so far");
          replaceStatement(Node, Node->getLHS());
        }
        compoundStmts.insert(insertPos, tempAssigns.begin(), tempAssigns.end());
      }
      if (compoundStmts.empty())
      {
        replaceStmts(CS, 0, 0);
      }
      else
      {
        replaceStmts(CS, &compoundStmts[0], compoundStmts.size());
      }
      result = CS;
    }
    return result;
  }

  void expandLocalVars(VectorizeInfo& info, Expr* expr)
  {
    for (MemRegion_iterator i(expr, info.m_ValueFlowContext), 
         e(info.m_ValueFlowContext); i != e;)
    {
      if (*i != expr)
      {
        llvm::DenseMap<const MemRegion*, Expr*>::iterator i2 = 
          info.m_LocalVarWrites.find(i.region());
        if (i2 != info.m_LocalVarWrites.end())
        {
          Expr* clonedExpr = cloneStmtTree(*this, i2->second);
          replaceStatement(*i, Paren_(clonedExpr));
          i.skipChildren();
          continue;
        }
      }
      ++i;
    }
  }


  llvm::DenseSet<ForStmt*>  m_TouchedLoops;

  VectorizeInfo::tFunctionGlobalAssigns m_GlobalAssigns;   
};

//--------------------------------------------------------- 
Expr* Vectorizer::getAndReplaceAlignedLoopEndExpr(VectorizeInfo& info, ForStmt* Node)
{
  // fortran loop, so we know for sure:
  BinaryOperator* forCond = cast<BinaryOperator>(Node->getCond());
  Expr* result = BinaryOp_(forCond->getRHS(), Int_(info.m_VectorSize-1), BO_Sub);
  forCond->setRHS(result);
  return result;
}

//--------------------------------------------------------- 
CompoundStmt* Vectorizer::splitLoopAtEnd(VectorizeInfo& info, ForStmt* Node, bool columnVectorized)
{
  ForStmt* restLoop = cloneStmtTree(*this, Node);
  if (columnVectorized)
  {
    for (stmt_iterator<ForStmt> i = stmt_ibegin(restLoop), 
         e = stmt_iend(restLoop); i != e; ++i)
    {
      m_TouchedLoops.insert(*i);
    }    
  }
  else
  {
    m_TouchedLoops.insert(restLoop);
  }
  restLoop->setInit(0);
  getAndReplaceAlignedLoopEndExpr(info, Node);
  return Compound_(Node, restLoop);
}

//--------------------------------------------------------- 
void Vectorizer::alignLoopStart(VectorizeInfo& info, CompoundStmt* CS, ForStmt* Node)
{
  if (info.m_AlignedArraySubscripts.empty())
  {
    Note(Node, "warning: align ignored (no alignable array access found)");
    return;
  }

  ForStmt* startLoop = cloneStmtTree(*this, Node);
  m_TouchedLoops.insert(startLoop);

  Node->setInit(0);

  // fortran loop, so we know for sure:
  BinaryOperator* forCond = cast<BinaryOperator>(startLoop->getCond());

  Expr* cloneExpr = cloneStmtTree(*this, info.m_AlignedArraySubscripts.front());
  expandLocalVars(info, cloneExpr);
  cloneExpr = cloneExpr->IgnoreParenNoopCasts(Ctx());
  
  Expr* addrOfExpr;
  UnaryOperator* UO = dyn_cast<UnaryOperator>(cloneExpr);
  if (UO != 0 && UO->getOpcode() == UO_Deref)
  { 
    //&*ptrExpr -> ptrExpr
    addrOfExpr = UO->getSubExpr();
  }
  else if (ArraySubscriptExpr* AS = dyn_cast<ArraySubscriptExpr>(cloneExpr))
  {
    //&a[i] -> a + i
    addrOfExpr = BinaryOp_(Paren_(AS->getBase()), Paren_(AS->getIdx()), BO_Add); 
  }
  else
  {
    //&(arrayexpr)
    addrOfExpr = UnaryOp_(Paren_(cloneExpr), UO_AddrOf); 
  }

  // expr -> expr && (uint)(addrOfExpr) & alignment-1 != 0
  startLoop->setCond(
    BinaryOp_(
      forCond,
      BinaryOp_(
        BinaryOp_(
          CCast_(Paren_(addrOfExpr), IntPtrTy_()),
          Int_(info.m_VectorByteAlignment-1), 
          BO_And),
        Int_(0), 
        BO_NE),
      BO_LAnd));    

  llvm::SmallVector<Stmt*, 4> newStmts;
  newStmts.push_back(startLoop);
  newStmts.append(CS->child_begin(), CS->child_end());
  replaceStmts(CS, &newStmts[0], newStmts.size());
}

//--------------------------------------------------------- 
void Vectorizer::emitConstantAssigns()
{
  DeclCollector collector(*this);
  CompoundStmt* rootCS = cast<CompoundStmt>(getRoot());
  llvm::SmallVector<Stmt*, 32> topLevelStmts;
  topLevelStmts.reserve(rootCS->size());
  Stmt::child_iterator i = rootCS->child_begin(), e = rootCS->child_end();
  while (i != e && (isa<DeclStmt>(*i)))
  {
    topLevelStmts.push_back(*i++);
  }
  addOptionalGlobalCall("vectorized_function_prolog", topLevelStmts);
  m_GlobalAssigns.generateFunctionGlobalAssigns(collector, topLevelStmts);
  topLevelStmts.append(i, e);

  addOptionalGlobalCall("vectorized_function_epilog", topLevelStmts);
  replaceStmts(rootCS, &topLevelStmts[0], topLevelStmts.size());
  collector.emitCollectedDecls();
}

//--------------------------------------------------------- 
void Vectorizer::emitLoopInvariantAssigns(CompoundStmt* CS, VectorizeInfo& info)
{
  DeclCollector collector(*this);
  llvm::SmallVector<Stmt*, 32> preStmts, postStmts;
  info.generateLoopEnclosingStmts(collector, preStmts, postStmts);
  preStmts.append(CS->child_begin(), CS->child_end());
  preStmts.append(postStmts.begin(), postStmts.end());
  replaceStmts(CS, &preStmts[0], preStmts.size());

  collector.collectAll(info.m_TempVars);
  collector.collectAll(info.m_NonVectorizedTempVars);
  collector.emitCollectedDecls();
}

//--------------------------------------------------------- 
Stmt* Vectorizer::generateRestLoop(VectorizeInfo& info, Stmt* ifBody, 
                                   VarDecl* stride1Var, DeclStmt* tmpArray, DeclStmt* tmpIndex)
{
  Stmt* restBody = cloneStmtTree(*this, ifBody);
  DeclStmt* restIndex = TmpVar_(Ctx().UnsignedIntTy);
  info.m_NonVectorizedTempVars.push_back(restIndex);
  ForStmt* restLoop = For_(
    Assign_(DeclRef_(restIndex), UInt_(0)), 
    BinaryOp_(DeclRef_(restIndex), DeclRef_(tmpIndex), BO_LT),
    UnaryOp_(DeclRef_(restIndex), UO_PreInc),
    restBody);

  m_TouchedLoops.insert(restLoop);

  for (stmt_iterator<DeclRefExpr> i = stmt_ibegin(restBody), 
       e = stmt_iend(restBody); i != e; ++i)
  {
    if ((*i)->getDecl() == stride1Var)
    {
      replaceStatement(*i, ArraySubscript_(DeclRef_(tmpArray), DeclRef_(restIndex)));
    }
  }

  return restLoop;
}


//--------------------------------------------------------- 
void Vectorizer::replaceIndex(VectorizeInfo& info, Stmt* ifBody, 
                              VarDecl* stride1Var, DeclStmt* tmpArray)
{
  Expr* dummyLHS = ArraySubscript_(DeclRef_(tmpArray), Int_(0));
  const MemRegion* arrayRegion = info.m_ValueFlowContext.getLValueMemRegion(dummyLHS);
  info.createLiveMemRegion(arrayRegion, STRIDE1, dummyLHS, STRIDE1_DEPENDENT);

  // ifBody_{stride1Var} -> ifBody_{tmpArray[0]}
  for (stmt_iterator<DeclRefExpr> i = stmt_ibegin(ifBody), 
       e = stmt_iend(ifBody); i != e; ++i)
  {
    if ((*i)->getDecl() == stride1Var)
    {
      Expr* index = Int_(0);
      Expr* arraySubscript = ArraySubscript_(DeclRef_(tmpArray), index);
      replaceStatement(*i, arraySubscript);
      info.setAnalyzedExprResult(index, STRIDE1);
      info.setAnalyzedExprResult(arraySubscript, STRIDE1_DEPENDENT);
      info.getLiveMemRegionResult(arrayRegion, STRIDE1, arraySubscript);
    }
  }
}

//--------------------------------------------------------- 
CompoundStmt* Vectorizer::appendIndexReset(Stmt* ifBody, DeclStmt* tmpIndex)
{
  // ifBody -> { ifBody; tmpIndex = 0; }
  CompoundStmt* thenBody = dyn_cast<CompoundStmt>(ifBody);
  if (thenBody == 0)
  {
    thenBody = ensureCompoundParent(ifBody);
  }
  appendStmt(thenBody, Assign_(DeclRef_(tmpIndex), UInt_(0)));
  return thenBody;
}

//--------------------------------------------------------- 
Stmt* Vectorizer::transformIfBody(VectorizeInfo& info, Stmt* ifBody, 
                                  VarDecl* stride1Var, llvm::SmallVector<Stmt*, 6>& enclosingCS)
{
  QualType arrayType = Ctx().getConstantArrayType(stride1Var->getType(), llvm::APInt(32, info.m_VectorSize), ArrayType::Normal, 0);
  DeclStmt* tmpArray = TmpVar_(arrayType, 0);
  info.m_NonVectorizedTempVars.push_back(tmpArray);
  DeclStmt* tmpIndex = TmpVar_(Ctx().UnsignedIntTy);
  info.m_NonVectorizedTempVars.push_back(tmpIndex);

  enclosingCS.insert(enclosingCS.begin(), Assign_(DeclRef_(tmpIndex), Int_(0))); 
  enclosingCS.push_back(generateRestLoop(info, ifBody, stride1Var, tmpArray, tmpIndex));
  replaceIndex(info, ifBody, stride1Var, tmpArray);
  CompoundStmt* thenBody = appendIndexReset(ifBody, tmpIndex);

  Stmt* thenCompund[2] = {
    Assign_(ArraySubscript_(DeclRef_(tmpArray), UnaryOp_(DeclRef_(tmpIndex), UO_PostInc)), DeclRef_(stride1Var)),
    If_(BinaryOp_(DeclRef_(tmpIndex), UInt_(info.m_VectorSize), BO_EQ), thenBody)
  };

  return Compound_(thenCompund);
}

//--------------------------------------------------------- 
CompoundStmt* Vectorizer::transformGuardingIf(VectorizeInfo& info, ForStmt* Node, 
                         IfStmt* guardingIf, VarDecl* stride1Var)
{
  llvm::SmallVector<Stmt*, 6> enclosingCS;
  enclosingCS.push_back(Node);
  guardingIf->setThen(transformIfBody(info, guardingIf->getThen(), stride1Var, enclosingCS));
  if (guardingIf->getElse() != 0)
  {
    guardingIf->setElse(transformIfBody(info, guardingIf->getElse(), stride1Var, enclosingCS));
  }
  return Compound_(&enclosingCS[0], enclosingCS.size());
}


//--------------------------------------------------------- 
void Vectorizer::prepareSplitInvariantIf(VectorizeInfo& info, ForStmt* forNode)
{
  info.moveConstAssigns();
  CompoundStmt* CS = ensureCompoundParent(forNode);

  llvm::SmallVector<Stmt*, 32> newStmts;
  newStmts.reserve(CS->size() + info.m_ScalarLoopInvariantExprs.size());
  newStmts.append(CS->child_begin(), CS->child_end());
  llvm::SmallVector<Stmt*, 32>::iterator i = 
    std::find(newStmts.begin(), newStmts.end(), forNode);
  assert(i != newStmts.end());

  newStmts.insert(i, info.m_ScalarLoopInvariantExprs.begin(), 
                     info.m_ScalarLoopInvariantExprs.end());
  replaceStmts(CS, &newStmts[0], newStmts.size());
}

//--------------------------------------------------------- 
Expr* Vectorizer::getVectorizedForIncrement(VectorizeInfo& info, Expr* Node)
{
  if (BinaryOperator* BO = dyn_cast<BinaryOperator>(Node))
  {
    switch (BO->getOpcode())
    {
    case BO_Comma:
      return BinaryOp_(getVectorizedForIncrement(info, BO->getLHS()), 
                       getVectorizedForIncrement(info, BO->getRHS()), 
                       BO_Comma);

    case BO_AddAssign:
    case BO_SubAssign:
      return BinaryOp_(Clone_(BO->getLHS()), 
                       BinaryOp_(Paren_(Clone_(BO->getRHS())), 
                                 Int_(info.m_VectorSize), 
                                 BO_Mul),
                       BO->getOpcode());

    default:
      assert(0 && "BinaryOperator should have been tested before");
      return Node;
    }
  }

  if (UnaryOperator* UO = dyn_cast<UnaryOperator>(Node))
  {
    BinaryOperator::Opcode opc = BO_AddAssign;
    switch (UO->getOpcode())
    {
    case UO_PostDec:
    case UO_PreDec:
      opc = BO_SubAssign;
      // fall through
    case UO_PreInc:
    case UO_PostInc:
      return BinaryOp_(Clone_(UO->getSubExpr()), Int_(info.m_VectorSize), opc);

    default:
      assert(0 && "UnaryOperator should have been tested before");
      return Node;
    }
  }

  if (ParenExpr* PE = dyn_cast<ParenExpr>(Node))
  {
    return Paren_(getVectorizedForIncrement(info, PE->getSubExpr()));
  }

  assert(0 && "should have been tested before");
  return Node;
}

//--------------------------------------------------------- 
bool Vectorizer::vectorizeColumn(ForStmt* Node)
{
  ForStmt* FS;
  CompoundStmt* outerLoopBody = cast<CompoundStmt>(Node->getBody());
  if (outerLoopBody->size() == 1 &&
      (FS = dyn_cast<ForStmt>(*outerLoopBody->body_begin())) != 0)
  {
    const PragmaArgumentInfo* pragmaInfo = findAttachedPragma(FS, "loop", "vectorize");
    if (pragmaInfo != 0)
    {
      ForStmt* firstOuterLoop = cloneStmtTree(*this, Node);
      Stmt* simpleCS[2] = { firstOuterLoop, Node };
      replaceStatement(Node, Compound_(simpleCS));
      outerLoopBody = cast<CompoundStmt>(firstOuterLoop->getBody());
      ForStmt* firstFS = cast<ForStmt>(*outerLoopBody->body_begin());
      m_TouchedLoops.insert(firstOuterLoop);
      m_TouchedLoops.insert(firstFS);
      m_TouchedLoops.insert(FS);
      tPragmaInfo pInfo (pragmaInfo, true);
      vectorizePreparedLoop(firstFS, pInfo);
      if (pInfo.m_CondRHS != 0)
      {
        cast<BinaryOperator>(FS->getInit())->setRHS(pInfo.m_CondRHS);
        return true;
      }
    }
  }
  return false;
}

//--------------------------------------------------------- 
bool Vectorizer::vectorizeLoop(ForStmt* Node)
{
  if (m_TouchedLoops.count(Node))
  {
    return false;
  }
  m_TouchedLoops.insert(Node);

  const PragmaArgumentInfo* pragmaInfo = findAttachedPragma(Node, "loop", "vectorize");
  if (pragmaInfo == 0)
  {
    return false;
  }

  if (!isa<CompoundStmt>(Node->getBody()))
  {
    ensureCompoundParent(Node->getBody());
  }

  doInline(*this, Node->getBody());
  removeInnerAssigns(*this, Node->getBody());
  splitInvariantIfs(*this, Node->getBody(), false);
  DeclCollector::makeCAst(*this);
  unrollRecordAssigns(*this, Node->getBody());
  tPragmaInfo pInfo(pragmaInfo, false);
  return vectorizePreparedLoop(Node, pInfo);
}

//--------------------------------------------------------- 
bool Vectorizer::vectorizePreparedLoop(ForStmt* Node, tPragmaInfo& pInfo)
{
  const char* pErrMsg;
  VectorizeInfo info(*this, m_GlobalAssigns);
  if (!info.analyzeForStmt(Node, pErrMsg, false))
  {
    Warn(Node, pErrMsg);
    return true;
  }
  
  const PragmaArgumentInfo* pragmaInfo = pInfo.m_PragmaInfo;
  int uForcedVectorSize = 0;
  pragmaInfo->findValueForArgument("size", Ctx(), uForcedVectorSize);

  AnalyzeResult analyzeResult = analyzeBody(Node, info);
  CompoundStmt* enclosingCS;
  switch(analyzeResult.m_Result)
  {
    case AnalyzeResult::VECTORIZE:
    {
      if (pragmaInfo->hasArgument("scalar"))
      {
        return true;
      }
      
      bool columnVectorized = vectorizeColumn(Node);

      if (!info.postInit(uForcedVectorSize))
      {
        return true;
      }

      Stmt* simpleCS[1] = { Node };
      switch (pInfo.m_Remainder)
      {
      case COLUMN_SPLIT:
        pInfo.m_CondRHS = getAndReplaceAlignedLoopEndExpr(info, Node);
        // fall through
      case NO_REMAINDER:
        enclosingCS = Compound_(simpleCS, 1);
        break;
      case AUTO_SPLIT: 
        enclosingCS = splitLoopAtEnd(info, Node, columnVectorized);
        break;
      }
      replaceStatement(Node, enclosingCS);

      Expr* alignExpr = pragmaInfo->getExprForArgument("aligned");
      if (alignExpr != 0)
      {
        info.initExplicitAlignment(alignExpr);
      }

      if (pragmaInfo->hasArgument("align"))
      {
        if (alignExpr == 0)
        {
          info.initMostDirectArrayAccess();
        }
        alignLoopStart(info, enclosingCS, Node);
      }
      
      Expr* ntExpr = pragmaInfo->getExprForArgument("nontemporal");
      if (ntExpr != 0)
      {
        info.initExplicitNonTemporal(ntExpr);
      }

      Node->setInc(getVectorizedForIncrement(info, Node->getInc()));
      break;
    }

    case AnalyzeResult::SPLIT_IF:
      assert(analyzeResult.m_InvariantIf != 0);
      prepareSplitInvariantIf(info, Node);
      splitInvariantIf(*this, Node, analyzeResult.m_InvariantIf, analyzeResult.m_StaticInvariantIfResult);
      m_TouchedLoops.erase(Node);
      return true;

    case AnalyzeResult::GUARDING_IF:
    {
      assert(analyzeResult.m_GuardingIf != 0);
      if (pragmaInfo->hasArgument("scalar"))
      {
        return true;
      }

      // if-collection needs stricter constraints to the for-stmt:
      VarDecl* stride1Var = isFortranLoop(Node, pErrMsg);
      if (stride1Var == 0)
      {
        Warn(Node, pErrMsg);
        return false;
      }

      if (!info.postInit(uForcedVectorSize, true))
      {
        return true;
      }
      enclosingCS = transformGuardingIf(info, Node, analyzeResult.m_GuardingIf, 
                                        stride1Var);
      replaceStatement(Node, enclosingCS);
      break;
    }
    default:
      assert(analyzeResult.m_Result == AnalyzeResult::DONT_VECTORIZE);
      return true;
  }

  info.m_Statistics = VectorizeInfo::tStatistics();
  CompoundStmt* lastCS = binarizeExprs(info);

  emitLoopInvariantAssigns(enclosingCS, info);

  std::string streamString;
  llvm::raw_string_ostream commentStream(streamString);
  commentStream.str().erase();
  commentStream << info.m_Statistics.numLoadOps << " packed loads, " 
                << info.m_Statistics.numStoreOps << " packed stores, "
                << info.m_Statistics.numPackedOps << " packed ops";
  // is this of any interest:
  //info.m_Statistics.numPackedAssigns
  attachComment(Node, commentStream.str().c_str());  

  ModificationNote(Node, "loop vectorized {tgt:%0:%1}");
  return true;
}


//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
bool vectorizeLoops(IntrinsicEditor& editor, Stmt* Node)
{
  editor.setIdentifierPolicy("vect");
  bool bVectorized = false;
  Vectorizer vectorizer(editor);
  for (stmt_iterator<ForStmt> i = stmt_ibegin(Node), 
       e = stmt_iend(Node); i != e;)
  {
    if (vectorizer.vectorizeLoop(*i))
    {
      i = stmt_ibegin(Node);
      bVectorized = true;
      continue;
    }
    ++i;
  }
  if (bVectorized)
  {
    vectorizer.emitConstantAssigns();
    flattenBlocks(editor, Node);
  }
  editor.restoreIdentifierPolicy();
  return bVectorized;
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

