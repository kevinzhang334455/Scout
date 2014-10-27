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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_STMTCOLLECTOR_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_STMTCOLLECTOR_H

#include "llvm/ADT/DenseSet.h"
#include "clang/ASTProcessing/ParentInfo.h"
#include "clang/ASTProcessing/ArtificialIdentifierPolicy.h"
#include "clang/ASTProcessing/ModificationNoteBuilder.h"
#include "clang/ASTProcessing/StmtTraversal.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/AST/PrettyPrinter.h"
#include <functional>
#include <memory>
#include <list>
#include <map>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class ASTContext;
class Stmt;
class FunctionDecl;
class Rewriter;
class CallExpr;
class CompilerInstance;
class Sema;

namespace ASTProcessing {

class StmtEditor;
class PragmaArgumentInfo;
class StmtCollector;
struct StmtCloneMapping;
class CommentPrinter;

//--------------------------------------------------------- 
// StmtCollector is the root class used by (perhaps multiple) 
// StmtEditors when manipulating a particular stmt tree. It 
// tracks all the changes in the tree and after you are finished
// it provides the means to rewrite the changes and collect the garbage
class StmtCollector
{
public:
  class LinkedPrinterHelper : public PrinterHelper
  {
  public:
    void setNextPrinterHelper(LinkedPrinterHelper* lph); // takes ownership
    virtual bool handledStmt(Stmt* E, llvm::raw_ostream& OS, unsigned IndentLevel);
  private:
    llvm::OwningPtr<LinkedPrinterHelper> m_Next;
  };

  void setNextPrinterHelper(LinkedPrinterHelper* lph); // takes ownership


  struct tLineLocation
  {
    PresumedLoc m_Loc;
    explicit tLineLocation(const PresumedLoc& L) : m_Loc(L) {}
    bool operator<(const tLineLocation& other) const;
  };

protected:
  CompilerInstance&     m_CI;
  Stmt*                 m_Root;           // placed at top for c'tor purposes 
  ASTContext&           Ctx;
  FunctionDecl&         m_FnContext;
  llvm::DenseSet<Stmt*> m_AllNodes, m_OriginalNodes, m_CreatedNodes;
  ParentInfo            m_ParentInfo;
  llvm::SmallVector<ArtificialIdentifierPolicy, 4> m_IdentifierPolicy;
  DiagnosticsEngine&    m_Diags;
  std::list<ModificationNoteBuilder>  m_ModificationNotes;
  PrintingPolicy        m_PrintingPolicy; 
  typedef llvm::DenseMap<const Stmt*, const Stmt*> tCloneMap;
  tCloneMap             m_ClonedNodes;   // first: cloned stmt, second: original stmt

  struct tStmtTreeMirror
  {
    typedef std::shared_ptr<tStmtTreeMirror> tSharedPtr;
    typedef llvm::SmallVector<tSharedPtr, 8>      tChildrenVector;

    Stmt*             m_OriginalStmt;
    tStmtTreeMirror*  m_OriginalParent;
    SourceRange       m_SourceRange;
    tChildrenVector   m_Children;
    explicit tStmtTreeMirror(Stmt* s, tStmtTreeMirror* parent);

    bool remainedEqual() const; 
    bool remainedEqualToClone(const tCloneMap& cloneMap, const Stmt* cloneStmt) const;
    static tChildrenVector::const_iterator find(tChildrenVector::const_iterator b, tChildrenVector::const_iterator e, Stmt*S);
    const tStmtTreeMirror* find_recursively(const Stmt*S) const;
  };
  tStmtTreeMirror       m_OriginalTree;
  
  CommentPrinter* m_CommentCollector;

  llvm::OwningPtr<LinkedPrinterHelper>  m_PrinterHelper;
  void attachComment(Stmt* S, const char* pComment);

  struct tPragmaInfo
  {
    const IdentifierInfo* m_pAction;
    std::string           m_Domain;
    tLineLocation         m_Location;

    tPragmaInfo(const IdentifierInfo* A, const llvm::StringRef& D, 
                const tLineLocation& L);
    bool operator<(const tPragmaInfo& other) const;
  };    

  typedef std::map<tPragmaInfo, const PragmaArgumentInfo*> tPragmaMap;  
  tPragmaMap  m_PragmaMap;

