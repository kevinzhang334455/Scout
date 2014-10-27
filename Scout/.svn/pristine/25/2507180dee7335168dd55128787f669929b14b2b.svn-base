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

#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/DeclCXX.h"    
#include "clang/AST/ASTContext.h"    
#include "llvm/Support/raw_ostream.h"

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
struct RenameLabels : StmtVisitor<RenameLabels>, StmtEditor
{
  RenameLabels(StmtEditor& editor) : StmtEditor(editor) {}

  void VisitLabelStmt(LabelStmt* label) 
  { 
    label->setDecl(createLabelDecl(label));
  }
};

//--------------------------------------------------------- 
struct RechainRefs : StmtVisitor<RechainRefs> 
{
  const StmtCloneMapping& m_ClonedStmts;
  RechainRefs(const StmtCloneMapping& clonedStmts) : m_ClonedStmts(clonedStmts) {}

  static StmtClone::StmtMapping::const_iterator findLabel(const StmtClone::StmtMapping& mapping, LabelDecl* LD)
  {
    LabelStmt* LS;
    StmtClone::StmtMapping::const_iterator i = mapping.begin(), e = mapping.end(); 
    for (; i != e; ++i)
    {
      if ((LS = dyn_cast<LabelStmt>(i->first)) != 0 && LS->getDecl() == LD)
      {
        break;
      }
    }
    return i;
  }


  void VisitGotoStmt(GotoStmt* Node) 
  { 
    StmtClone::StmtMapping::const_iterator i = findLabel(m_ClonedStmts.m_StmtMapping, Node->getLabel());
    if (i != m_ClonedStmts.m_StmtMapping.end())
    {
      Node->setLabel(static_cast<LabelStmt*>(i->second)->getDecl());
    }
  }

  void VisitDeclRefExpr(DeclRefExpr* Node)
  {
    StmtClone::DeclMapping::const_iterator i = m_ClonedStmts.m_DeclMapping.find(Node->getDecl());
    if (i != m_ClonedStmts.m_DeclMapping.end())
    {
      Node->setDecl(i->second);
    }
  }

  void VisitSwitchStmt(SwitchStmt* Node)
  { 
    StmtClone::StmtMapping::const_iterator i = m_ClonedStmts.m_StmtMapping.find(Node->getSwitchCaseList());
    if (i != m_ClonedStmts.m_StmtMapping.end())
    {
      Node->setSwitchCaseList(static_cast<SwitchCase*>(i->second));
    }
  }

  void VisitAddrLabelExpr(AddrLabelExpr* Node)
  { 
    StmtClone::StmtMapping::const_iterator i = findLabel(m_ClonedStmts.m_StmtMapping, Node->getLabel());
    if (i != m_ClonedStmts.m_StmtMapping.end())
    {
      Node->setLabel(static_cast<LabelStmt*>(i->second)->getDecl());
    }
  }
};

//--------------------------------------------------------- 
struct FlattenBlocks : public StmtVisitor<FlattenBlocks>, StmtEditor
{
  bool m_bAllBlocks;

  FlattenBlocks(StmtEditor& editor, bool bAllBlocks) : StmtEditor(editor), m_bAllBlocks(bAllBlocks) {}

  void VisitCompoundStmt(CompoundStmt* Node)
  {
    std::vector<Stmt*> newStmts;
    bool result = false;
    for (llvm::df_iterator<Stmt*> i = ++llvm::df_iterator<Stmt*>::begin(Node), 
         e = llvm::df_iterator<Stmt*>::end(Node); i != e;)
    {
      if (dyn_cast<CompoundStmt>(*i) != 0 && (m_bAllBlocks || !isOriginalStmt(*i)))
      {
        ++i;  // dive into childs
        result = true;
      }
      else
      {
        newStmts.push_back(*i);
        i.skipChildren();
      }
    }
    if (result)
    {
      if (newStmts.empty())
      {
        removeStmt(Node);
      }
      else
      {
        replaceStmts(Node, &newStmts[0], newStmts.size());
      }
    }
  }
};

