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

#include "clang/ASTProcessing/ReplaceExprWithStmt.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/StmtGraphTraits.h"
#include "clang/AST/CanonicalType.h"
#include "llvm/ADT/DepthFirstIterator.h"

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
class MoveTestToStmt : public StmtVisitor<MoveTestToStmt, Stmt*>, StmtEditor 
{
  Expr* top;

  void generateTest(Stmt** whereToPut, Expr* boolExpr)
  {
    // generates in whereToPut:
    //   bool temp;
    //   temp = boolExpr;
    //   if (!temp) break;
    DeclStmt* tmpVarDecl = TmpVar_(BoolTy_());
    whereToPut[0] = tmpVarDecl;
    whereToPut[1] = Assign_(DeclRef_(tmpVarDecl), boolExpr);
    whereToPut[2] = If_(UnaryOp_(DeclRef_(tmpVarDecl), UO_LNot), Break_());
  }

  void replaceContinueWithGoto(Stmt* body, LabelStmt* gotoTarget)
  {
    for (llvm::df_iterator<Stmt*> i = llvm::df_begin(body), e = llvm::df_end(body); i != e;)
    {
      if (isa<Expr>(*i) || isa<ForStmt>(*i) || isa<DoStmt>(*i) || isa<WhileStmt>(*i))
      {
        i.skipChildren();
      }
      else
      {
        if (isa<ContinueStmt>(*i))
        {
          replaceStatement(*i, Goto_(gotoTarget));   
        }
        ++i;
      }
    }
  }

public:
  MoveTestToStmt(StmtEditor& editor, Expr* t) : StmtEditor(editor), top(t) {}

  Stmt* VisitIfStmt(IfStmt* IS)
  {
    assert(IS->getCond() == top);
    // if (e) body; becomes
    // {
    //   bool temp;
    //   temp = e;
    //   if (temp) body;
    // }
    DeclStmt* tmpVarDecl = TmpVar_(BoolTy_());
    Stmt* ifBodyCompound[3] = {
      tmpVarDecl,
      Assign_(DeclRef_(tmpVarDecl), IS->getCond()),
      IS };
    IS->setCond(DeclRef_(tmpVarDecl));
    return replaceStatement(IS, Compound_(ifBodyCompound));
  }

  Stmt* VisitSwitchStmt(SwitchStmt* SS)
  {
    assert(SS->getCond() == top);
    // switch(e) body; becomes
    // { 
    //   type_{e} temp; 
    //   temp = e; 
    //   switch(temp) body; 
    // }
    DeclStmt* tmpVarDecl = TmpVar_(SS->getCond()->getType());
    Stmt* switchBodyCompound[3] = {
      tmpVarDecl,
      Assign_(DeclRef_(tmpVarDecl), SS->getCond()),
      SS };
    SS->setCond(DeclRef_(tmpVarDecl));
    return replaceStatement(SS, Compound_(switchBodyCompound));
  }

  Stmt* VisitWhileStmt(WhileStmt* WS)
  {
    assert(WS->getCond() == top);
    // while (e) body; becomes
    // while (true) {
    //   bool temp;
    //   temp = e;
    //   if (!temp) break;
    //   body;
    // }
    Stmt* whileBodyCompound[4];
    generateTest(whileBodyCompound, WS->getCond());
    whileBodyCompound[3] = WS->getBody();
    WS->setCond(Bool_(true));
    WS->setBody(Compound_(whileBodyCompound));
    return replaceStatement(WS, WS);
  }

  Stmt* VisitDoStmt(DoStmt* DS)
  {
    assert(DS->getCond() == top);
    assert(0 && "FIXME: first fix DoStmt::getSourceRange");

    // do body; while (e); 
    // {bool temp = true;
    //  do {
    //    body (with "continue" changed to "goto label";)
    //    label:
    //    temp = e;
    //  } while (temp);}

    DeclStmt* tmpVarDecl = TmpVar_(BoolTy_());
    LabelStmt* tmpLabel = Label_(Assign_(DeclRef_(tmpVarDecl), DS->getCond()));
    replaceContinueWithGoto(DS->getBody(), tmpLabel);
    DS->setCond(DeclRef_(tmpVarDecl));
    DS->setBody(Compound_(DS->getBody(), tmpLabel));
    return replaceStatement(DS, Compound_(tmpVarDecl, DS));
  }

