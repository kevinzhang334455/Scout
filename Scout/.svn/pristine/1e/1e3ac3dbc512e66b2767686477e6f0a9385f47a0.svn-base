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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_TOOLBOX_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_TOOLBOX_H

#include <functional>
#include <string>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class Stmt;
class Expr;
class CastExpr;
class CallExpr;
class VarDecl;
class ForStmt;
class FunctionDecl;

//--------------------------------------------------------- 
namespace ASTProcessing {

class StmtEditor;
struct StmtCloneMapping;


// clones the given Node and does all the common preprocessing necessary anyway
// it generates new names for all cloned labels and all cloned declarations
// and rechain label references (from GotoStmt, AddrLabelExpr), variable references (from DeclRefExpr),
// SwitchCase lists (from SwitchStmt) to their respective clones
// precondition: Node is a child of the stmt tree hold by StmtEditor
template<class T> 
inline T* cloneStmtTree(StmtEditor& editor, T* Node, StmtCloneMapping* pCloneMapping = 0)
{
  return static_cast<T*>(cloneStmtTreeInternal(editor, Node, pCloneMapping));
}

// rechain label references (from GotoStmt, AddrLabelExpr), 
// variable references (from DeclRefExpr),
// SwitchCase lists (from SwitchStmt) to their respective clones
void rechainRefs(const StmtCloneMapping& cloneMapping, Stmt* Node);

// remove all compounds (include original compounds if bAllBlocks is true) that are
// straight embedded in other compounds
// note: the lifetime of variables may change. but this should not be an issue
// if original compounds aren't touched
// precondition: Node is a child of the stmt tree hold by StmtEditor
void flattenBlocks(StmtEditor& editor, Stmt* Node, bool bAllBlocks = false);

// remove all generated gotos that jumps to the label immediately follwing the goto,
// then it removes all generated labels for which no goto exists anymore
// note: call flattenBlocks before removeGotosToNextLabel
// precondition: Node is a child of the stmt tree hold by StmtEditor
void removeGotosToNextLabel(StmtEditor& editor, Stmt* Node);

// merges generated variable declarations and first assignment to initializations
// note: call flattenBlocks before removeGotosToNextLabel
// precondition: Node is a child of the stmt tree hold by StmtEditor
void moveDeclsToFirstWrite(StmtEditor& editor, Stmt* Node);

// if generated variable declaration is used only once, the appropriate init-expr 
// is moved to that use and the variable declaration is removed
// note: call moveDeclsToFirstWrite before 
// precondition: Node is a child of the stmt tree hold by StmtEditor
void expandOneUsedInitDeclsToUse(StmtEditor& editor, Stmt* Node);

// remove all stmts after unconditional break's or continue's
// precondition: Node is a child of the stmt tree hold by StmtEditor
void removeStmtsAfterBreakAndContinue(StmtEditor& editor, Stmt* Node);

// internal function for cloneStmtTree template
Stmt* cloneStmtTreeInternal(StmtEditor& editor, Stmt* Node, StmtCloneMapping* pCloneMapping);

// fold constants
Expr* fold(Expr* Node, StmtEditor& editor);

// returns true if S has any potential side effects (doesn't look into functions)
bool hasSideEffects(StmtEditor& editor, Stmt* S);

// writes S as string in result (helpful for simple comparisions)
void getAstAsString(StmtEditor& editor, const Expr* S, std::string& result);

// a expression is simple, if it contains at most one subexpression
// which must be simple too. CallExpr is never simple.
bool isSimple(Expr* Node);

// strips promotion casts (like int<->long or float<->double)
Expr* stripParenCasts(Expr* Node);
bool isPromotionCast(const CastExpr* CE);

// returns VarDecl if condition is of the form "var >/</!=/ expr" 
// and inc is of form "++var/var++":
VarDecl* isFortranLoop(ForStmt* Node, const char*& errMsg);

// remove assign operators that are not a full expression, like
// p = q = 0; => p = 0; q = 0;
void removeInnerAssigns(StmtEditor& editor, Stmt* S);

// unroll record assignments to their fundamental type assigments
// only top level assignments are unrolled
void unrollRecordAssigns(StmtEditor& editor, Stmt* S);

// find function declarations of simple DeclRefExpr calls
// return NULL, if not found
FunctionDecl* findFunctionDecl(CallExpr* expr);


//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_TOOLBOX_H
