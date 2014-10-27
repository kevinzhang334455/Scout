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

#include "clang/ASTProcessing/StmtEditor.h"
#include "clang/ASTProcessing/StmtClone.h"
#include "clang/ASTProcessing/StmtCollector.h"
#include "clang/AST/ASTContext.h"    
#include "clang/AST/APValue.h"    
#include <algorithm>

//--------------------------------------------------------- 
namespace clang {
  namespace ASTProcessing {

const SourceLocation StmtEditor::nopos;

//--------------------------------------------------------- 
ASTContext& StmtEditor::Ctx()
{return m_Collector.Ctx;
}

//--------------------------------------------------------- 
IdentifierInfo *StmtEditor::createVariable(const tOriginalNameInfo& originalSuffix)
{
  const std::string* pName = boost::get<std::string>(&originalSuffix.m_Data);
  if (pName != 0)
  {
    return m_Collector.getCurrentIdentifierPolicy().createVariable(m_Collector.Ctx, *pName);
  }
  else
  {
    return m_Collector.getCurrentIdentifierPolicy().createVariable(m_Collector.Ctx, boost::get<const NamedDecl*>(originalSuffix.m_Data));
  }
}

//--------------------------------------------------------- 
LabelDecl* StmtEditor::createLabelDecl(const LabelStmt* originalLabel)
{
  IdentifierInfo *id = m_Collector.getCurrentIdentifierPolicy().createLabel(m_Collector.Ctx, originalLabel);
  return LabelDecl::Create(Ctx(), &getFnDecl(), nopos, id);
}

//--------------------------------------------------------- 
IfStmt* StmtEditor::If_(Expr* cond, Stmt* thenStmt, Stmt* elseStmt)
{
  return m_Collector.record(new(m_Collector.Ctx)IfStmt(m_Collector.Ctx, nopos, 0, cond, thenStmt, nopos, elseStmt));
}

//--------------------------------------------------------- 
ForStmt* StmtEditor::For_(Stmt *Init, Expr *Cond, Expr *Inc, Stmt *Body)
{
  return m_Collector.record(new(m_Collector.Ctx)ForStmt(m_Collector.Ctx, Init, Cond, 0, Inc, Body, nopos, nopos, nopos));
}

//--------------------------------------------------------- 
BinaryOperator* StmtEditor::BinaryOp_(Expr* lhs, Expr* rhs, BinaryOperator::Opcode opc)
{
  if (opc >= BO_MulAssign && opc <= BO_OrAssign)
  {
    return m_Collector.record(new(m_Collector.Ctx)CompoundAssignOperator(lhs, rhs, opc, lhs->getType(), VK_RValue, OK_Ordinary, lhs->getType(), lhs->getType(), nopos, false));
  }

  QualType resultType = 
    (BinaryOperator::isComparisonOp(opc) || BinaryOperator::isLogicalOp(opc)) ? 
    Ctx().BoolTy : lhs->getType();

  return m_Collector.record(new(m_Collector.Ctx)BinaryOperator(lhs, rhs, opc, resultType, VK_RValue, OK_Ordinary, nopos, false));
}

//--------------------------------------------------------- 
Expr* StmtEditor::Conditional_(Expr *cond, Expr *lhs, Expr *rhs)
{
  return m_Collector.record(new(m_Collector.Ctx)ConditionalOperator(cond, nopos, lhs, nopos, rhs, lhs->getType(), VK_RValue, OK_Ordinary));
}

//--------------------------------------------------------- 
DeclStmt* StmtEditor::TmpVar_(QualType tmpType, Expr* init, const tOriginalNameInfo& originalVar)
{
  VarDecl* tmpVar = VarDecl_(tmpType, init, originalVar);
  return m_Collector.record(new(m_Collector.Ctx)DeclStmt(DeclGroupRef(tmpVar), nopos, nopos));
}

//--------------------------------------------------------- 
VarDecl* StmtEditor::VarDecl_(QualType tmpType, Expr* init, const tOriginalNameInfo& originalVar)
{
  VarDecl* tmpVar = VarDecl::Create(
    m_Collector.Ctx, &m_Collector.m_FnContext, nopos, nopos, createVariable(originalVar), 
    tmpType, m_Collector.Ctx.CreateTypeSourceInfo(tmpType), SC_None); 
  if (init != 0)
  {
    tmpVar->setInit(init);
  }
  return tmpVar;
}

//--------------------------------------------------------- 
DeclStmt* StmtEditor::DeclStmt_(Decl** decls, unsigned numdecls)
{
  return m_Collector.record(new(Ctx())DeclStmt(DeclGroupRef::Create(Ctx(), decls, numdecls), nopos, nopos));
}

//--------------------------------------------------------- 
CallExpr* StmtEditor::Call_(FunctionDecl* FD, Expr **args, unsigned numargs)
{
  assert(FD);
  ASTContext& ctx = m_Collector.Ctx;
  DeclRefExpr* callee = m_Collector.record(new(ctx)DeclRefExpr(FD, false, FD->getType(), VK_RValue, nopos));
  return m_Collector.record(new(ctx)CallExpr(ctx, callee, llvm::makeArrayRef(args, numargs), FD->getResultType(), VK_RValue, nopos));
}

//--------------------------------------------------------- 
DeclRefExpr* StmtEditor::DeclRef_(DeclStmt* singleVarDecl)
{
  assert(singleVarDecl->isSingleDecl() && isa<VarDecl>(singleVarDecl->getSingleDecl()));
  VarDecl* VD = static_cast<VarDecl*>(singleVarDecl->getSingleDecl());
  return DeclRef_(VD);
}

//--------------------------------------------------------- 
Expr* StmtEditor::Sizeof_(QualType type)
{
  ASTContext& ctx = m_Collector.Ctx;
  return m_Collector.record(new(ctx)UnaryExprOrTypeTraitExpr(UETT_SizeOf, 
    ctx.CreateTypeSourceInfo(type), ctx.getSizeType(), nopos, nopos));
}

//--------------------------------------------------------- 
Expr* StmtEditor::Paren_(Expr* sub)
{
  Expr* testNode = sub;
  while (ImplicitCastExpr* IC = dyn_cast<ImplicitCastExpr>(testNode))
  {
    testNode = IC->getSubExpr();
  }

  return (isa<ParenExpr>(testNode) || 
          testNode->child_begin() == testNode->child_end()) ? 
    sub : m_Collector.record(new(m_Collector.Ctx)ParenExpr(nopos, nopos, sub));
}

//--------------------------------------------------------- 
ArraySubscriptExpr* StmtEditor::ArraySubscript_(Expr *lhs, Expr *rhs)
{
  assert(rhs->getType()->isIntegerType());

  QualType elemType;
  if (const PointerType* PT = lhs->getType()->getAs<PointerType>())
  {
    elemType = PT->getPointeeType();  
  }
  else if (const Type* ET = lhs->getType()->getArrayElementTypeNoTypeQual())
  {
    elemType = QualType(ET, 0);
  }
  else
  {
    assert(0 && "could not retrieve element type information");
  }
  return m_Collector.record(new(m_Collector.Ctx)ArraySubscriptExpr(lhs, rhs, elemType, VK_RValue, OK_Ordinary, nopos));
}

//--------------------------------------------------------- 
MemberExpr* StmtEditor::MemberPoint_(Expr* base, FieldDecl* field)
{
  return m_Collector.record(new(m_Collector.Ctx)MemberExpr(base, false, field, nopos, field->getType(), VK_RValue, OK_Ordinary));
}

//--------------------------------------------------------- 
DeclRefExpr* StmtEditor::DeclRef_(ValueDecl* VD)
{
  return m_Collector.record(new(m_Collector.Ctx)DeclRefExpr(VD, false, VD->getType(), VK_RValue, nopos));
}

//--------------------------------------------------------- 
UnaryOperator* StmtEditor::UnaryOp_(Expr* sub, UnaryOperator::Opcode opc)
{
  return m_Collector.record(new(m_Collector.Ctx)UnaryOperator(sub, opc, sub->getType(), VK_RValue, OK_Ordinary, nopos));
}

//--------------------------------------------------------- 
Expr* StmtEditor::CCast_(Expr* sub, QualType castType)
{
  return m_Collector.record(CStyleCastExpr::Create(m_Collector.Ctx, castType, VK_RValue, CK_BitCast, sub, 0, 
    m_Collector.Ctx.getTrivialTypeSourceInfo(castType), nopos, nopos));
}

//--------------------------------------------------------- 
BreakStmt* StmtEditor::Break_()
{
  return m_Collector.record(new(m_Collector.Ctx)BreakStmt(nopos));
}

//--------------------------------------------------------- 
Stmt* StmtEditor::CloneInternal(Stmt* stmt, StmtCloneMapping* mapping)
{
  StmtCloneMapping defaultMapping;
  if (mapping == 0)
  {
    mapping = &defaultMapping;
  }
  Stmt* result = StmtClone(m_Collector.Ctx, mapping).Clone(stmt);
  m_Collector.recordClone(*mapping);
  m_Collector.m_ParentInfo.updateParentMap(result);
  return result;
}

//--------------------------------------------------------- 
void StmtEditor::updateParentMap(Stmt* stmt)
{
  m_Collector.m_ParentInfo.updateParentMap(stmt);
}

//--------------------------------------------------------- 
Expr* StmtEditor::Bool_(bool value)
{
  return Int_(value ? 1 : 0);
  //return m_Collector.record(new(m_Collector.Ctx)CXXBoolLiteralExpr(value, 
  //  m_Collector.Ctx.BoolTy, nopos));
}

//--------------------------------------------------------- 
Expr* StmtEditor::Int_(const llvm::APInt& value, QualType t)
{
  return m_Collector.record(IntegerLiteral::Create(m_Collector.Ctx, value, 
    QualType(t.getTypePtr(), Qualifiers::Const), nopos));
}

//--------------------------------------------------------- 
Expr* StmtEditor::Int_(int value)
{
  return Int_(llvm::APInt(m_Collector.Ctx.getIntWidth(m_Collector.Ctx.IntTy), value, true), m_Collector.Ctx.IntTy);
}

//--------------------------------------------------------- 
Expr* StmtEditor::UInt_(unsigned int value)
{
  return Int_(llvm::APInt(m_Collector.Ctx.getIntWidth(m_Collector.Ctx.UnsignedIntTy), static_cast<uint64_t>(value), false), m_Collector.Ctx.UnsignedIntTy);
}

//--------------------------------------------------------- 
Expr* StmtEditor::Float_(const llvm::APFloat& value, QualType t)
{
  return m_Collector.record(FloatingLiteral::Create(m_Collector.Ctx, value, 
    true, QualType(t.getTypePtr(), Qualifiers::Const), nopos));
}

//--------------------------------------------------------- 
Expr* StmtEditor::createScalarConst(const APValue& value, BuiltinType::Kind type)
{
  if (type >= BuiltinType::Bool && type <= BuiltinType::Int128)
  {
    if (value.isFloat())
    {
      return Int_(value.getFloat().convertToDouble());
    }
    else
    {
      return Int_(value.getInt(), BuiltinTy_(type));
    }
  }
  else
  {
    if (value.isFloat())
    {
      return Float_(value.getFloat(), BuiltinTy_(type));
    }
    else
    {
      return Float_(llvm::APFloat(type == BuiltinType::Float ? llvm::APFloat::IEEEsingle : llvm::APFloat::IEEEdouble, value.getInt()), BuiltinTy_(type));
    }
  }
}

//--------------------------------------------------------- 
Stmt* StmtEditor::LazyCompound_(Stmt** s, unsigned numStmts)
{
  assert(numStmts);
  return numStmts < 2 ? s[0] : Compound_(s, numStmts);
}

//--------------------------------------------------------- 
CompoundStmt* StmtEditor::Compound_(Stmt** s, unsigned numStmts)
{
  return m_Collector.record(new(m_Collector.Ctx)CompoundStmt(m_Collector.Ctx, llvm::makeArrayRef(s, numStmts), nopos, nopos));
}

//--------------------------------------------------------- 
CompoundStmt* StmtEditor::Compound_(Stmt* s1, Stmt* s2)
{
  Stmt* s[2] =  { s1, s2 };
  return Compound_(s, 2);
}

//--------------------------------------------------------- 
GotoStmt* StmtEditor::Goto_(LabelStmt* target)
{
  return m_Collector.record(new (m_Collector.Ctx) GotoStmt(target->getDecl(), nopos, nopos));
}

//--------------------------------------------------------- 
LabelStmt* StmtEditor::Label_(Stmt* subStmt)
{
  return m_Collector.record(new (m_Collector.Ctx) LabelStmt(nopos, createLabelDecl(0), subStmt));
}

//--------------------------------------------------------- 
NullStmt* StmtEditor::NullStmt_()
{
  return m_Collector.record(new (m_Collector.Ctx) NullStmt(nopos));
}

//--------------------------------------------------------- 
CXXFunctionalCastExpr* StmtEditor::CXXFunctionalCast_(Expr* E)
{
  return m_Collector.record(CXXFunctionalCastExpr::Create(m_Collector.Ctx, E->getType(), VK_RValue, m_Collector.Ctx.CreateTypeSourceInfo(E->getType()), CK_NoOp, E, 0, nopos, nopos));
}

//--------------------------------------------------------- 
CXXTemporaryObjectExpr* StmtEditor::CXXTemporaryObject_(CXXConstructExpr *CCE)
{
  return m_Collector.record(new (m_Collector.Ctx) CXXTemporaryObjectExpr(m_Collector.Ctx, CCE->getConstructor(), m_Collector.Ctx.CreateTypeSourceInfo(CCE->getType()), llvm::makeArrayRef(CCE->getArgs(), CCE->getNumArgs()), nopos, false, false, false));
}

//--------------------------------------------------------- 
const CanQualType& StmtEditor::BoolTy_()
{
  return m_Collector.Ctx.BoolTy;
}

//--------------------------------------------------------- 
const CanQualType& StmtEditor::BuiltinTy_(BuiltinType::Kind baseType)
{
  switch (baseType)
  {
  case BuiltinType::Void:
    return m_Collector.Ctx.VoidTy;
  case BuiltinType::Bool: 
    return m_Collector.Ctx.BoolTy;
  case BuiltinType::Char_U: 
  case BuiltinType::Char_S:
    return m_Collector.Ctx.CharTy;
  case BuiltinType::UChar:
    return m_Collector.Ctx.UnsignedCharTy;
  case BuiltinType::SChar:
    return m_Collector.Ctx.SignedCharTy;
  case BuiltinType::WChar_U:
  case BuiltinType::WChar_S:
    return m_Collector.Ctx.WCharTy;
  case BuiltinType::Char16:
    return m_Collector.Ctx.Char16Ty;
  case BuiltinType::Char32:
    return m_Collector.Ctx.Char32Ty;
  case BuiltinType::UShort:
    return m_Collector.Ctx.UnsignedShortTy;
  case BuiltinType::UInt:
    return m_Collector.Ctx.UnsignedIntTy;
  case BuiltinType::ULong:
    return m_Collector.Ctx.UnsignedLongTy;
  case BuiltinType::ULongLong:
    return m_Collector.Ctx.UnsignedLongLongTy;
  case BuiltinType::UInt128:
    return m_Collector.Ctx.UnsignedInt128Ty;
  case BuiltinType::Short:
    return m_Collector.Ctx.ShortTy;
  case BuiltinType::Int:
    return m_Collector.Ctx.IntTy;
  case BuiltinType::Long:
    return m_Collector.Ctx.LongTy;
  case BuiltinType::LongLong:
    return m_Collector.Ctx.LongLongTy;
  case BuiltinType::Int128:
    return m_Collector.Ctx.Int128Ty;
  case BuiltinType::Float:
    return m_Collector.Ctx.FloatTy;
  case BuiltinType::Double:
    return m_Collector.Ctx.DoubleTy;
  case BuiltinType::LongDouble:
    return m_Collector.Ctx.LongDoubleTy;
  default:
    assert(0 && "unsupported builtin type");
    return m_Collector.Ctx.VoidTy;
  }
}

//--------------------------------------------------------- 
const CanQualType& StmtEditor::IntPtrTy_()
{
  // FIXME: what is the right type here?
  return m_Collector.Ctx.UnsignedIntTy;
}

//--------------------------------------------------------- 
Stmt* StmtEditor::replaceStatement(Stmt* from, Stmt* newStmt)
{
  if (from == m_Collector.m_Root)
  {
    assert(from == newStmt && "never replace the root, select another root or replace compund members only");
    m_Collector.m_ParentInfo.updateParentMap(m_Collector.m_Root);
  }
  else
  {
    Stmt* parentStmt = m_Collector.m_ParentInfo.getParent(from);
    Stmt::child_iterator i = std::find(parentStmt->child_begin(), parentStmt->child_end(), from);
    if (i != parentStmt->child_end()) 
    {
      *i = newStmt;
      m_Collector.m_ParentInfo.updateParentMap(parentStmt);
    }
    else
    {
      assert(0 && "parent-child relation broken");
    }
  }
  return newStmt;
}

//--------------------------------------------------------- 
void StmtEditor::removeStmt(Stmt* stmtToRemove)
{
  Stmt* parentStmt = m_Collector.m_ParentInfo.getParent(stmtToRemove);
  if (CompoundStmt* CS = dyn_cast<CompoundStmt>(parentStmt))
  {
    removeStmt(CS, stmtToRemove);
  }
  else
  {
    replaceStatement(stmtToRemove, NullStmt_());
  }
}

//--------------------------------------------------------- 
void StmtEditor::appendStmt(CompoundStmt* S, Stmt* stmtToAppend)
{
  llvm::SmallVector<Stmt*, 8> allStmts(S->size() + 1);
  std::copy(S->body_begin(), S->body_end(), allStmts.begin());
  allStmts[S->size()] = stmtToAppend;
  replaceStmts(S, &allStmts[0], allStmts.size());
}

//--------------------------------------------------------- 
void StmtEditor::removeStmt(CompoundStmt* S, Stmt* stmtToRemove)
{
  llvm::SmallVector<Stmt*, 8> allStmts(S->size());
  std::copy(S->body_begin(), S->body_end(), allStmts.begin());
  std::remove(allStmts.begin(), allStmts.end(), stmtToRemove);
  replaceStmts(S, &allStmts[0], allStmts.size() - 1);
}

//--------------------------------------------------------- 
void StmtEditor::replaceStmts(CompoundStmt* S, Stmt **Stmts, unsigned NumStmts)
{
  S->setStmts(m_Collector.Ctx, Stmts, NumStmts);
  m_Collector.m_ParentInfo.updateParentMap(S);
}

//--------------------------------------------------------- 
CompoundStmt* StmtEditor::ensureCompoundParent(Stmt* stmt)
{
  // precondition: stmt is in a place where a compound stmt is allowed
  // postcondition: parent of stmt is a CompoundStmt
  Stmt* parent = getParent(stmt);
  CompoundStmt* compundParent;
  if ((compundParent = dyn_cast<CompoundStmt>(parent)) == NULL)
  {
    Stmt* newBlock[1] = { stmt };
    compundParent = Compound_(newBlock);
    replaceStatement(stmt, compundParent);
  }
  return compundParent;
}

//--------------------------------------------------------- 
Stmt* StmtEditor::getRoot() const
{
  return m_Collector.getRoot();
}

//--------------------------------------------------------- 
Stmt* StmtEditor::getParent(Stmt* S)
{
  return m_Collector.m_ParentInfo.getParent(S);
}

//--------------------------------------------------------- 
Stmt* StmtEditor::getParentIgnore(const Stmt* S, unsigned ignoreMask)
{
  Stmt* P = const_cast<Stmt*>(S);
  bool ignoreable;
  do {
    P = getParent(P);
    ignoreable = P != 0 && 
                 ((isa<ParenExpr>(P) && (ignoreMask & IG_Paren)) ||
                  (isa<ImplicitCastExpr>(P) && (ignoreMask & IG_ImpCasts)) ||
                  (isa<CastExpr>(P) && (ignoreMask & IG_AllCasts)));
  } while (ignoreable);
  return P;
}

//--------------------------------------------------------- 
Expr* StmtEditor::getFullExpression(Expr* subexpr)
{
  return m_Collector.m_ParentInfo.getFullExpression(subexpr);
}

//--------------------------------------------------------- 
Stmt* StmtEditor::getStatementOfExpression(Expr* subexpr)
{
  return m_Collector.m_ParentInfo.getStatementOfExpression(subexpr);
}

//--------------------------------------------------------- 
bool StmtEditor::isChildOf(const Stmt* Node, const Stmt* Parent) const
{
  return m_Collector.m_ParentInfo.isChildOf(Node, Parent);
}

//--------------------------------------------------------- 
bool StmtEditor::hasNoSideEffects(const FunctionDecl* FD)
{
  return m_Collector.hasNoSideEffects(FD);
}

//--------------------------------------------------------- 
bool StmtEditor::inlineable(const FunctionDecl* FD) const
{
  return m_Collector.inlineable(FD);
}

//--------------------------------------------------------- 
FunctionDecl& StmtEditor::getFnDecl() const
{
  return m_Collector.getFnDecl();
}

//--------------------------------------------------------- 
bool StmtEditor::isOriginalStmt(Stmt* S) const
{
  return m_Collector.isOriginalStmt(S);
}

//--------------------------------------------------------- 
void StmtEditor::attachComment(Stmt* S, const char* pComment)
{
  return m_Collector.attachComment(S, pComment);
}

//--------------------------------------------------------- 
void StmtEditor::setIdentifierPolicy(const char* commonSuffix)
{
  m_Collector.setArtificialIdentifierPolicy(ArtificialIdentifierPolicy(commonSuffix));
}

//--------------------------------------------------------- 
void StmtEditor::restoreIdentifierPolicy()
{
  m_Collector.restoreIdentifierPolicy();
}

//--------------------------------------------------------- 
const PragmaArgumentInfo* StmtEditor::findAttachedPragma(const Stmt* S, 
  const char* pDomain, const char* pAction) const
{
  return m_Collector.findAttachedPragma(S->getLocStart(), pDomain, pAction);
}

//--------------------------------------------------------- 
const PragmaArgumentInfo* StmtEditor::findAttachedPragma(const Decl* D, 
    const char* pDomain, const char* pAction) const
{
  return m_Collector.findAttachedPragma(D->getLocation(), pDomain, pAction);
}

//--------------------------------------------------------- 
DiagnosticBuilder StmtEditor::Note(const Stmt* S, const char* msg)
{
  return m_Collector.Diag(DiagnosticIDs::Note, S, msg);
}

//--------------------------------------------------------- 
ModificationNoteBuilder& StmtEditor::ModificationNote(const Stmt* S, const char* msg)
{
  return m_Collector.ModificationDiag(DiagnosticIDs::Note, S, msg);
}

//--------------------------------------------------------- 
DiagnosticBuilder StmtEditor::Warn(const Stmt* S, const char* msg)
{
  return m_Collector.Diag(DiagnosticIDs::Warning, S, msg);
}

//--------------------------------------------------------- 
const PrintingPolicy& StmtEditor::getPrintingPolicy() const
{
  return m_Collector.getPrintingPolicy();
}

//--------------------------------------------------------- 
bool StmtEditor::EvaluateNull_(Expr* E)
{
  APValue Val;
  return EvaluateAsRValue_(E, Val) && 
         Val.getKind() == APValue::Int && 
         Val.getInt() == 0;
}

//--------------------------------------------------------- 
bool StmtEditor::EvaluateAsBooleanCondition_(Expr* E, bool& b)
{
  APValue Val;
  if (EvaluateAsRValue_(E, Val))  
  {
    // just the basic stuff of clang::ExprConstant::HandleConversionToBool:
    switch (Val.getKind()) {
    case APValue::Int:
      b = Val.getInt().getBoolValue();
      return true;
    case APValue::Float:
      b = !Val.getFloat().isZero();
      return true;
    default:
      return false;
    }
  }
  return false;
}

namespace {
//--------------------------------------------------------- 
using namespace llvm;

struct EvaluateVisitor : StmtVisitor<EvaluateVisitor, APValue>
{
  ASTContext& Ctx;

