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

#include "clang/ASTProcessing/LoopSplit.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/StmtClone.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/ASTProcessing/StmtTraversal.h"    
#include "clang/AST/StmtVisitor.h"    
#include "clang/AST/DeclCXX.h"    
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

namespace {

//--------------------------------------------------------- 
bool splitAtPragmas(StmtEditor& editor, ForStmt* Node)
{
  CompoundStmt* CS = dyn_cast<CompoundStmt>(Node->getBody());
  if (CS == 0)
  {
    return false;
  }
  for (Stmt::child_iterator i = CS->child_begin(), 
       e = CS->child_end(); i != e; ++i)
  {
    if (editor.findAttachedPragma(*i, "loop", "split") != 0)
    {
      llvm::SmallVector<Stmt*, 8> beforeSplitStmts(CS->child_begin(), i);
      llvm::SmallVector<Stmt*, 8> afterSplitStmts(i, e);
      if (beforeSplitStmts.empty())
      {
        editor.Warn(*i, "split pragma at start of compound, nothing to split");
        continue;
      }

      Node->setBody(0);
      StmtCloneMapping cloneMapping;
      ForStmt* clonedFor = cloneStmtTree(editor, Node, &cloneMapping);
      Node->setBody(CS);
      editor.replaceStmts(CS, &beforeSplitStmts[0], beforeSplitStmts.size());
      CompoundStmt* splittedCompound = editor.Compound_(&afterSplitStmts[0], afterSplitStmts.size());
      rechainRefs(cloneMapping, splittedCompound);
      clonedFor->setBody(splittedCompound);      
      editor.attachComment(clonedFor, "start of splitted loop");
      editor.replaceStatement(Node, editor.Compound_(Node, clonedFor));
      editor.Note(Node, "loop splitted due to pragma");
      return true;
    }
  }
  return false;
}


//--------------------------------------------------------- 
} // anon namespace 

//--------------------------------------------------------- 
void splitLoops(StmtEditor& editor, Stmt* Node)
{
  for (stmt_iterator<ForStmt> i = stmt_ibegin(Node), 
       e = stmt_iend(Node); i != e;)
  {
    if (splitAtPragmas(editor, *i))
    {
      i = stmt_ibegin(Node);
      continue;
    }
    ++i;
  }
  flattenBlocks(editor, Node);
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