  Stmt* VisitForStmt(ForStmt* FS)
  {
    if (FS->getInit() == top)
    {
      // for (e; x; y) body; (where e contains from) becomes
      // {
      //   e;
      //   for (; x; y) body;
      // }
      FS->setInit(NULL);
      return replaceStatement(FS, Compound_(top, FS));
    }
    else if (FS->getCond() == top)
    {
      // for (x; e; y) body; (where e contains from) becomes
      // for (x; true; y) {
      //   bool temp;
      //   temp = e;
      //   if (!temp) break;
      //   body;
      // }
      FS->setCond(Bool_(true));
      Stmt* forBodyCompound[4];
      generateTest(forBodyCompound, top);
      forBodyCompound[3] = FS->getBody();
      FS->setBody(Compound_(forBodyCompound));
      return replaceStatement(FS, FS);
    }
    else 
    {
      assert(FS->getInc() == top);
      // for (x; y; e) body; (where e contains from) becomes
      // for (x; y; ) {
      //   body (with "continue" changed to "goto label");
      //   label: e;
      // }
      FS->setInc(NULL);
      LabelStmt* tmpLabel = Label_(top);
      replaceContinueWithGoto(FS->getBody(), tmpLabel);
      FS->setBody(Compound_(FS->getBody(), tmpLabel));
      return replaceStatement(FS, FS);
    }
  }
};

//--------------------------------------------------------- 
class ShortCutGenerator : public StmtVisitor<ShortCutGenerator>, StmtEditor 
{
  Stmt* generateSequencePointReplacement(BinaryOperator* binOp, Expr* resultTarget);
  Stmt* generateConditionReplacement(ConditionalOperator* condOp, Expr* resultTarget);

public:
  ShortCutGenerator(StmtEditor& editor) : StmtEditor(editor) {}

  //--------------------------------------------------------- visitor part (not for public use)
  void VisitBinaryOperator(BinaryOperator* Node)
  {
    replaceExprWithStmt(*this, Node, std::bind(&ShortCutGenerator::generateSequencePointReplacement, this, Node, _1));
  }

  void VisitConditionalOperator(ConditionalOperator* Node)
  {
    replaceExprWithStmt(*this, Node, std::bind(&ShortCutGenerator::generateConditionReplacement, this, Node, _1));
  }

  void VisitStmt(Stmt*) { assert(0 && "unknown stmt class to break up"); }
};

//--------------------------------------------------------- 
Stmt* ShortCutGenerator::generateSequencePointReplacement(BinaryOperator* binOp, Expr* resultTarget)
{
  if (binOp->getOpcode() == BO_LAnd)
  {
    // resultTarget = a && b -> if (a) resultTarget = b; else resultTarget = false;
    return resultTarget ? If_(binOp->getLHS(), 
                              Assign_(resultTarget, binOp->getRHS()), 
                              Assign_(Clone_(resultTarget), Bool_(false)))
                        : If_(binOp->getLHS(), 
                              binOp->getRHS());
  }
  else if (binOp->getOpcode() == BO_LOr)
  {
    // resultTarget = a || b -> if (a) resultTarget = true; else resultTarget = b;
    return resultTarget ? If_(binOp->getLHS(), 
                              Assign_(resultTarget, Bool_(true)), 
                              Assign_(Clone_(resultTarget), binOp->getRHS()))
                        : If_(binOp->getLHS(), 
                              NullStmt_(),
                              binOp->getRHS());


  }
  else
  {
    assert(binOp->getOpcode() == BO_Comma && "the given operator is not a sequence point");
    // resultTarget = a, b -> { a; resultTarget = b; }
    return resultTarget ? Compound_(binOp->getLHS(), Assign_(resultTarget, binOp->getRHS())) 
                        : Compound_(binOp->getLHS(), binOp->getRHS());
  }
}

//--------------------------------------------------------- 
Stmt* ShortCutGenerator::generateConditionReplacement(ConditionalOperator* condOp, Expr* resultTarget)
{
  return resultTarget ? If_(condOp->getCond(), 
                            Assign_(resultTarget, condOp->getTrueExpr()), 
                            Assign_(Clone_(resultTarget), condOp->getFalseExpr())) :
                        If_(condOp->getCond(), 
                            condOp->getTrueExpr(), 
                            condOp->getFalseExpr());
}

//--------------------------------------------------------- 
class ReplaceExpression : StmtEditor
{
  Stmt* replaceSubexpressionWithStatement(Expr* from, const std::function<Stmt*(Expr*)>& fnGenerator);
  BinaryOperator* splitExpression(Expr* from, Stmt* stmt);
  bool breakShortcutExpressions(Expr* from);

public:
  ReplaceExpression(StmtEditor& editor);

  Stmt* doReplace(Expr* from, const std::function<Stmt*(Expr*)>& fnGenerator);
};

//--------------------------------------------------------- 
ReplaceExpression::ReplaceExpression(StmtEditor& editor) :
  StmtEditor(editor)
{}

//--------------------------------------------------------- 
Stmt* ReplaceExpression::doReplace(Expr* from, const std::function<Stmt*(Expr*)>& fnGenerator)
{
  Stmt* result = NULL;
  Expr* top = getFullExpression(from);
  Stmt* stmt = getStatementOfExpression(top);
  if (stmt != top && getParent(top) != stmt)
  {
    // this is a decl stmt in a for,while or if init-expr
    DeclStmt* decl = dyn_cast<DeclStmt>(getParent(top));
    assert(decl && getParent(decl) == stmt);
    assert(isa<ForStmt>(stmt) || isa<DoStmt>(stmt) || isa<WhileStmt>(stmt) || isa<IfStmt>(stmt));
    if (ForStmt* FS = dyn_cast<ForStmt>(stmt))
    {
      assert(FS->getInit() == decl);
      // for (e; x; y) body; (where e contains from) becomes
      // {
      //   e;
      //   for (; x; y) body;
      // }
      FS->setInit(NULL);
      result = replaceStatement(FS, Compound_(decl, FS));
    }
    else
    {
      assert(0 && "FIXME: to do");
    }
  }
  else
  {
    result = MoveTestToStmt(*this, top).Visit(stmt);
  }
  Stmt* subExprStmt = replaceSubexpressionWithStatement(from, fnGenerator);
  return result ? result : subExprStmt;
}

