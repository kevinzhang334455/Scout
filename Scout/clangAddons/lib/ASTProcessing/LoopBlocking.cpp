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

#include "clang/ASTProcessing/LoopBlocking.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/ASTProcessing/DeclCollector.h"    
#include "clang/ASTProcessing/AttachedPragmas.h"    
#include "clang/AST/ASTContext.h"    
#include "llvm/ADT/DenseSet.h"

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
class LoopBlocker : StmtEditor
{
public:
  LoopBlocker(StmtEditor& editor) : StmtEditor(editor) {}

  void doBlocking(ForStmt* Node, BinaryOperator* condCmp, 
                  VarDecl* loopVar, int tileSize);
};

//--------------------------------------------------------- 
void LoopBlocker::doBlocking(ForStmt* Node, BinaryOperator* condCmp, 
                             VarDecl* loopVar, int tileSize)
{
  setIdentifierPolicy("tiled");
  DeclCollector declCollector(*this);

  Stmt* forBody = Node->getBody();
  assert(forBody);

  DeclStmt *temp = TmpVar_(Ctx().IntTy), 
           *temp_bound = TmpVar_(Ctx().IntTy),
           *i_bound = TmpVar_(Ctx().IntTy);

  declCollector.collect(temp);
  declCollector.collect(temp_bound);
  declCollector.collect(i_bound);

  Stmt* innerBody[3] = {

    // temp_bound = i_bound - i;
    Assign_(
      DeclRef_(temp_bound), 
      BinaryOp_(DeclRef_(i_bound), DeclRef_(loopVar), BO_Sub)),

    // temp_bound = temp_bound < tileSize ? temp_bound : tileSize;
    Assign_(
      DeclRef_(temp_bound), 
      Conditional_(
        BinaryOp_(DeclRef_(temp_bound), Int_(tileSize), BO_LT),
        DeclRef_(temp_bound), 
        Int_(tileSize))),

    // for(temp = 0; temp < temp_bound; ++temp) 
    For_(
      Assign_(DeclRef_(temp), Int_(0)),
      BinaryOp_(DeclRef_(temp), DeclRef_(temp_bound), BO_LT),
      UnaryOp_(DeclRef_(temp), UO_PreInc),
      forBody)
  };

  // body_{i replaced by (i+temp)}
  for (stmt_iterator<DeclRefExpr> i = stmt_ibegin(forBody), 
       e = stmt_iend(forBody); i != e; ++i)
  {
    if ((*i)->getDecl() == loopVar)
    {
      replaceStatement(*i, Paren_(BinaryOp_(DeclRef_(loopVar), DeclRef_(temp), BO_Add)));
    }
  }

  // for(stmt; i < i_bound; i += uTileSize) { innerBody }
  Node->setCond(BinaryOp_(DeclRef_(loopVar), DeclRef_(i_bound), BO_LT));
  Node->setInc(BinaryOp_(DeclRef_(loopVar), Int_(tileSize), BO_AddAssign));
  Node->setBody(Compound_(innerBody));

  // prepare expr[+1]:
  Expr* condCmpRHS = condCmp->getRHS();
  if (condCmp->getOpcode() == BO_LE)
  { 
    condCmpRHS = BinaryOp_(Paren_(condCmpRHS), Int_(1), BO_Add);
  }

  replaceStatement(Node, 
                   Compound_(Assign_(DeclRef_(i_bound), condCmpRHS), Node));

  declCollector.emitCollectedDecls();
  restoreIdentifierPolicy();
}

//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
bool blockLoop(StmtEditor& editor, ForStmt* Node, int tileSize, bool emitError)
{
  const char* pErrMsg;
  VarDecl* loopVar = isFortranLoop(Node, pErrMsg);
  if (loopVar != 0 && Node->getBody() != 0)
  {
    // we know: for (???; var rel expr; ++var)
    BinaryOperator* condCmp = cast<BinaryOperator>(Node->getCond());
    if (condCmp->getOpcode() == BO_NE ||
        condCmp->getOpcode() == BO_LT ||
        condCmp->getOpcode() == BO_LE)
    { 
      LoopBlocker(editor).doBlocking(Node, condCmp, loopVar, tileSize);
      return true;
    }
    pErrMsg = "loop blocking: for-cond has unsupported relation (must be <|<=|!=)";  
  }
  if (emitError && Node->getBody() != 0)
  {
    editor.Warn(Node, pErrMsg);
  }
  return false;
}


//--------------------------------------------------------- 
bool blockLoops(StmtEditor& editor, Stmt* Node)
{
  bool bResult = false;
  llvm::DenseSet<ForStmt*> touchedLoops;
  for (stmt_iterator<ForStmt> i = stmt_ibegin(Node), e = stmt_iend(Node); i != e;)
  {
    if (touchedLoops.insert(*i).second)
    {
      const PragmaArgumentInfo* argInfo = editor.findAttachedPragma(*i, "loop", "block");
      if (argInfo != 0)
      {
        int tileSize;
        if (!argInfo->findValueForArgument("size", editor.Ctx(), tileSize))
        {
          editor.Warn(*i, "block pragma doesn't specify tile size or tile size is 0");
        }
        else if (blockLoop(editor, *i, tileSize, true))
        {
          i = stmt_ibegin(Node);
          bResult = true;
          continue;
        }
      }
    }
    ++i;
  }
  return bResult;
}


//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