//--------------------------------------------------------- 
struct RemoveGotosToNextLabel : public StmtVisitor<RemoveGotosToNextLabel>, StmtEditor
{
  struct tUseMarker
  {
    LabelStmt* LS;
    bool       m_Used;
    tUseMarker() : LS(0), m_Used(false) {}
    void setLabel(LabelStmt* L) { LS = L; }
    void markUsed() { m_Used = true; }
  };

  typedef llvm::DenseMap<LabelDecl*, tUseMarker> tTouchedLabels;
  tTouchedLabels m_TouchedLabels;

  RemoveGotosToNextLabel(StmtEditor& editor) : StmtEditor(editor) {}

  void VisitCompoundStmt(CompoundStmt* Node)
  {
    llvm::SmallVector<Stmt*, 8> newStmts(Node->size());
    size_t stmtPos = 0;
    for (Stmt::child_iterator i = Node->child_begin(), e = Node->child_end(); i != e; ++i)
    {
      if (GotoStmt* GS = dyn_cast<GotoStmt>(*i))
      {
        Stmt::child_iterator nextStmt = i;
        ++nextStmt;
        LabelStmt* LS;
        if (nextStmt != e && 
            (LS = dyn_cast<LabelStmt>(*nextStmt)) != 0 &&
            (!isOriginalStmt(LS)) && 
            LS->getDecl() == GS->getLabel())
        {
          m_TouchedLabels[LS->getDecl()].setLabel(LS); 
          continue;
        }
      }
      newStmts[stmtPos++] = *i;
    }
    if (stmtPos != newStmts.size())
    {
      replaceStmts(Node, &newStmts[0], stmtPos);
    }
  }

  void VisitGotoStmt(GotoStmt* Node)
  {
    m_TouchedLabels[Node->getLabel()].markUsed();
  }

  void VisitLabelStmt(LabelStmt* Node)
  {
    // necessary for orphan labels
    if (!isOriginalStmt(Node))
    {
      m_TouchedLabels[Node->getDecl()].setLabel(Node);
    }
  }
};

//--------------------------------------------------------- 
struct MoveDeclsToFirstWrite : public StmtVisitor<MoveDeclsToFirstWrite>, StmtEditor
{
  MoveDeclsToFirstWrite(StmtEditor& editor) : StmtEditor(editor) {} 

  static bool isVar(DeclRefExpr* refExpr, VarDecl* varDecl)
  {
    return refExpr->getDecl() == varDecl;
  }

  static bool findFirstAssign(Stmt* node, VarDecl* varDecl)
  {
    BinaryOperator* binOp = dyn_cast<BinaryOperator>(node);
    if (binOp != 0 && binOp->getOpcode() == BO_Assign)
    {
      DeclRefExpr* refExpr = dyn_cast<DeclRefExpr>(binOp->getLHS());
      return refExpr != 0 && isVar(refExpr, varDecl);
    }
    return false;
  }

  static bool hasRef(Stmt* Node, VarDecl* VD)
  {
    // this catches cases like:
    // int x;
    // if (y) { x = 0; goto z; }
    // x = 1;
    // z:
    for (stmt_iterator<DeclRefExpr> i = stmt_ibegin(Node), 
         e = stmt_iend(Node); i != e; ++i) 
    {
      if (isVar(*i, VD)) 
      {
        return true;
      }
    }
    return false;
  }

  void VisitCompoundStmt(CompoundStmt* Node)
  {
    llvm::SmallVector<Stmt*, 8> newStmts(Node->size());
    size_t stmtPos = 0;
    for (Stmt::child_iterator i = Node->child_begin(), e = Node->child_end(); i != e; ++i)
    {
      DeclStmt* DS = dyn_cast<DeclStmt>(*i);
      if (DS && DS->isSingleDecl() && !isOriginalStmt(DS))
      {
        VarDecl* VD = dyn_cast<VarDecl>(DS->getSingleDecl());
        if (VD && VD->getInit() == 0)
        {
          Stmt::child_iterator firstAssign = std::find_if(i, e, std::bind(&MoveDeclsToFirstWrite::findFirstAssign, _1, VD));
          if (firstAssign != e && 
              std::find_if(i, firstAssign, std::bind(&MoveDeclsToFirstWrite::hasRef, _1, VD)) == firstAssign)
          {
            BinaryOperator* binOp = dyn_cast<BinaryOperator>(*firstAssign);
            assert(binOp);
            VD->setInit(binOp->getRHS());
            *firstAssign = DS;
            continue;
          }
        }
      }
      newStmts[stmtPos++] = *i;
    }
    if (stmtPos != newStmts.size())
    {
      replaceStmts(Node, &newStmts[0], stmtPos);
    }
  }
};

