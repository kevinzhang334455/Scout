//===--- ArtificialIdentifierPolicy.cpp -----------------------------------===//
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

#include "clang/ASTProcessing/AttachedPragmas.h"
#include "clang/ASTProcessing/StmtCollector.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/LiteralSupport.h"
#include "clang/Lex/Pragma.h"
#include "clang/Parse/Parser.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Diagnostic.h"
#include <map>

#ifdef SCOUT_VECTORIZER_PARSER_PATCHED
extern clang::Parser *globalHackedParser;
#endif

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- 
namespace ASTProcessing {

//--------------------------------------------------------- 
void PragmaArgumentInfo::insertArgument(const llvm::StringRef& name, Expr* E)
{
  m_Arguments[name.str()] = E;
}

//--------------------------------------------------------- 
Expr* PragmaArgumentInfo::getExprForArgument(const char* pName) const
{
  tArguments::const_iterator i = m_Arguments.find(pName);
  return i != m_Arguments.end() ? i->second : 0;
}

//--------------------------------------------------------- 
bool PragmaArgumentInfo::hasArgument(const char* pName) const
{
  return m_Arguments.find(pName) != m_Arguments.end();
}

//--------------------------------------------------------- 
bool PragmaArgumentInfo::findValueForArgument(const char* pName, const ASTContext& Ctx, int& value) const
{
  tArguments::const_iterator i = m_Arguments.find(pName);
  llvm::APSInt Result;
  if (i != m_Arguments.end() &&
      i->second != 0 && 
      i->second->EvaluateAsInt(Result, Ctx, Expr::SE_NoSideEffects))
  {
    value = Result.getSExtValue();
    return true;
  }
  return false;
}

//--------------------------------------------------------- 
class AttachedPragmaCollector::Handler : public PragmaHandler
{
  struct tPragma
  {
    IdentifierInfo*     m_ActionName;
    PragmaArgumentInfo  m_Arguments;
  };

  typedef StmtCollector::tLineLocation tLineLocation;

  typedef std::map<tLineLocation, tPragma> tCollectedPragmas;
  tCollectedPragmas m_CollectedPragmas;   

  typedef std::multimap<tLineLocation, const tPragma*> tDeclPragmas;
  tDeclPragmas m_DeclPragmas;
  SourceManager& m_SM;

public:
  Handler(SourceManager& SM, const IdentifierInfo* domainName);
  virtual void HandlePragma(Preprocessor &PP, PragmaIntroducerKind, Token &FirstToken);

  void sortCollectionToStmts(StmtCollector& collector) const;
  void collectDeclarationPragmas(Decl* D);

  DiagnosticBuilder Warn(Preprocessor &PP, const SourceLocation& loc, const char* msg);
};

//--------------------------------------------------------- 
AttachedPragmaCollector::Handler::Handler(SourceManager& SM, 
                                          const IdentifierInfo* domainName) :
  PragmaHandler(domainName->getName()),
  m_SM(SM)
{}

//--------------------------------------------------------- 
void AttachedPragmaCollector::Handler::HandlePragma(Preprocessor &PP, PragmaIntroducerKind,
                                                    Token &FirstToken)
{
  assert(FirstToken.is(tok::identifier) && 
         getName() == FirstToken.getIdentifierInfo()->getName());
  Token nextTokenAtLine;
  IdentifierInfo* actionName = 0;
  PragmaArgumentInfo* pArguments = 0;
  PP.Lex(nextTokenAtLine);
  while (!nextTokenAtLine.is(tok::eod))
  {
    if (actionName == 0 && nextTokenAtLine.is(tok::identifier))
    {
      tLineLocation pragmaLine(m_SM.getPresumedLoc(nextTokenAtLine.getLocation()));
      if (pragmaLine.m_Loc.isInvalid())
      {
        return;
      }
      actionName = nextTokenAtLine.getIdentifierInfo();
      tPragma& scoutPragma = m_CollectedPragmas[pragmaLine];
      pArguments = &scoutPragma.m_Arguments;
      scoutPragma.m_ActionName = actionName;
    }
    else if (pArguments != 0)
    {
      if (nextTokenAtLine.is(tok::identifier))
      {
        StringRef argName(nextTokenAtLine.getIdentifierInfo()->getName());
        Expr* E = 0;
        PP.Lex(nextTokenAtLine);
        if (nextTokenAtLine.is(tok::l_paren))
        {
#ifdef SCOUT_VECTORIZER_PARSER_PATCHED
          //SourceLocation saveLoc = globalHackedParser->accessPrevTokLocation();
          globalHackedParser->ConsumeAnyToken();
          // suppress warnings about unused expression results:
          bool bIgnoreWarnings = PP.getDiagnostics().getIgnoreAllWarnings();
          PP.getDiagnostics().setIgnoreAllWarnings(true);
          ExprResult result = globalHackedParser->ParseExpression();
          PP.getDiagnostics().setIgnoreAllWarnings(bIgnoreWarnings);
          //globalHackedParser->accessPrevTokLocation() = saveLoc;
          if (result.isUsable())
          {
            E = result.get();
            nextTokenAtLine = globalHackedParser->getCurToken();
          }
          if (nextTokenAtLine.is(tok::r_paren))
          {
            PP.Lex(nextTokenAtLine);
          }
          else
          {
            Warn(PP, nextTokenAtLine.getLocation(), "expected ')' token");
          }
#else
          Warn(PP, nextTokenAtLine.getLocation(), "unpatched clang does not support arguments - rest of line ignored");
#endif
        }
        pArguments->insertArgument(argName, E);
        continue;
      }
      else
      {
        Warn(PP, nextTokenAtLine.getLocation(), "unrecognized argument in pragma action %0") << actionName;
      }
    }
    PP.Lex(nextTokenAtLine);
  }
  if (actionName == 0)
  {
    Warn(PP, nextTokenAtLine.getLocation(), "missing action in pragma domain %0") << FirstToken.getIdentifierInfo();
  }
}

//--------------------------------------------------------- 
DiagnosticBuilder AttachedPragmaCollector::Handler::Warn(Preprocessor &PP, const SourceLocation& loc, const char* msg)
{
  unsigned warn_pragma = PP.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, msg);
  return PP.getDiagnostics().Report(FullSourceLoc(loc, m_SM), warn_pragma);
}

