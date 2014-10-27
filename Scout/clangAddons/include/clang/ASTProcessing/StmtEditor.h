//===--- Cleanup.h - "Umbrella" header for AST library ----------*- C++ -*-===//
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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_STMTEDITOR_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_STMTEDITOR_H

#include <boost/variant.hpp>
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class DiagnosticBuilder;
class Sema;

namespace ASTProcessing {

struct StmtCloneMapping;
class StmtCollector;
class PragmaArgumentInfo;
class ModificationNoteBuilder;

// StmtEditor provides all the basic means needed to manipulate statement trees.
// You create a StmtEditor either by supplying a StmtCollector or by simply
// copiing another StmtEditor (which is cheap). The best way to use StmtEditor 
// is to derive from it and define a ctor in the derived class which takes an 
// existing StmtEditor. See the replaceExpressionWithStatement implementation
// for an example.
class StmtEditor
{
  Stmt* CloneInternal(Stmt*, StmtCloneMapping*);

public:
  struct tOriginalNameInfo
  {
    boost::variant<std::string, const NamedDecl*> m_Data;
    tOriginalNameInfo() : m_Data((const NamedDecl*)0) {}
    tOriginalNameInfo(const std::string& name) : m_Data(name) {}
    tOriginalNameInfo(const NamedDecl* decl) : m_Data(decl) {}
  };

protected:
  StmtCollector&  m_Collector;

public:
  explicit StmtEditor(StmtCollector& collector);

  static const SourceLocation nopos;
  ASTContext& Ctx();

  // create variable names
  IdentifierInfo *createVariable(const tOriginalNameInfo& originalSuffix);
  LabelDecl* createLabelDecl(const LabelStmt* originalLabel);

  // stmt creation
  IfStmt* If_(Expr* cond, Stmt* thenStmt, Stmt* elseStmt = 0);
  ForStmt* For_(Stmt *Init, Expr *Cond, Expr *Inc, Stmt *Body);
  GotoStmt* Goto_(LabelStmt* target);
  LabelStmt* Label_(Stmt* subStmt);
  NullStmt* NullStmt_();
  BreakStmt* Break_();

  // expr creation
  BinaryOperator* BinaryOp_(Expr* lhs, Expr* rhs, BinaryOperator::Opcode opc);
  UnaryOperator* UnaryOp_(Expr* sub, UnaryOperator::Opcode opc);
  BinaryOperator* Assign_(Expr* lhs, Expr* rhs);
  BinaryOperator* Add_(Expr* lhs, Expr* rhs);
  DeclRefExpr* DeclRef_(DeclStmt* singleVarDecl);
  DeclRefExpr* DeclRef_(ValueDecl* VD);

  // literals:
  Expr* Bool_(bool value);
  Expr* Int_(const llvm::APInt& value, QualType t);
  Expr* Int_(int value);              // simple 32 bit integer
  Expr* UInt_(unsigned int value); 
  Expr* Float_(const llvm::APFloat& value, QualType t);
  Expr* createScalarConst(const APValue& value, BuiltinType::Kind type);

  Expr* Sizeof_(QualType type);

  Expr* CCast_(Expr* sub, QualType castType);
  Expr* Conditional_(Expr *cond, Expr *lhs, Expr *rhs);
  ArraySubscriptExpr* ArraySubscript_(Expr *lhs, Expr *rhs);
  Expr* Paren_(Expr* sub);
  MemberExpr* MemberPoint_(Expr* base, FieldDecl* field);
  DeclStmt* DeclStmt_(Decl** decls, unsigned numdecls);
  CallExpr* Call_(FunctionDecl* FD, Expr **args, unsigned numargs);
  template<unsigned N>
  CallExpr* Call_(FunctionDecl* FD, Expr *(&args)[N]);

  // temp variable creation
  DeclStmt* TmpVar_(QualType tmpType, Expr* init = 0, const tOriginalNameInfo& originalVar = tOriginalNameInfo());
  VarDecl* VarDecl_(QualType tmpType, Expr* init = 0, const tOriginalNameInfo& originalVar = tOriginalNameInfo());

  // C++ support:
  CXXFunctionalCastExpr* CXXFunctionalCast_(Expr* E);
  CXXTemporaryObjectExpr* CXXTemporaryObject_(CXXConstructExpr *CCE);

