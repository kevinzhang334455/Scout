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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_STMTCLONE_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_STMTCLONE_H

#include "clang/AST/StmtVisitor.h"
#include "llvm/ADT/DenseMap.h"

//--------------------------------------------------------- 
namespace clang {
  namespace ASTProcessing {

//--------------------------------------------------------- forwards
struct StmtCloneMapping;

//--------------------------------------------------------- 
class StmtClone : public StmtVisitor<StmtClone, Stmt*> 
{
public:
  // first: original stmt, second: appropriate cloned stmt
  typedef llvm::DenseMap<Stmt*, Stmt*> StmtMapping; 
  typedef llvm::DenseMap<ValueDecl*, ValueDecl*> DeclMapping; 
  typedef StmtCloneMapping Mapping; 

private:  
  ASTContext& Ctx;
  Mapping*    m_OriginalToClonedStmts;

  Decl* CloneDecl(Decl* Node);
  VarDecl* CloneDeclOrNull(VarDecl* Node);

public:
  StmtClone(ASTContext& ctx, Mapping* originalToClonedStmts = 0);

  template<class StmtTy>
  StmtTy* Clone(StmtTy* S);

  //--------------------------------------------------------- visitor part (not for public use)
  // Stmt.def could be used if ABSTR_STMT is introduced
#define DECLARE_CLONE_FN(CLASS) Stmt* Visit ## CLASS(CLASS *Node);
  DECLARE_CLONE_FN(BinaryOperator)
  DECLARE_CLONE_FN(UnaryOperator)
  DECLARE_CLONE_FN(ReturnStmt)
  DECLARE_CLONE_FN(GotoStmt)
  DECLARE_CLONE_FN(IfStmt)
  DECLARE_CLONE_FN(ForStmt)
  DECLARE_CLONE_FN(NullStmt)
  DECLARE_CLONE_FN(LabelStmt)
  DECLARE_CLONE_FN(CompoundStmt)
  DECLARE_CLONE_FN(DeclRefExpr)
  DECLARE_CLONE_FN(DeclStmt)
  DECLARE_CLONE_FN(IntegerLiteral)
  DECLARE_CLONE_FN(SwitchStmt)
  DECLARE_CLONE_FN(CaseStmt)
  DECLARE_CLONE_FN(DefaultStmt)
  DECLARE_CLONE_FN(WhileStmt)
  DECLARE_CLONE_FN(DoStmt)
  DECLARE_CLONE_FN(ContinueStmt)
  DECLARE_CLONE_FN(BreakStmt)
  DECLARE_CLONE_FN(CXXCatchStmt)
  DECLARE_CLONE_FN(CXXTryStmt)
  DECLARE_CLONE_FN(PredefinedExpr)
  DECLARE_CLONE_FN(CharacterLiteral)
  DECLARE_CLONE_FN(FloatingLiteral)
  DECLARE_CLONE_FN(ImaginaryLiteral)
  DECLARE_CLONE_FN(StringLiteral)
  DECLARE_CLONE_FN(ParenExpr)
  DECLARE_CLONE_FN(ArraySubscriptExpr)
  DECLARE_CLONE_FN(MemberExpr)
  DECLARE_CLONE_FN(CompoundLiteralExpr)
  DECLARE_CLONE_FN(ImplicitCastExpr)
  DECLARE_CLONE_FN(CStyleCastExpr)
  DECLARE_CLONE_FN(CompoundAssignOperator)
  DECLARE_CLONE_FN(ConditionalOperator)
  DECLARE_CLONE_FN(InitListExpr)
  DECLARE_CLONE_FN(DesignatedInitExpr)
  DECLARE_CLONE_FN(AddrLabelExpr)
  DECLARE_CLONE_FN(StmtExpr)
  DECLARE_CLONE_FN(ChooseExpr)
  DECLARE_CLONE_FN(GNUNullExpr)
  DECLARE_CLONE_FN(VAArgExpr)
  DECLARE_CLONE_FN(ImplicitValueInitExpr)
  DECLARE_CLONE_FN(ExtVectorElementExpr)
  DECLARE_CLONE_FN(UnaryExprOrTypeTraitExpr)
  DECLARE_CLONE_FN(CallExpr)
  DECLARE_CLONE_FN(ShuffleVectorExpr)
  DECLARE_CLONE_FN(CXXOperatorCallExpr)
  DECLARE_CLONE_FN(CXXMemberCallExpr)
  DECLARE_CLONE_FN(CXXStaticCastExpr)
  DECLARE_CLONE_FN(CXXDynamicCastExpr)
  DECLARE_CLONE_FN(CXXReinterpretCastExpr)
  DECLARE_CLONE_FN(CXXConstCastExpr)
  DECLARE_CLONE_FN(CXXFunctionalCastExpr)
  DECLARE_CLONE_FN(CXXBoolLiteralExpr)
  DECLARE_CLONE_FN(CXXNullPtrLiteralExpr)
  DECLARE_CLONE_FN(CXXThisExpr)
  DECLARE_CLONE_FN(CXXThrowExpr)
  DECLARE_CLONE_FN(CXXConstructExpr)
  DECLARE_CLONE_FN(CXXTemporaryObjectExpr)
  DECLARE_CLONE_FN(MaterializeTemporaryExpr)
  DECLARE_CLONE_FN(BinaryTypeTraitExpr)

  Stmt* VisitStmt(Stmt*);
};

//--------------------------------------------------------- 
// not a StmtClone member class to make it forwardable:
struct StmtCloneMapping
{
  StmtClone::StmtMapping m_StmtMapping; 
  StmtClone::DeclMapping m_DeclMapping;
};


//--------------------------------------------------------- inlines
inline StmtClone::StmtClone(ASTContext& ctx, Mapping* originalToClonedStmts) : 
  Ctx(ctx), 
  m_OriginalToClonedStmts(originalToClonedStmts) 
{}

//--------------------------------------------------------- 
template<class StmtTy>
StmtTy* StmtClone::Clone(StmtTy* S)
{
  if (S == NULL)
  {
    return NULL;
  }

  Stmt* cloned_Stmt = Visit(S);
  if (m_OriginalToClonedStmts != 0)
  {
    m_OriginalToClonedStmts->m_StmtMapping[S] = cloned_Stmt;
  }
  return static_cast<StmtTy*>(cloned_Stmt);
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_STMTCLONE_H