  EvaluateVisitor(ASTContext& C) : Ctx(C) {}

  APValue VisitDeclRefExpr(DeclRefExpr *D) 
  {
    if (const EnumConstantDecl *ECD = dyn_cast<EnumConstantDecl>(D->getDecl())) 
    {
      llvm::APSInt Val = ECD->getInitVal();
      if (ECD->getInitVal().isSigned()
                     != D->getType()->isSignedIntegerOrEnumerationType())
      {
        Val.setIsSigned(!ECD->getInitVal().isSigned());
      }
      return APValue(Val);
    }
    return APValue();
  }

  APValue VisitCharacterLiteral(const CharacterLiteral *E) 
  { 
    assert(E->getType()->isIntegralOrEnumerationType() && 
           "unexpected literal type");
    return APValue(Ctx.MakeIntValue(E->getValue(), E->getType()));
  }

  APValue VisitIntegerLiteral(IntegerLiteral *I) { return APValue(llvm::APSInt(I->getValue())); }
  APValue VisitFloatingLiteral(FloatingLiteral *F) { return APValue(F->getValue()); }
  APValue VisitParenExpr(ParenExpr *E) { return Visit(E->getSubExpr()); }
  APValue VisitCastExpr(CastExpr *C) { return Visit(C->getSubExpr()); }
  APValue VisitUnaryPlus(const UnaryOperator *E) { return Visit(E->getSubExpr()); }
  APValue VisitUnaryMinus(const UnaryOperator *E) 
  { 
    APValue result = Visit(E->getSubExpr()); 
    if (result.isInt())
    {
      return APValue(-result.getInt());
    }
    else if (result.isFloat())
    {
      result.getFloat().changeSign();
      return result;
    }
    return APValue();
  }