  virtual bool hasNoSideEffects(const FunctionDecl* FD) const;
  virtual bool inlineable(const FunctionDecl* FD) const;

  template<class T>
  T* record(T* Node);     //return Node;

  void recordClone(const StmtCloneMapping& mapping);

  // a very simple garbage collector for AST processing: when calling gc(), 
  // all recorded Stmts, which are not in the root tree (anymore) are destroyed
  void gc();

  void emitModificationNotes(Rewriter& rewriter);
  const PrintingPolicy& getPrintingPolicy() const;

  StmtCollector(const StmtCollector&);            // not defined
  StmtCollector& operator=(const StmtCollector&); // not defined

public:
  // the fnContext contains the function defines of the processed Stmt tree
  // precondition: fnContext.getBody(Ctx) != 0
  explicit StmtCollector(CompilerInstance& compiler, 
                         const ArtificialIdentifierPolicy& identifierPolicy, 
                         FunctionDecl& fnContext);
  // calls gc:
  ~StmtCollector();

  void setArtificialIdentifierPolicy(const ArtificialIdentifierPolicy& identifierPolicy);
  void restoreIdentifierPolicy();
  const ArtificialIdentifierPolicy& getCurrentIdentifierPolicy() const;

  void rewrite(Rewriter& rewriter);

  Stmt* getRoot() const;
  FunctionDecl& getFnDecl() const;

  void attachPragma(const tLineLocation& loc, 
                    llvm::StringRef domainName, 
                    const IdentifierInfo* actionName, 
                    const PragmaArgumentInfo* arguments);
  const PragmaArgumentInfo* findAttachedPragma(SourceLocation loc, const char* pDomain, const char* pAction) const;
  bool hasAnyPragmas() const;

  // returns true, if S is part of the original stmt tree (useful in order to 
  // prevent changes of not-generated AST parts:
  bool isOriginalStmt(Stmt* S) const;

  DiagnosticBuilder Diag(DiagnosticIDs::Level l, const Stmt* S, const char* msg);
  ModificationNoteBuilder& ModificationDiag(DiagnosticIDs::Level l, const Stmt* S, const char* msg);


  const CanQualType& getTypeFromChar(char fnArg) const;
  bool typeCharMatch(const QualType& type, char fnArg) const;
  bool argumentsMatch(FunctionDecl* FD, const char* fnArgs);
  LinkedPrinterHelper* getPrinterHelper() const;

  Sema& getSema();

  friend class StmtEditor;
  friend class CommentPrinter; 
};


//--------------------------------------------------------- inlines
template<class T>
inline T* StmtCollector::record(T* Node)
{
  m_AllNodes.insert(Node);
  m_CreatedNodes.insert(Node);
  return Node;
}

//--------------------------------------------------------- 
inline bool StmtCollector::isOriginalStmt(Stmt* S) const
{
  return !m_CreatedNodes.count(S);
}

//--------------------------------------------------------- 
inline void StmtCollector::setArtificialIdentifierPolicy(const ArtificialIdentifierPolicy& identifierPolicy)
{
  m_IdentifierPolicy.push_back(identifierPolicy);
}

//--------------------------------------------------------- 
inline void StmtCollector::restoreIdentifierPolicy()
{
  m_IdentifierPolicy.pop_back();
  assert(!m_IdentifierPolicy.empty());
}

//--------------------------------------------------------- 
inline const ArtificialIdentifierPolicy& StmtCollector::getCurrentIdentifierPolicy() const
{
  assert(!m_IdentifierPolicy.empty());
  return m_IdentifierPolicy.back();
}

//--------------------------------------------------------- 
inline Stmt* StmtCollector::getRoot() const
{
  return m_Root;
}

//--------------------------------------------------------- 
inline FunctionDecl& StmtCollector::getFnDecl() const
{
  return m_FnContext;
}

//--------------------------------------------------------- 
inline bool StmtCollector::hasAnyPragmas() const
{
  return !m_PragmaMap.empty();
}

//--------------------------------------------------------- 
inline StmtCollector::LinkedPrinterHelper* StmtCollector::getPrinterHelper() const
{
  return m_PrinterHelper.get();
}

//--------------------------------------------------------- 
inline const PrintingPolicy& StmtCollector::getPrintingPolicy() const
{
  return m_PrintingPolicy;
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_STMTCOLLECTOR_H
