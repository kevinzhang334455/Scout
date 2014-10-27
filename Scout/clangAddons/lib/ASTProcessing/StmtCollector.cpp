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

#include "clang/ASTProcessing/StmtCollector.h"
#include "clang/ASTProcessing/StmtClone.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/AST/ASTContext.h"    
#include "clang/AST/Expr.h"
#include "clang/Lex/Lexer.h"
#include "clang/Basic/SourceManager.h"
#include "llvm/Support/MemoryBuffer.h"
#include <algorithm>
#include <boost/format.hpp>

using namespace std::placeholders;

//--------------------------------------------------------- 
namespace clang {
  namespace ASTProcessing {

//--------------------------------------------------------- 
class CommentPrinter : public StmtCollector::LinkedPrinterHelper
{
public:
  CommentPrinter(const StmtCollector& c) : m_Collector(c) {}
  virtual bool handledStmt(Stmt* E, llvm::raw_ostream& OS, unsigned IndentLevel);
  virtual bool handledStmt(Stmt* E, llvm::raw_ostream& OS)
  {
    return handledStmt(E, OS, 0);
  }

  typedef llvm::SmallVector<std::string, 8> tCommentCollection;
  typedef llvm::DenseMap<Stmt*, tCommentCollection> tCommentMap;
  tCommentMap m_AttachedComments;

private:
  bool rewriteOriginal(const SourceRange& SR, llvm::raw_ostream& OS);
  static bool checkParentSourceRange(const StmtCollector::tStmtTreeMirror* original);
  const StmtCollector& m_Collector;
};

//--------------------------------------------------------- 
StmtCollector::StmtCollector(CompilerInstance& compiler, const ArtificialIdentifierPolicy& identifierPolicy, 
                             FunctionDecl& fnContext) :
  m_CI(compiler),
  m_Root(fnContext.getBody()),      
  Ctx(compiler.getASTContext()),
  m_FnContext(fnContext),
  m_ParentInfo(m_Root),
  m_Diags(compiler.getDiagnostics()),
  m_PrintingPolicy(Ctx.getLangOpts()),
  m_OriginalTree(m_Root, 0)
{
  m_CommentCollector = new CommentPrinter(*this);
  setNextPrinterHelper(m_CommentCollector);
  m_IdentifierPolicy.push_back(identifierPolicy);

  for_each_df(m_Root, std::bind(insertValue<Stmt*>, &m_AllNodes, _1));
  m_OriginalNodes = m_AllNodes;

  //m_PrintingPolicy.IndentStyle = PrintingPolicy::ANSIStyle;
}

//--------------------------------------------------------- 
StmtCollector::~StmtCollector()
{
  gc();
}

//--------------------------------------------------------- 
void StmtCollector::setNextPrinterHelper(LinkedPrinterHelper* lph)
{
  m_PrinterHelper ? 
    m_PrinterHelper->setNextPrinterHelper(lph) : m_PrinterHelper.reset(lph);
}

//--------------------------------------------------------- 
StmtCollector::tStmtTreeMirror::tStmtTreeMirror(Stmt* s, tStmtTreeMirror* parent) : 
  m_OriginalStmt(s),
  m_OriginalParent(parent)
{
  if (s != 0)
  {
    m_SourceRange = s->getSourceRange();
    for (Stmt::child_iterator i = s->child_begin(), e = s->child_end(); i != e; ++i)
    {
      m_Children.push_back(tSharedPtr(new tStmtTreeMirror(*i, this)));
    }
  }
}

//--------------------------------------------------------- 
void StmtCollector::recordClone(const StmtCloneMapping& mapping)
{
  for (StmtClone::StmtMapping::const_iterator i = mapping.m_StmtMapping.begin(), 
       e = mapping.m_StmtMapping.end(); i != e; ++i)
  {
    m_AllNodes.insert(i->second);
    if (m_CreatedNodes.count(i->first))
    {
      m_CreatedNodes.insert(i->second);
    }
    else if (m_OriginalNodes.count(i->first))
    {
      m_ClonedNodes[i->second] = i->first;
    }
    else
    {
      const Stmt* original = m_ClonedNodes.lookup(i->first);
      if (original != 0)
      {
        m_ClonedNodes[i->second] = original;
      }
    }
  }
}

//--------------------------------------------------------- 
void StmtCollector::gc()
{
/*
  using namespace std;
  for_each_df(m_Root, bind(static_cast<bool(llvm::DenseSet<Stmt*>::*)(Stmt* const &)>(&llvm::DenseSet<Stmt*>::erase), ref(m_AllNodes), _1));
  for (llvm::DenseSet<Stmt*>::iterator i = m_AllNodes.begin(), e = m_AllNodes.end(); i != e; ++i)
  {
    std::fill((*i)->child_begin(), (*i)->child_end(), (Stmt*)0);
    (*i)->Destroy(Ctx);
  }
*/
}

//--------------------------------------------------------- 
StmtCollector::tStmtTreeMirror::tChildrenVector::const_iterator 
StmtCollector::tStmtTreeMirror::find(tChildrenVector::const_iterator b, tChildrenVector::const_iterator e, Stmt*S)
{
  for (; b != e; ++b)
  {
    if ((*b)->m_OriginalStmt == S)
    {
      break;
    }
  }
  return b;
}

//--------------------------------------------------------- 
const StmtCollector::tStmtTreeMirror* StmtCollector::tStmtTreeMirror::find_recursively(const Stmt*S) const
{
  if (m_OriginalStmt == S)
  {
    return this;
  }

  const tStmtTreeMirror* result = 0;
  for (tChildrenVector::const_iterator i = m_Children.begin(), 
       e = m_Children.end(); i != e; ++i)
  {
    if ((result = (*i)->find_recursively(S)) != 0)
    {
      return result;
    }
  }
  return 0;
}

//--------------------------------------------------------- 
bool StmtCollector::tStmtTreeMirror::remainedEqualToClone(const tCloneMap& cloneMap, const Stmt* cloneStmt) const
{
  if (m_OriginalStmt == 0)
  {
    return true;    // we are only here, if the new stmt is also 0
  }

  tChildrenVector::const_iterator originalStmts = m_Children.begin();
  tChildrenVector::const_iterator originalStmtsEnd = m_Children.end();
  Stmt::const_child_iterator newStmts = cloneStmt->child_begin();
  Stmt::const_child_iterator newStmtsEnd = cloneStmt->child_end();

  for(; originalStmts != originalStmtsEnd && newStmts != newStmtsEnd; 
      ++originalStmts, ++newStmts)
  {
    tCloneMap::const_iterator i = cloneMap.find(*newStmts);
    if (i == cloneMap.end())
    {
      return false;
    }

    if ((*originalStmts)->m_OriginalStmt != i->second || 
        !(*originalStmts)->remainedEqualToClone(cloneMap, *newStmts))
    {
      return false;
    }
  }
  return originalStmts == originalStmtsEnd && newStmts == newStmtsEnd;   
}


//--------------------------------------------------------- 
bool StmtCollector::tStmtTreeMirror::remainedEqual() const
{
  if (m_OriginalStmt == 0)
  {
    return true;    // we are only here, if the new stmt is also 0
  }

  tChildrenVector::const_iterator originalStmts = m_Children.begin();
  tChildrenVector::const_iterator originalStmtsEnd = m_Children.end();
  Stmt::const_child_iterator newStmts = m_OriginalStmt->child_begin();
  Stmt::const_child_iterator newStmtsEnd = m_OriginalStmt->child_end();

  for(; originalStmts != originalStmtsEnd && newStmts != newStmtsEnd; 
      ++originalStmts, ++newStmts)
  {
    if ((*originalStmts)->m_OriginalStmt != *newStmts || 
        !(*originalStmts)->remainedEqual())
    {
      return false;
    }
  }
  return originalStmts == originalStmtsEnd && newStmts == newStmtsEnd;   
}

//--------------------------------------------------------- 
void StmtCollector::attachComment(Stmt* S, const char* pComment)
{
  m_CommentCollector->m_AttachedComments[S].push_back(pComment);
}

//--------------------------------------------------------- 
void StmtCollector::LinkedPrinterHelper::setNextPrinterHelper(LinkedPrinterHelper* lph)
{
  m_Next ? m_Next->setNextPrinterHelper(lph) : m_Next.reset(lph);
}

//--------------------------------------------------------- 
bool StmtCollector::LinkedPrinterHelper::handledStmt(Stmt* E, llvm::raw_ostream& OS, unsigned IndentLevel)
{
  if (m_Next)
  {
    return m_Next->handledStmt(E, OS, IndentLevel);
  }
  return false;
}

//--------------------------------------------------------- 
bool CommentPrinter::rewriteOriginal(const SourceRange& SR, llvm::raw_ostream& OS)
{
  if (!(SR.getBegin().isMacroID() && SR.getEnd().isMacroID()))
  {
    return false;
  }
  SourceManager& SM = m_Collector.Ctx.getSourceManager();
  std::pair<SourceLocation,SourceLocation> range = SM.getExpansionRange(SR.getBegin());
  if (SM.getFileID(range.first) != SM.getMainFileID() || 
      SM.getFileID(range.second) != SM.getMainFileID())
  {
    return false;
  }

  bool Invalid = false;
  const char* LocStart = SM.getCharacterData(range.first, &Invalid);
  if (Invalid)
  {
    return false;
  }
  if (range.first != range.second)
  {
    const char* LocEnd = SM.getCharacterData(range.second, &Invalid);
    if (!Invalid)
    {
      OS << llvm::StringRef(LocStart, LocEnd - LocStart + 1);
      return true;
    }
  }
  else
  {
    const llvm::MemoryBuffer* buffer = SM.getBuffer(SM.getMainFileID(), &Invalid);
    if (!Invalid)
    {
      assert(buffer->getBufferStart() <= LocStart);
      assert(LocStart < buffer->getBufferEnd());
      Lexer rawLexer(range.first, m_Collector.Ctx.getLangOpts(), buffer->getBufferStart(), LocStart, buffer->getBufferEnd());
      Token token;
      rawLexer.LexFromRawLexer(token);
      if (!token.isAnyIdentifier()) 
      {
        assert(0);
        return false;
      }
      OS << llvm::StringRef(LocStart, token.getLength());
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------- 
bool CommentPrinter::checkParentSourceRange(const StmtCollector::tStmtTreeMirror* original) 
{
  while (original != 0 &&
         original->m_OriginalParent != 0 && 
         original->m_SourceRange == original->m_OriginalParent->m_SourceRange)
  {
    original = original->m_OriginalParent;
  }

  return 
    original != 0 && 
    original->m_OriginalParent != 0 &&
    ((!original->m_OriginalParent->m_SourceRange.getBegin().isMacroID()) || 
     (!original->m_OriginalParent->m_SourceRange.getEnd().isMacroID()));
}

//--------------------------------------------------------- 
bool CommentPrinter::handledStmt(Stmt* E, llvm::raw_ostream& OS, unsigned IndentLevel)
{
  tCommentMap::iterator stmtComments = m_AttachedComments.find(E);
  if (stmtComments != m_AttachedComments.end())
  {
    std::string indentStr(IndentLevel, ' ');
    for (tCommentCollection::iterator i = stmtComments->second.begin(), e = stmtComments->second.end(); i != e; ++i)
    {
      OS << indentStr << "/* " << *i << " */\n";
    }
    if (isa<Expr>(E))
    {
      OS << indentStr;
    }
  }

  if (isa<ParenExpr>(E))
  {
    bool rewriteUnchanged = false;
    const StmtCollector::tStmtTreeMirror* original = 
      m_Collector.m_OriginalTree.find_recursively(E);
    if (original)
    {
      if (!original->remainedEqual())
      {
        original = 0;
      }
    }
    else
    {
      const Stmt* originalStmt = m_Collector.m_ClonedNodes.lookup(E);
      if ((originalStmt != 0) && 
          (original = m_Collector.m_OriginalTree.find_recursively(originalStmt)) != 0 &&  
          !original->remainedEqualToClone(m_Collector.m_ClonedNodes, E))
      {
        original = 0;
      }
    }

    if (checkParentSourceRange(original) && 
        rewriteOriginal(original->m_SourceRange, OS))
    {
      return true;
    }
  }

  return LinkedPrinterHelper::handledStmt(E, OS, IndentLevel);
}

//--------------------------------------------------------- 
void StmtCollector::rewrite(Rewriter& rewriter)
{
  assert(m_OriginalTree.m_OriginalStmt == m_Root && "never replace the root! BTW: how did you managed this?");
 
  std::string newStmtText;
  llvm::raw_string_ostream S(newStmtText);
  m_Root->printPretty(S, m_PrinterHelper.get(), m_PrintingPolicy);
  S.flush();


  SourceRange range = m_OriginalTree.m_SourceRange;
  range.setBegin(Ctx.getSourceManager().getExpansionLoc(range.getBegin()));
  range.setEnd(Ctx.getSourceManager().getExpansionLoc(range.getEnd()));
  int Size = rewriter.getRangeSize(range);
  rewriter.ReplaceText(range.getBegin(), Size, newStmtText);
  emitModificationNotes(rewriter);
}

//--------------------------------------------------------- 
StmtCollector::tPragmaInfo::tPragmaInfo(const IdentifierInfo* A, 
  const llvm::StringRef& D, const tLineLocation& L) :
  m_pAction(A),
  m_Domain(D.str()),
  m_Location(L)
{}

//--------------------------------------------------------- 
bool StmtCollector::tPragmaInfo::operator<(const StmtCollector::tPragmaInfo& other) const 
{
  return m_pAction < other.m_pAction || 
        (m_pAction == other.m_pAction && (m_Domain < other.m_Domain || 
        (m_Domain == other.m_Domain && m_Location < other.m_Location)));
}

//--------------------------------------------------------- 
bool StmtCollector::tLineLocation::operator<(const StmtCollector::tLineLocation& other) const 
{
  int cmpRes = strcmp(m_Loc.getFilename(), other.m_Loc.getFilename());
  return cmpRes < 0 || 
        (cmpRes == 0 && m_Loc.getLine() < other.m_Loc.getLine());
}

//--------------------------------------------------------- 
void StmtCollector::attachPragma(const tLineLocation& loc, llvm::StringRef domainName, 
  const IdentifierInfo* actionName, 
  const PragmaArgumentInfo* arguments)
{
  m_PragmaMap[tPragmaInfo(actionName, domainName, loc)] = arguments;
}

//--------------------------------------------------------- 
const PragmaArgumentInfo* StmtCollector::findAttachedPragma(SourceLocation loc, const char* pDomain, const char* pAction) const
{
  const IdentifierInfo* actionId = &Ctx.Idents.get(llvm::StringRef(pAction));
  PresumedLoc PL = Ctx.getSourceManager().getPresumedLoc(loc);
  tPragmaMap::const_iterator i = m_PragmaMap.end();
  if (PL.isValid())
  {
    i = m_PragmaMap.find(tPragmaInfo(actionId, llvm::StringRef(pDomain), tLineLocation(PL)));
  }
  return i != m_PragmaMap.end() ? i->second : 0;
}

//--------------------------------------------------------- 
bool StmtCollector::hasNoSideEffects(const FunctionDecl* FD) const
{
  return false;
}

//--------------------------------------------------------- 
bool StmtCollector::inlineable(const FunctionDecl* FD) const
{
  return true;
}

//--------------------------------------------------------- 
DiagnosticBuilder StmtCollector::Diag(DiagnosticIDs::Level l, const Stmt* S, const char* msg)
{
  unsigned diagID = m_Diags.getDiagnosticIDs()->getCustomDiagID(l, msg);
  SourceLocation loc = S->getSourceRange().getBegin();
  return m_Diags.Report(loc, diagID);
}

//--------------------------------------------------------- 
ModificationNoteBuilder& StmtCollector::ModificationDiag(DiagnosticIDs::Level l, const Stmt* S, const char* msg)
{
  unsigned diagID = m_Diags.getDiagnosticIDs()->getCustomDiagID(l, msg);
  SourceRange loc = S->getSourceRange();
  const tStmtTreeMirror* originalStmt = m_OriginalTree.find_recursively(S);
  if (originalStmt != 0)
  {
    loc = originalStmt->m_SourceRange;
  }
  else
  {
    // TODO: what shall I do with introduced nodes?
    // test case: a for-Node with a non-compounded body becomes inlined 
    // and then vectorized -> invalid source range end -> assertion
  }
  m_ModificationNotes.push_back(ModificationNoteBuilder(loc, diagID));
  return m_ModificationNotes.back();
}

//--------------------------------------------------------- 
void StmtCollector::emitModificationNotes(Rewriter& rewriter)
{
  FileID mainFile = Ctx.getSourceManager().getMainFileID();
  const RewriteBuffer* buffer = rewriter.getRewriteBufferFor(mainFile);
  assert(buffer && "expect to rewrite the main file!");
  std::map<unsigned, unsigned> offsetToLineMap;
  unsigned dummy = 0;
  for (std::list<ModificationNoteBuilder>::iterator i = m_ModificationNotes.begin(),
       e = m_ModificationNotes.end(); i != e; ++i)
  {
    /*std::pair<FileID,unsigned> VB = Ctx.getSourceManager().getDecomposedLoc(i->m_OriginalLoc.getBegin());
    std::pair<FileID,unsigned> VE = Ctx.getSourceManager().getDecomposedLoc(i->m_OriginalLoc.getEnd());
    if (mainFile == VB.first && mainFile == VE.first)
    {
      unsigned EndOff = buffer->getMappedOffset(VE.second, true);
      unsigned StartOff = buffer->getMappedOffset(VB.second);
      i->m_pStartLine = &offsetToLineMap[StartOff];
      i->m_pEndLine = &offsetToLineMap[EndOff];
    }    
    else*/
    {
      // FIXME: introduce better target diagnostic independent of source
      i->m_pStartLine = i->m_pEndLine = &dummy;
    }
  }

  unsigned uCurrentLineNr = 1, uLastOffset = 0;
  RewriteBuffer::iterator currentPos = buffer->begin();
  for (std::map<unsigned, unsigned>::iterator i = offsetToLineMap.begin(),
       e = offsetToLineMap.end(); i != e; ++i)
  {
    for (unsigned cnt = 0, eCnt = i->first - uLastOffset; cnt < eCnt && currentPos != buffer->end(); 
         ++cnt, ++currentPos)
    {
      if (*currentPos == '\n')
      {
        ++uCurrentLineNr;
      }
    }
    i->second = uCurrentLineNr;
    uLastOffset = i->first;
  }

  for (std::list<ModificationNoteBuilder>::iterator i = m_ModificationNotes.begin(),
       e = m_ModificationNotes.end(); i != e; ++i)
  {
    assert(i->m_pStartLine && i->m_pEndLine);
    if (!i->m_pArg.empty())
    {
      m_Diags.Report(i->m_OriginalLoc.getBegin(), i->m_DiagId)
          << i->m_pArg << *i->m_pStartLine << *i->m_pEndLine;
    }
    else
    {
      m_Diags.Report(i->m_OriginalLoc.getBegin(), i->m_DiagId)
          << *i->m_pStartLine << *i->m_pEndLine;
    }
  }
}

//--------------------------------------------------------- 
Sema& StmtCollector::getSema()
{
  return m_CI.getSema();
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // ns clang