  APValue Success(bool b)
  {
    return APValue(Ctx.MakeIntValue(b ? 1 : 0, Ctx.UnsignedIntTy));
  }

  APValue operateInts(const APSInt& lhs, const APSInt& rhs, BinaryOperatorKind opc)
  {
    APSInt result;
    switch (opc) {
      default: return APValue();
      case BO_Mul: result = lhs * rhs; break;
      case BO_Add: result = lhs + rhs; break;
      case BO_Sub: result = lhs - rhs; break;
      case BO_And: result = lhs & rhs; break;
      case BO_Xor: result = lhs ^ rhs; break;
      case BO_Or:  result = lhs | rhs; break;
      case BO_LT: return Success(lhs < rhs); 
      case BO_GT: return Success(lhs > rhs);
      case BO_LE: return Success(lhs <= rhs); 
      case BO_GE: return Success(lhs >= rhs); 
      case BO_EQ: return Success(lhs == rhs); 
      case BO_NE: return Success(lhs != rhs); 

      case BO_Div:
      case BO_Rem:
        if (rhs == 0) return APValue();
        result = opc == BO_Div ? lhs / rhs : lhs % rhs;
        break;
    }
    return APValue(result);
  }

  APValue operateFloats(APFloat lhs, const APFloat& rhs, BinaryOperatorKind opc)
  {
    llvm::APFloat::cmpResult CR = lhs.compare(rhs);
    switch (opc) {
      default: return APValue();
      case BO_Mul: lhs.multiply(rhs, APFloat::rmNearestTiesToEven); break;
      case BO_Add: lhs.add(rhs, APFloat::rmNearestTiesToEven); break;
      case BO_Sub: lhs.subtract(rhs, APFloat::rmNearestTiesToEven); break;
      case BO_LT: return Success(CR == APFloat::cmpLessThan);
      case BO_GT: return Success(CR == APFloat::cmpGreaterThan);
      case BO_LE: return Success(CR == APFloat::cmpLessThan || CR == APFloat::cmpEqual);
      case BO_GE: return Success(CR == APFloat::cmpGreaterThan || CR == APFloat::cmpEqual);
      case BO_EQ: return Success(CR == APFloat::cmpEqual);
      case BO_NE: return Success(CR == APFloat::cmpLessThan || CR == APFloat::cmpGreaterThan);

      case BO_Div:
      case BO_Rem:
        if (rhs.isZero()) return APValue();
        opc == BO_Div ? lhs.divide(rhs, APFloat::rmNearestTiesToEven) : 
                        lhs.remainder(rhs);
        break;
    }
    return APValue(lhs);
  }