//--------------------------------------------------------- 
struct CountVarUses : public StmtVisitor<CountVarUses>, StmtEditor
{
  typedef llvm::DenseMap<VarDecl*, std::pair<DeclStmt*, int> > tVarRefCountMap;
  tVarRefCountMap m_VarUseCount;

  CountVarUses(StmtEditor& editor) : StmtEditor(editor) {} 

  void VisitDeclStmt(DeclStmt* Node)
  {
    if ((!isOriginalStmt(Node)) && Node->isSingleDecl())
    {
      if (VarDecl* VD = dyn_cast<VarDecl>(Node->getSingleDecl()))
      {
        m_VarUseCount[VD] = std::make_pair(Node, 0);
      }
    }
  }

  void VisitDeclRefExpr(DeclRefExpr* Node)
  {
    if (VarDecl* VD = dyn_cast<VarDecl>(Node->getDecl()))
    {
      tVarRefCountMap::iterator i = m_VarUseCount.find(VD);
      if (i != m_VarUseCount.end())
      {
        ++i->second.second;
      }
    }
  }
};

//--------------------------------------------------------- 
struct ExpandVariableRefs : public StmtVisitor<ExpandVariableRefs>, StmtEditor
{
  VarDecl*  m_VarDecl;
  bool      m_bCloneInitExpr;

  ExpandVariableRefs(StmtEditor& editor, VarDecl* varDecl) : 
    StmtEditor(editor), m_VarDecl(varDecl), m_bCloneInitExpr(false)
  { 
    assert (m_VarDecl->getInit()); 
  }

  void VisitDeclRefExpr(DeclRefExpr* Node)
  {
    if (Node->getDecl() == m_VarDecl)
    {
      Expr* initExpr = m_bCloneInitExpr ? Clone_(m_VarDecl->getInit()) : m_VarDecl->getInit();
      if (UnaryOperator* UnOp = dyn_cast<UnaryOperator>(initExpr))
      {
        MemberExpr* ME;
        switch (UnOp->getOpcode()) 
        {
          case UO_AddrOf:       
            // (&x)->member  becomes  x.member 
            if ((ME = dyn_cast<MemberExpr>(getParentIgnore(Node, IG_Paren))) != 0 &&
                ME->isArrow())
            {
              ME->setArrow(false);
              initExpr = UnOp->getSubExpr();
            }
            break;

          case UO_Deref:        
            // (*x).member  becomes  x->member 
            if ((ME = dyn_cast<MemberExpr>(getParentIgnore(Node, IG_Paren))) != 0 &&
                (!ME->isArrow()))
            {
              ME->setArrow(true);
              initExpr = UnOp->getSubExpr();
            }
            break;

          default:
            break;
        }
      }
      replaceStatement(Node, Paren_(initExpr));
      m_bCloneInitExpr = true;
    }
  }
};


//--------------------------------------------------------- 
struct RemoveStmtsAfterBreakAndContinue : public StmtVisitor<RemoveStmtsAfterBreakAndContinue>, StmtEditor
{
  RemoveStmtsAfterBreakAndContinue(StmtEditor& editor) : StmtEditor(editor) {} 

  void VisitCompoundStmt(CompoundStmt* Node)
  {
    for (CompoundStmt::body_iterator i = Node->body_begin(), e = Node->body_end(); i != e; ++i)
    {
      if (isa<BreakStmt>(*i) || isa<ContinueStmt>(*i))
      {
        ++i;
        if (i != e)
        {
          llvm::SmallVector<Stmt*, 8> allStmts(Node->body_begin(), i);
          replaceStmts(Node, &allStmts[0], allStmts.size());
        }
        break;
      }
    }
  }
};

