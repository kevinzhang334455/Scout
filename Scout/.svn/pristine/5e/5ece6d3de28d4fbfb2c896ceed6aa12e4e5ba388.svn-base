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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_DECLCOLLECTOR_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_DECLCOLLECTOR_H

#include "clang/ASTProcessing/StmtEditor.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class DeclStmt;
class CompoundStmt;

//--------------------------------------------------------- 
namespace ASTProcessing {

//--------------------------------------------------------- 
class DeclCollector : StmtEditor
{
public:
  DeclCollector(StmtEditor& editor);
  ~DeclCollector(); 

  typedef llvm::SmallVector<VarDecl*, 8> tVarDecls;
  void collect(DeclStmt* Node, tVarDecls* collectedDecls = NULL);
  void collect(VarDecl* VD, tVarDecls* pCollectedDecls = NULL);

  template<class T>
  void collectAll(const T& seq);

  void emitCollectedDecls();

  // move all declarations at start of the root block
  static void makeCAst(StmtEditor& editor);

private:
  typedef llvm::SmallVector<VarDecl*, 8> tCollectedDecls;
  llvm::DenseMap<void*, tCollectedDecls> m_CollectedDecls;

  void insertCollectedDeclsAtCompoundBegin(CompoundStmt* CS);
};

//--------------------------------------------------------- inlines
inline DeclCollector::DeclCollector(StmtEditor& editor) :
  StmtEditor(editor)
{}

//--------------------------------------------------------- 
inline DeclCollector::~DeclCollector()
{
  assert(m_CollectedDecls.empty() && "call emitCollectedDecls");
}

//--------------------------------------------------------- 
template<class T>
void DeclCollector::collectAll(const T& seq)
{
  for (typename T::const_iterator i = seq.begin(), e = seq.end(); i != e; ++i)
  {
    collect(*i);
  }
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_DECLCOLLECTOR_H