  APValue VisitBinaryOperator(const BinaryOperator *E) 
  {
    if (E->isAssignmentOp()) return APValue();
    
    APValue lhs = Visit(E->getLHS());
    APValue rhs = Visit(E->getRHS());
    if ((lhs.isInt() || lhs.isFloat()) && (rhs.isInt() || rhs.isFloat()))
    {
      if (lhs.isInt() && rhs.isInt())
      {
        return operateInts(lhs.getInt(), rhs.getInt(), E->getOpcode());
      }
      llvm::APFloat lhsF = lhs.isFloat() ? 
        lhs.getFloat() : llvm::APFloat(double(lhs.getInt().getLimitedValue()));
      llvm::APFloat rhsF = rhs.isFloat() ? 
        rhs.getFloat() : llvm::APFloat(double(rhs.getInt().getLimitedValue()));

      return operateFloats(lhsF, rhsF, E->getOpcode());
    }
    return APValue();
  }

  APValue VisitStmt(Stmt*) { return APValue(); }
};

}

//--------------------------------------------------------- 
bool StmtEditor::EvaluateAsRValue_(Expr* E, APValue& Val)
{
  Expr::EvalResult Result;
  if(E->EvaluateAsRValue(Result, Ctx()) && !Result.HasSideEffects)
  {
    Val = Result.Val;
    return true;
  }
  return false;  

  //Val = EvaluateVisitor(Ctx()).Visit(E);
  //assert(!Val.isFloat() || !Val.getFloat().isNaN());
  //return !Val.isUninit();
}

//--------------------------------------------------------- 
Sema& StmtEditor::getSema()
{
  return m_Collector.getSema();
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // ns clang
