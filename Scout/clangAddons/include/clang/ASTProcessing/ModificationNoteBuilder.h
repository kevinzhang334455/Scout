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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_MODIFICATIONNOTEBUILDER_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_MODIFICATIONNOTEBUILDER_H

#include "clang/Basic/SourceLocation.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class IdentifierInfo;

//--------------------------------------------------------- 
namespace ASTProcessing {

class StmtCollector;

//--------------------------------------------------------- 
// ModificationNoteBuilder is tightly coupled to StmtCollector 
// but not a member class in order to be forward declareable
class ModificationNoteBuilder
{
  SourceRange       m_OriginalLoc;
  unsigned          m_DiagId;
  std::string       m_pArg;
  const unsigned    *m_pStartLine, *m_pEndLine;
  ModificationNoteBuilder(const SourceRange& loc, unsigned d);
  friend class StmtCollector;
public:
  ModificationNoteBuilder& operator<<(const std::string& pArg);
};

//--------------------------------------------------------- inlines
inline ModificationNoteBuilder::ModificationNoteBuilder
    (const SourceRange& loc, unsigned d) : 
  m_OriginalLoc(loc), 
  m_DiagId(d), 
  m_pStartLine(0), 
  m_pEndLine(0) 
{}

//--------------------------------------------------------- 
inline ModificationNoteBuilder& ModificationNoteBuilder::operator<<
    (const std::string& pArg)
{
  assert(!pArg.empty());
  m_pArg = pArg;
  return *this;
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_MODIFICATIONNOTEBUILDER_H