  // compound stmt creation
  CompoundStmt* Compound_(Stmt* s1, Stmt* s2);
  CompoundStmt* Compound_(Stmt** s, unsigned numStmts);
  Stmt* LazyCompound_(Stmt** s, unsigned numStmts); // compund only numStmts > 1
  template<unsigned N>
  CompoundStmt* Compound_(Stmt* (&s)[N]);

  // types
  const CanQualType& BoolTy_();
  const CanQualType& IntPtrTy_();  // returns intptr_t for casts from void* to int
  const CanQualType& BuiltinTy_(BuiltinType::Kind baseType);

  // compound stmt editing
  void appendStmt(CompoundStmt* S, Stmt* stmtToAppend);
  void removeStmt(CompoundStmt* S, Stmt* stmtToRemove);
  void replaceStmts(CompoundStmt* S, Stmt **Stmts, unsigned NumStmts);

  // precondition: stmt is in a place where a compound stmt is allowed
  // postcondition: parent of stmt is a CompoundStmt
  CompoundStmt* ensureCompoundParent(Stmt* stmt);
  
  // stmt tree deep copy
  template<class T>
  T* Clone_(T* stmt, StmtCloneMapping* mapping = 0);

  // replaces from in the parent with newStmt, returns newStmt
  Stmt* replaceStatement(Stmt* from, Stmt* newStmt);

  // removes stmtToRemove from the parent 
  // only applicable if parent can be an CompoundStmt or NullStmt
  void removeStmt(Stmt* stmtToRemove);

  // delegates from parent map
  Stmt* getRoot() const;
  Stmt* getParent(Stmt* s);

  enum { IG_Paren = 0x01, IG_ImpCasts = 0x02, IG_AllCasts = 0x04 };
  Stmt* getParentIgnore(const Stmt* S, unsigned ignoreMask);
  Expr* getFullExpression(Expr* subexpr);
  Stmt* getStatementOfExpression(Expr* subexpr);
  bool isChildOf(const Stmt* Node, const Stmt* Parent) const;

  FunctionDecl& getFnDecl() const;

  // needed only, if stmt was never inserted in the tree before:
  void updateParentMap(Stmt* stmt);

  bool hasNoSideEffects(const FunctionDecl* FD);
  bool inlineable(const FunctionDecl* FD) const;

  // delegates from StmtCollector
  bool isOriginalStmt(Stmt* S) const;
  void attachComment(Stmt* S, const char* pComment);
  void setIdentifierPolicy(const char* commonSuffix);
  void restoreIdentifierPolicy();

  // pragma handling
  const PragmaArgumentInfo* findAttachedPragma(const Stmt* S, 
    const char* pDomain, const char* pAction) const;
  const PragmaArgumentInfo* findAttachedPragma(const Decl* D, 
    const char* pDomain, const char* pAction) const;

  DiagnosticBuilder Note(const Stmt* S, const char* msg);
  DiagnosticBuilder Warn(const Stmt* S, const char* msg);
  ModificationNoteBuilder& ModificationNote(const Stmt* S, const char* msg);
  const PrintingPolicy& getPrintingPolicy() const;

  // Evaluation always without side effects:
  bool EvaluateNull_(Expr* E);  // returns true, if 0, false otherwise (!= 0 or not evalutable)
  bool EvaluateAsBooleanCondition_(Expr* E, bool&);
  bool EvaluateAsRValue_(Expr* E, APValue&);

  Sema& getSema();
};


//--------------------------------------------------------- inlines
inline StmtEditor::StmtEditor(StmtCollector& collector) :
  m_Collector(collector)
{}

//--------------------------------------------------------- 
template<unsigned N>
inline CompoundStmt* StmtEditor::Compound_(Stmt* (&s)[N])
{
  return Compound_(s, N);
}

//--------------------------------------------------------- 
template<unsigned N>
inline CallExpr* StmtEditor::Call_(FunctionDecl* FD, Expr *(&args)[N])
{
  return Call_(FD, args, N);
}

//--------------------------------------------------------- 
template<class T>
inline T* StmtEditor::Clone_(T* stmt, StmtCloneMapping* mapping)
{
  return static_cast<T*>(CloneInternal(stmt, mapping));
}

//--------------------------------------------------------- 
inline BinaryOperator* StmtEditor::Assign_(Expr* lhs, Expr* rhs)
{
  return BinaryOp_(lhs, rhs, BO_Assign);
}

//--------------------------------------------------------- 
inline BinaryOperator* StmtEditor::Add_(Expr* lhs, Expr* rhs)
{
  return BinaryOp_(lhs, rhs, BO_Add);
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_STMTEDITOR_H