//--------------------------------------------------------- 
struct SideEffectTester : public StmtVisitor<SideEffectTester, bool>, StmtEditor
{
  SideEffectTester(StmtEditor& editor) : StmtEditor(editor) {} 

#define HAS_SIDE_EFFECT(CLASS, CONDITION)     \
bool Visit ## CLASS(CLASS *Node)  \
{                                             \
  return CONDITION;                           \
}                 

  HAS_SIDE_EFFECT(UnaryOperator, Node->isIncrementDecrementOp())
  HAS_SIDE_EFFECT(BinaryOperator, Node->isAssignmentOp())
  HAS_SIDE_EFFECT(CXXThrowExpr, true)
  HAS_SIDE_EFFECT(CXXNewExpr, true)
  HAS_SIDE_EFFECT(CXXDeleteExpr, true)

#undef HAS_SIDE_EFFECT

  bool VisitCallExpr(CallExpr* Node)
  {
    FunctionDecl* FD = 0;
    if (DeclRefExpr* fnDecl = dyn_cast<DeclRefExpr>(Node->getCallee()->IgnoreParenCasts()))
    {
      FD = dyn_cast<FunctionDecl>(fnDecl->getDecl());
    }
    return FD == 0 || !hasNoSideEffects(FD);  
    // dive-in to args done by caller 
  }
};