//--------------------------------------------------------- 
void AttachedPragmaCollector::Handler::sortCollectionToStmts(StmtCollector& collector) const
{
  for (tDeclPragmas::const_iterator i = m_DeclPragmas.begin(), 
       e = m_DeclPragmas.end(); i != e; ++i)
  {
    collector.attachPragma(i->first, getName(), 
      i->second->m_ActionName, &i->second->m_Arguments);
  }

  llvm::df_iterator<Stmt*> i = llvm::df_begin(collector.getRoot()), 
                           e = llvm::df_end(collector.getRoot());
  assert(i != e && "there is no root!");
  tLineLocation currentLine(m_SM.getPresumedLoc(i->getSourceRange().getBegin()));
  if (currentLine.m_Loc.isInvalid())
  {
    return;
  }

  tCollectedPragmas::const_iterator 
    // we don't support pragmas between a function declaration and the body:
    iPragma = m_CollectedPragmas.upper_bound(currentLine), 
    ePragma = m_CollectedPragmas.end();
  for (++i; i != e && iPragma != ePragma; ++i)
  {
    tLineLocation loc(m_SM.getPresumedLoc(i->getSourceRange().getBegin()));
    if (loc.m_Loc.isInvalid())
    {
      continue;
    }
    unsigned stmtLine = loc.m_Loc.getLine();
    if (stmtLine != currentLine.m_Loc.getLine())
    {
      for (; iPragma != ePragma && iPragma->first < loc; ++iPragma)
      {
        collector.attachPragma(loc, getName(), 
          iPragma->second.m_ActionName, &iPragma->second.m_Arguments);
      }
      currentLine = loc;
    }
  }
}

//--------------------------------------------------------- 
void AttachedPragmaCollector::Handler::collectDeclarationPragmas(Decl* D)
{
  PresumedLoc loc = m_SM.getPresumedLoc(D->getLocation());
  if (loc.isValid())
  {
    for (unsigned pragmaLine = loc.getLine(); pragmaLine > 0;)
    {
      --pragmaLine;
      PresumedLoc prevLoc(loc.getFilename(), pragmaLine, 0, loc.getIncludeLoc());
      tCollectedPragmas::const_iterator iPragma = 
        m_CollectedPragmas.find(tLineLocation(prevLoc));
      // support only pragmas as immediate successors of a function declaration:
      if (iPragma == m_CollectedPragmas.end())
      {
        break;
      }
      m_DeclPragmas.insert(tDeclPragmas::value_type(tLineLocation(loc), &iPragma->second));
    }
  }
}

//--------------------------------------------------------- 
AttachedPragmaCollector::~AttachedPragmaCollector()
{
  for (tHandlers::const_iterator i = PragmaHandlers.begin(), 
       e = PragmaHandlers.end(); i != e; ++i)
  {
    PP.RemovePragmaHandler(m_Namespace.c_str(), i->get());
  }
}

//--------------------------------------------------------- 
void AttachedPragmaCollector::addDomain(const char* pDomainName)
{
  const IdentifierInfo* domainName = PP.getIdentifierInfo(pDomainName);
  Handler* pHandler = new Handler(PP.getSourceManager(), domainName);
  PragmaHandlers.push_back(std::shared_ptr<Handler>(pHandler));
  PP.AddPragmaHandler(m_Namespace.c_str(), pHandler);
}

//--------------------------------------------------------- 
void AttachedPragmaCollector::collectParsedPragmas(
  StmtCollector& collector) const
{
  for (tHandlers::const_iterator i = PragmaHandlers.begin(),
       e = PragmaHandlers.end(); i != e; ++i)
  {
    (*i)->sortCollectionToStmts(collector);
  }
}

//--------------------------------------------------------- 
void AttachedPragmaCollector::collectDeclarationPragmas(Decl* D) 
{
  for (tHandlers::const_iterator i = PragmaHandlers.begin(),
       e = PragmaHandlers.end(); i != e; ++i)
  {
    (*i)->collectDeclarationPragmas(D);
  }
}


//--------------------------------------------------------- 
} // namespace ASTProcessing

} // ns clang