//--------------------------------------------------------- 
Stmt* ReplaceExpression::replaceSubexpressionWithStatement(Expr* from, const std::function<Stmt*(Expr*)>& fnGenerator)
{
  // precondition: from is part of a statement where a CompoundStmt is allowed
  Expr* top = getFullExpression(from);
  Stmt* stmt = getStatementOfExpression(top);
  if (Expr::classof(stmt))
  {
    BinaryOperator* binOp;
    assert(stmt == top && "if a statement is an expression, it should be a full expression");
    if (top == from)
    {
      // expression result is ignored 
      return replaceStatement(from, fnGenerator(0));
    }
    else if ((binOp = dyn_cast<BinaryOperator>(top)) != NULL && 
             binOp ->getOpcode() == BO_Assign &&
             binOp->getRHS() == from)
    {
      // the other simple case: erase the complete asssigment expression and instead put the statement in that place
      return replaceStatement(binOp, fnGenerator(binOp->getLHS()));
    }
  }
  BinaryOperator* assignExpr = splitExpression(from, stmt);
  assert(assignExpr->getRHS() == from && "split expression should deliver the simple case");
  return replaceStatement(assignExpr, fnGenerator(assignExpr->getLHS()));
}


//--------------------------------------------------------- 
BinaryOperator* ReplaceExpression::splitExpression(Expr* from, Stmt* stmt)
{
  // precondition: from is a (maybe indirect) child of stmt, stmt is in a place where a compound stmt is allowed
  // postcondition: parent of stmt is a CompoundStmt
  // postcondition: stmt_{from}; -> type_{from} tmpvar; tmpvar = from; stmt_{tmpvar}; 
  // returns: tmpvar = from; 
  CompoundStmt* parentCompound = ensureCompoundParent(stmt);
  if (breakShortcutExpressions(from))
  {
    stmt = getStatementOfExpression(from);
    parentCompound = ensureCompoundParent(stmt);
  }

  QualType tmpType = from->getType();
  tmpType.removeLocalConst();
  DeclStmt* tmpDecl = TmpVar_(tmpType);
  BinaryOperator* tmpAssign = Assign_(DeclRef_(tmpDecl), from); 

  llvm::SmallVector<Stmt*, 8> stmtCompound(parentCompound->size() + 2);
  llvm::SmallVector<Stmt*, 8>::iterator insertIter = stmtCompound.begin();
  for (CompoundStmt::body_iterator i = parentCompound->body_begin(), e = parentCompound->body_end(); i != e; ++i)
  {
    if (*i == stmt)
    {
      replaceStatement(from, DeclRef_(tmpDecl));    // stmt_{from} -> stmt_{tmpvar}; 
      *insertIter++ = tmpDecl;
      *insertIter++ = tmpAssign;
    }
    *insertIter++ = *i;
  }
  replaceStmts(parentCompound, &stmtCompound[0], stmtCompound.size());
  return tmpAssign;
}

//--------------------------------------------------------- 
bool ReplaceExpression::breakShortcutExpressions(Expr* from)
{
  llvm::SmallVector<Expr*, 8> ancestors;
  for (Expr* parent = dyn_cast<Expr>(getParent(from)); parent != 0; from = parent, parent = dyn_cast<Expr>(getParent(from)))
  {
    BinaryOperator* binOp = dyn_cast<BinaryOperator>(parent);
    if (binOp != 0 &&
        binOp->getLHS() != from &&       
        (binOp->getOpcode() == BO_LAnd || 
         binOp->getOpcode() == BO_LOr  || 
         binOp->getOpcode() == BO_Comma))
    {
      ancestors.push_back(binOp);
    }
    ConditionalOperator* condOp = dyn_cast<ConditionalOperator>(parent);
    if (condOp != 0 &&
        condOp->getCond() != from)       
    {
      ancestors.push_back(condOp);
    }
  }
  for (llvm::SmallVector<Expr*, 8>::reverse_iterator i = ancestors.rbegin(), e = ancestors.rend(); i != e; ++i)
  {
    ShortCutGenerator(*this).Visit(*i);
  }      
  return !ancestors.empty();
}

//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
Stmt* replaceExprWithStmt(StmtEditor& editor, Expr* from, const std::function<Stmt*(Expr*)>& fnGenerator)
{
  return ReplaceExpression(editor).doReplace(from, fnGenerator);
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