//--------------------------------------------------------- 
struct CompileTimeComputedExprTester : 
  public StmtVisitor<CompileTimeComputedExprTester, bool>
{
  bool VisitParenExpr(ParenExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  bool VisitImplicitCastExpr(ImplicitCastExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  bool VisitDeclRefExpr(DeclRefExpr*)
  {
    return true;
  }

  bool VisitMemberExpr(MemberExpr* Node)
  {
    return Visit(Node->getBase());
  }

  bool VisitUnaryOperator(UnaryOperator* Node)
  {
    switch (Node->getOpcode()) 
    {
      case UO_AddrOf:      
      case UO_Deref:
        return Visit(Node->getSubExpr());
      default:
        break;
    }
    return false;
  }
};

//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
Expr* fold(Expr* Node, StmtEditor& editor)
{
  if (isa<IntegerLiteral>(Node) || isa<CharacterLiteral>(Node) || 
      isa<ImaginaryLiteral>(Node) || isa<FloatingLiteral>(Node) || 
      isa<StringLiteral>(Node))
  {
    return Node;
  }

  APValue result;
  if (editor.EvaluateAsRValue_(Node, result))
  {
    if (result.isInt())
    {
      return editor.Int_(result.getInt(), Node->getType());
    }
    else if (result.isFloat())
    {
      return editor.Float_(result.getFloat(), Node->getType());
    }
  }

  bool bUpdateParents = false;
  for (Stmt::child_iterator i = Node->child_begin(), e = Node->child_end(); 
       i != e; ++i)
  {
    Stmt* folded = fold(cast<Expr>(*i), editor);
    if (*i != folded)
    {
      *i = folded;
      bUpdateParents = true;
    }
  }
  if (bUpdateParents)
  {
    editor.updateParentMap(Node);
  }
  return Node;
}

//--------------------------------------------------------- 
// generate new names for all cloned labels and all cloned declarations
void generateNamesForClones(const StmtCloneMapping& cloneMapping, StmtEditor& editor)
{
  for (StmtClone::DeclMapping::const_iterator i = cloneMapping.m_DeclMapping.begin(), 
	   e = cloneMapping.m_DeclMapping.end(); i != e; ++i)
  {
    i->second->setDeclName(DeclarationName(editor.createVariable(i->second)));
  }

  RenameLabels labelVisitor(editor);
  for (StmtClone::StmtMapping::const_iterator i = cloneMapping.m_StmtMapping.begin(), 
	   e = cloneMapping.m_StmtMapping.end(); i != e; ++i)
  {  
    labelVisitor.Visit(i->second);
  }
}

//--------------------------------------------------------- 
// rechain label refernces (from GotoStmt, AddrLabelExpr), variable references (from DeclRefExpr),
// SwitchCase lists (from SwitchStmt) to their respective clones
// Node is expected to be the clone
void rechainRefs(const StmtCloneMapping& cloneMapping, Stmt* Node)
{
  visit_df(Node, RechainRefs(cloneMapping));
}

//--------------------------------------------------------- 
Stmt* cloneStmtTreeInternal(StmtEditor& editor, Stmt* Node, StmtCloneMapping* pCloneMapping)
{
  StmtClone::Mapping cloneMap;
  if (pCloneMapping == 0)
  {
    pCloneMapping = &cloneMap;
  }
  Stmt* Node_copy = editor.Clone_(Node, pCloneMapping);
  generateNamesForClones(*pCloneMapping, editor);
  rechainRefs(*pCloneMapping, Node_copy);
  return Node_copy;
}

//--------------------------------------------------------- 
void flattenBlocks(StmtEditor& editor, Stmt* Node, bool bAllBlocks)
{
  visit_df(Node, FlattenBlocks(editor, bAllBlocks));
}

//--------------------------------------------------------- 
void removeGotosToNextLabel(StmtEditor& editor, Stmt* Node)
{
  RemoveGotosToNextLabel visitor(editor);
  visit_df(Node, visitor);
  for (RemoveGotosToNextLabel::tTouchedLabels::iterator i = visitor.m_TouchedLabels.begin(), 
       e = visitor.m_TouchedLabels.end(); i != e; ++i)
  {
    if (!i->second.m_Used)
    {
      LabelStmt* LS = i->second.LS;
      assert(LS);
      CompoundStmt* CS = dyn_cast<CompoundStmt>(editor.getParent(LS));
      if (CS && CS->size() > 1 && isa<NullStmt>(LS->getSubStmt()))
      {
        editor.removeStmt(CS, LS);
      }
      else
      {
        editor.replaceStatement(LS, LS->getSubStmt());
      }
    }
  }
}

//--------------------------------------------------------- 
void moveDeclsToFirstWrite(StmtEditor& editor, Stmt* Node)
{
  visit_df(Node, MoveDeclsToFirstWrite(editor));
}


//--------------------------------------------------------- 
void expandOneUsedInitDeclsToUse(StmtEditor& editor, Stmt* Node)
{
  CountVarUses visitor(editor);
  CompileTimeComputedExprTester tester;
  visit_df(Node, visitor);
  for (CountVarUses::tVarRefCountMap::iterator i = visitor.m_VarUseCount.begin(), 
       e = visitor.m_VarUseCount.end(); i != e; ++i)
  {
    if (i->second.second < 2 || 
        (i->first->getInit() && tester.Visit(i->first->getInit())))
    {
      CompoundStmt* CS = dyn_cast<CompoundStmt>(editor.getParent(i->second.first));
      if (CS && CS->size() > 1)
      {
        if (i->second.second > 0)
        {
          visit_df(Node, ExpandVariableRefs(editor, i->first));
          i->first->setInit(0); // ownership was transferred, so don't confuse anyone after us
        }
        editor.removeStmt(CS, i->second.first);
      }
    }
  }
}

//--------------------------------------------------------- 
void removeStmtsAfterBreakAndContinue(StmtEditor& editor, Stmt* Node)
{
  visit_df(Node, RemoveStmtsAfterBreakAndContinue(editor));
}

//--------------------------------------------------------- 
bool hasSideEffects(StmtEditor& editor, Stmt* S)
{
  SideEffectTester tester(editor);
  for (llvm::df_iterator<Stmt*> i = llvm::df_begin(S), 
       e = llvm::df_end(S); i != e; ++i)
  {
    if (tester.Visit(*i))
    {   
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------- 
bool isPromotionCast(const CastExpr* CE)
{
  QualType destType = CE->getType();
  QualType srcType = CE->getSubExpr()->getType();
  return (destType->isRealFloatingType() && srcType->isRealFloatingType()) ||
         (destType->isSignedIntegerType() && srcType->isSignedIntegerType()) ||
         (destType->isUnsignedIntegerType() && srcType->isUnsignedIntegerType());
}

//--------------------------------------------------------- 
Expr* stripParenCasts(Expr* Node)
{
  while (Node != 0)
  {
    if (CastExpr* CE = dyn_cast<CastExpr>(Node))
    {
      if (isPromotionCast(CE))
      {
        Node = CE->getSubExpr();
        continue;
      }
    }
    else if (ParenExpr* P = dyn_cast<ParenExpr>(Node))
    {
      Node = P->getSubExpr();
      continue;
    }
    break;
  }
  return Node;
}

//--------------------------------------------------------- 
bool isSimple(Expr* Node)
{
  if (isa<CallExpr>(Node))
  {
    return false;
  }
    
  Stmt::child_iterator i = Node->child_begin(), 
                       e = Node->child_end();
  if (i == e)
  {
    return true;
  }
  Node = cast<Expr>(*i++);
  return i == e && isSimple(Node);
}

//--------------------------------------------------------- 
/*
bool isAstEqual(StmtEditor& editor, Stmt* S1, Stmt* S2)
{
  PrintingPolicy printingPolicy(editor.Ctx().getLangOptions()); 
  std::string strS1, strS2;
  llvm::raw_string_ostream streamS1(strS1), streamS2(strS2);

  S1->printPretty(streamS1, 0, printingPolicy);
  S2->printPretty(streamS2, 0, printingPolicy);
  return streamS1.str() == streamS2.str();
}
*/
//--------------------------------------------------------- 
void getAstAsString(StmtEditor& editor, const Expr* S, std::string& result)
{
  result.clear();
  PrintingPolicy printingPolicy(editor.Ctx().getLangOpts()); 
  llvm::raw_string_ostream streamS(result);
  S->IgnoreParenImpCasts()->printPretty(streamS, 0, printingPolicy);
}

//--------------------------------------------------------- 
VarDecl* isFortranLoop(ForStmt* Node, const char*& errMsg)
{
  // cond must be of form "x >/</!=/ expr":
  BinaryOperator* condCmp = dyn_cast_or_null<BinaryOperator>(Node->getCond());
  if (condCmp == 0 || 
      ((!condCmp->isRelationalOp()) && condCmp->getOpcode() != BO_NE))
  {
    errMsg = "for-cond not fortran-like (must be x rel expr)";  
    return 0;
  }
  DeclRefExpr* condVar = dyn_cast<DeclRefExpr>(stripParenCasts(condCmp->getLHS()));
  if (condVar == 0 || !condVar->getType()->isIntegerType())
  {
    errMsg = "no integer for-init variable";  
    return 0;
  }
  VarDecl* VD = dyn_cast<VarDecl>(condVar->getDecl());
  if (VD == 0)
  {
    errMsg = "strange unrecognized lhs in for-condition";  
    return 0;
  }

  // inc must be of form "++x/x++":
  UnaryOperator* incStmt = dyn_cast_or_null<UnaryOperator>(Node->getInc());
  if (incStmt == 0 || (!incStmt->isIncrementOp()))
  {
    errMsg = "for-inc not fortran-like (must be ++x/x++)";  
    return 0;
  }
  DeclRefExpr* incVar = dyn_cast<DeclRefExpr>(incStmt->getSubExpr());
  if (incVar == 0 || incVar->getDecl() != VD)
  {
    errMsg = "for-inc doesn't refer to for-cond variable";
    return 0;
  }
  return VD;
}

//--------------------------------------------------------- 
Expr* selectInnerAssignResult(BinaryOperator* Node)
{
  if (isa<IntegerLiteral>(Node->getRHS()) ||
      isa<CharacterLiteral>(Node->getRHS()) ||
      isa<FloatingLiteral>(Node->getRHS()) ||
      isa<DeclRefExpr>(Node->getRHS()))
  {
    return Node->getRHS();
  }
  return Node->getLHS();
}

//--------------------------------------------------------- 
void removeInnerAssigns(StmtEditor& editor, Stmt* S)
{    
  for (stmt_iterator<BinaryOperator> i = stmt_ibegin(S), 
       e = stmt_iend(S); i != e;)
  {
    if (i->getOpcode() == BO_Assign && 
        isa<Expr>(editor.getParent(*i)))
    {
      Stmt* exprStmt = editor.getStatementOfExpression(*i);
      CompoundStmt* CS = editor.ensureCompoundParent(exprStmt);
      std::vector<Stmt*> compoundStmts(CS->child_begin(), CS->child_end());
      std::vector<Stmt*>::iterator insertPos = 
        std::find(compoundStmts.begin(), compoundStmts.end(), exprStmt);
      assert(insertPos != compoundStmts.end());
      compoundStmts.insert(insertPos, *i);
      editor.replaceStatement(*i, selectInnerAssignResult(*i));
      editor.replaceStmts(CS, &compoundStmts[0], compoundStmts.size());
      i = stmt_ibegin(S);
    }
    else
    {
      ++i;
    }
  }
}

//--------------------------------------------------------- 
struct RecordAssignUnroller : public StmtVisitor<RecordAssignUnroller>, StmtEditor
{
  RecordDecl *RD;
  Expr* LHS;
  std::vector<Stmt*> compoundStmts;

  //--------------------------------------------------------- 
  RecordAssignUnroller(StmtEditor& editor, RecordDecl* RDa, BinaryOperator* BO) :
    StmtEditor(editor),
    RD(RDa),
    LHS(BO->getLHS())
  {
    Visit(BO->getRHS());
  }

  //--------------------------------------------------------- 
  void VisitInitListExpr(InitListExpr *RHS) 
  {
    unsigned elementNo = 0;
    for (RecordDecl::field_iterator i = RD->field_begin(), 
         e = RD->field_end(); i != e; ++i)
    {
      // how to handle?
      if (i->isUnnamedBitfield())
        continue;
    
      compoundStmts.push_back(Assign_(
        MemberPoint_(Paren_(Clone_(LHS)), *i), 
        elementNo < RHS->getNumInits() ?        
          Clone_ (RHS->getInit(elementNo)) : 
          Int_(0)));

      elementNo++;
    }
  }

  //--------------------------------------------------------- 
  void VisitExpr(Expr* Node)
  {
    for (RecordDecl::field_iterator i = RD->field_begin(), 
         e = RD->field_end(); i != e; ++i)
    {
      compoundStmts.push_back(Assign_(
        MemberPoint_(Paren_(Clone_(LHS)), *i), 
        MemberPoint_(Paren_(Clone_(Node)), *i)));
    }
  }
};


//--------------------------------------------------------- 
void unrollRecordAssigns(StmtEditor& editor, Stmt* S)
{    
  const RecordType *RT;
  for (stmt_iterator<BinaryOperator> i = stmt_ibegin(S), 
       e = stmt_iend(S); i != e;)
  {
    BinaryOperator* BO = *i;
    if (BO->getOpcode() == BO_Assign && 
        (RT = BO->getType()->getAsStructureType()) != 0 && 
        editor.getStatementOfExpression(BO) == BO)  // ensures top level assign
    {
      CompoundStmt* CS = editor.ensureCompoundParent(BO);
      std::vector<Stmt*> compoundStmts(CS->child_begin(), CS->child_end());
      std::vector<Stmt*>::iterator insertPos = 
        std::find(compoundStmts.begin(), compoundStmts.end(), BO);
      assert(insertPos != compoundStmts.end());
      insertPos = compoundStmts.erase(insertPos);
      RecordAssignUnroller unroller(editor, RT->getDecl(), BO);
      compoundStmts.insert(insertPos, 
        unroller.compoundStmts.begin(), unroller.compoundStmts.end());
      editor.replaceStmts(CS, &compoundStmts[0], compoundStmts.size());
      i = stmt_ibegin(S);
    }
    else
    {
      ++i;
    }
  }
}

//--------------------------------------------------------- 
FunctionDecl* findFunctionDecl(CallExpr* expr)
{
  if (CXXMemberCallExpr* CMC = dyn_cast<CXXMemberCallExpr>(expr))
  {
    return CMC->getMethodDecl();
  }
  
  CastExpr* castExpr;
  Expr* calleeExpr = expr->getCallee();
  while ((castExpr = dyn_cast<CastExpr>(calleeExpr)) != NULL) 
  {
    calleeExpr = castExpr->getSubExpr();
  }

  DeclRefExpr* fnRef = dyn_cast<DeclRefExpr>(calleeExpr);
  return fnRef ? dyn_cast<FunctionDecl>(fnRef->getDecl()) : NULL;
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

