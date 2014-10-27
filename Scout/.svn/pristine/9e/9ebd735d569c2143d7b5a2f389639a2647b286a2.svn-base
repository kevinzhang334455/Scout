#include "clang/Interface/Interface.h"

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Path.h"
#include "llvm/Option/OptTable.h"

#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/ASTProcessing/ArtificialIdentifierPolicy.h"
#include "clang/ASTProcessing/StmtEditor.h"
#include "clang/ASTProcessing/StmtCollector.h"
#include "clang/ASTProcessing/Inline.h"    
#include "clang/ASTProcessing/InvariantIf.h"    
#include "clang/ASTProcessing/LoopUnroll.h"    
#include "clang/ASTProcessing/LoopBlocking.h"    
//#include "clang/ASTProcessing/LoopSplit.h"    
#include "clang/ASTProcessing/AttachedPragmas.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/Vectorizing/Vectorize.h"    
#include "clang/Vectorizing/IntrinsicCollector.h"    
#include "clang/Vectorizing/IntrinsicEditor.h"    
#include "clang/ASTProcessing/DeclCollector.h" 
#include "clang/AST/ASTConsumer.h"
#include "clang/Lex/Pragma.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"

#include "clang/Vectorizing/ValueFlowAnalysis.h"    
#include "clang/Vectorizing/Configuration.h"    

#include "clang/Driver/CC1AsOptions.h"

#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "clang/Frontend/FrontendDiagnostic.h"
//#include "clang/Frontend/DiagnosticOptions.h"
//#include "clang/Frontend/HeaderSearchOptions.h"
//#include "clang/Frontend/PreprocessorOptions.h"
//#include "clang/Frontend/FrontendOptions.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/Utils.h"
#include "clang/Sema/SemaDiagnostic.h"
#include "clang/Sema/SemaConsumer.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/Version.h"
#include "clang/Parse/ParseAST.h"

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- 
namespace {

//--------------------------------------------------------- 
void testValueFlowAnalysis(ASTProcessing::StmtEditor& editor, Stmt *Body)
{
  ASTProcessing::ValueFlowCollector valueFlowCollector(editor.Ctx(), &editor.getFnDecl());
  ASTProcessing::ValueFlowContext vfCtx(valueFlowCollector);
  PrintingPolicy printingPolicy(editor.Ctx().getLangOpts()); 

  BinaryOperator* Node;
  std::string streamString;
  llvm::raw_string_ostream commentStream(streamString);
  for (Stmt::child_iterator i = Body->child_begin(), e = Body->child_end(); 
       i != e; ++i)
  {
    if ((Node = dyn_cast<BinaryOperator>(*i)) != 0 && 
         Node->getOpcode() == BO_Assign)
    {
      const MemRegion* memId = vfCtx.getLValueMemRegion(Node->getLHS());
      if (memId == 0)
      {
        editor.attachComment(Node, "error: memregion of lhs not computed!");  
        continue;
      }
      SVal rhsResult = vfCtx.getRValueSVal(Node->getRHS());
      vfCtx.bindValue(memId, rhsResult);
      commentStream.str().erase();
      commentStream << "lhs: " << (void*)memId << " bound to ";
      rhsResult.dumpToStream(commentStream);
      editor.attachComment(Node, commentStream.str().c_str());  
      editor.attachComment(Node, "rhs bindings:");  
      for (ASTProcessing::MemRegion_iterator i2(Node->getRHS(), vfCtx), e2(vfCtx); i2 != e2; ++i2)
      {
        commentStream.str().erase();
        i2->printPretty(commentStream, 0, printingPolicy);
        commentStream << ": " << (void*)i2.region();
        SVal boundVal = vfCtx.getSVal(i2.region());
        if (!boundVal.isUnknown())
        {
          commentStream << " -> ";
          boundVal.dumpToStream(commentStream);
        }
        else
        {
          commentStream << " -> not bound";
        }
        editor.attachComment(Node, commentStream.str().c_str());  
      }
    }
  }
}

//--------------------------------------------------------- 
struct RewriteInlineData
{
  llvm::raw_os_ostream targetStream;
  ASTProcessing::Configuration& m_Configuration;
  const std::string& prologText;
  bool m_bTestValueFlow;

  RewriteInlineData(std::ostream& t, ASTProcessing::Configuration& c,
                    const std::string& pT) : 
    targetStream(t), m_Configuration(c), prologText(pT), m_bTestValueFlow(false) 
  {}
};

//--------------------------------------------------------- 
class RewriteInline : public SemaConsumer
{
  CompilerInstance& CI;
  RewriteInlineData& m_Data;

  ASTProcessing::AttachedPragmaCollector PragmaHandler;

  SourceLocation getPrettyIncludePosition();

  void collectPreprocessorDirectives();

  std::map<unsigned, std::string> m_PreprocessorDirectives;

public:
  RewriteInline(CompilerInstance &CInst, RewriteInlineData& d) : 
    CI(CInst),
    m_Data(d),
    PragmaHandler(CInst.getPreprocessor(), "scout")
  {
    PragmaHandler.addDomain("condition");
    PragmaHandler.addDomain("loop");
    PragmaHandler.addDomain("function");
  }

  virtual void HandleTranslationUnit(ASTContext &C);

  virtual void InitializeSema(Sema &S) { CI.setSema(&S); }

  virtual void ForgetSema() { CI.takeSema(); }

};

//--------------------------------------------------------- 
void RewriteInline::collectPreprocessorDirectives()
{
  /*Preprocessor& PP = CI.getPreprocessor();
  SourceManager& SM = CI.getSourceManager();
  FileID MainFileID = SM.getMainFileID();
  const llvm::MemoryBuffer *FromFile = SM.getBuffer(MainFileID);
  Lexer RawLex(MainFileID, FromFile, SM, CI.getLangOpts());

  Token Tok;
  RawLex.Lex(Tok);
  while (!Tok.is(tok::eof))  
  {  
    assert(Tok.isAtStartOfLine());
    if (Tok.is(tok::hash))
    {
      RawLex.setParsingPreprocessorDirective(true);
      std::string restOfLine = RawLex.ReadToEndOfLine();
      unsigned directiveLine = SM.getPresumedLoc(Tok.getLocation()).getLine();
      m_PreprocessorDirectives[directiveLine] = restOfLine;
    }
    else
    {
      while (!(Tok.is(tok::eof) || Tok.isAtStartOfLine())) { RawLex.Lex(Tok); } 
    }
  }*/
}

//--------------------------------------------------------- 
SourceLocation RewriteInline::getPrettyIncludePosition()
{
  Preprocessor& PP = CI.getPreprocessor();
  SourceManager& SM = CI.getSourceManager();
  FileID MainFileID = SM.getMainFileID();
  const llvm::MemoryBuffer *FromFile = SM.getBuffer(MainFileID);
  Lexer RawLex(MainFileID, FromFile, SM, CI.getLangOpts());
  SourceLocation result = SM.getLocForStartOfFile(MainFileID);

  Token Tok;
  RawLex.LexFromRawLexer(Tok);
  while (!Tok.is(tok::eof))  
  {  
    assert(Tok.isAtStartOfLine());
    result = Tok.getLocation();
    if (!Tok.is(tok::hash))
    {
      break;
    }

    RawLex.LexFromRawLexer(Tok);
    if (!Tok.isAnyIdentifier()) 
    {
      break;
    }

    IdentifierInfo* II = PP.LookUpIdentifierInfo(Tok);
    tok::PPKeywordKind K = II->getPPKeywordID();
    if (K != tok::pp_include && K != tok::pp_line)
    {
      break;
    }

    while (!(Tok.is(tok::eof) || Tok.isAtStartOfLine())) { RawLex.LexFromRawLexer(Tok); }
  }
  return result;
}

//--------------------------------------------------------- 
void RewriteInline::HandleTranslationUnit(ASTContext &C) 
{
  if (CI.getPreprocessor().getDiagnostics().hasErrorOccurred())
    return;

  Rewriter Rewrite;
  SourceManager& SM = C.getSourceManager();
  Rewrite.setSourceMgr(C.getSourceManager(), C.getLangOpts());

  unsigned diagID = CI.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "start processing");
  CI.getDiagnostics().Report(FullSourceLoc(), diagID);

  collectPreprocessorDirectives();

  FileID MainFileID = SM.getMainFileID();
  Rewrite.InsertText(SM.getLocForStartOfFile(MainFileID), "", false);

  for (DeclContext::decl_iterator 
         D = C.getTranslationUnitDecl()->decls_begin(),
         DEnd = C.getTranslationUnitDecl()->decls_end();
       D != DEnd; 
       ++D)
  {
    PragmaHandler.collectDeclarationPragmas(*D);
    FunctionDecl *FD;
    Stmt *Body;
    if ((FD = dyn_cast<FunctionDecl>(*D)) != 0 &&  
        FD->isThisDeclarationADefinition() && 
        (Body = FD->getBody()) != 0 &&
        MainFileID == FullSourceLoc(Body->getLocStart(), SM).getFileID())
    {  
      ArtificialIdentifierPolicy::resetCounter();
      ASTProcessing::IntrinsicCollector rewriter(CI, ArtificialIdentifierPolicy("inlined"), *FD, m_Data.m_Configuration);
      if (m_Data.m_bTestValueFlow)
      {
        ASTProcessing::StmtEditor editor(rewriter);
        testValueFlowAnalysis(editor, Body);
        rewriter.rewrite(Rewrite);
      }
      else
      {
        PragmaHandler.collectParsedPragmas(rewriter);
        if (rewriter.hasAnyPragmas()) 
        {
          bool bInlineAll = rewriter.findAttachedPragma(FD->getLocation(), "function", "expand") != 0; 
          ASTProcessing::IntrinsicEditor editor(rewriter);
          bool bFunctionChanged = bInlineAll ? ASTProcessing::doInline(editor, Body) : false;
          //ASTProcessing::splitLoops(editor, Body);
          if (bFunctionChanged)
          {
            ASTProcessing::DeclCollector::makeCAst(editor);
          }
          bFunctionChanged = ASTProcessing::splitInvariantIfs(editor, Body, true) || bFunctionChanged;
          bFunctionChanged = ASTProcessing::unrollLoops(editor, Body) || bFunctionChanged;
          bFunctionChanged = ASTProcessing::blockLoops(editor, Body) || bFunctionChanged;
          bFunctionChanged = ASTProcessing::vectorizeLoops(editor, Body) || bFunctionChanged;
          if (bFunctionChanged)
          {
            rewriter.rewrite(Rewrite);
          }
        }
      }
    }
  }

  if (!m_Data.prologText.empty())
  {
    Rewrite.InsertTextBefore(getPrettyIncludePosition(), m_Data.prologText);
  }

  // Get the buffer corresponding to MainFileID.  If we haven't changed it, then
  // we are done.
  if (const RewriteBuffer *RewriteBuf = 
      Rewrite.getRewriteBufferFor(MainFileID)) {
    //printf("Changed:\n");
    m_Data.targetStream << std::string(RewriteBuf->begin(), RewriteBuf->end());
  } else {
    unsigned diagID = CI.getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "no changes");
    CI.getDiagnostics().Report(FullSourceLoc(), diagID);
  }

  m_Data.targetStream.flush();
}

} // anon namespace


//------------------------------------------------------------------------- 
class ScoutClangAddonAction : public FrontendAction 
{
public:
  virtual bool usesPreprocessorOnly() const { return false; }

  void Execute()
  {
    ExecuteAction();
  }

protected:
  virtual void ExecuteAction()
  {
    CompilerInstance &CI = getCompilerInstance();
    ParseAST(CI.getPreprocessor(), &CI.getASTConsumer(), CI.getASTContext(),
             CI.getFrontendOpts().ShowStats);
  }
};

//------------------------------------------------------------------------- 
class RewriteInlineAction : public ScoutClangAddonAction
{
public:
  RewriteInlineAction(RewriteInlineData& d) : m_Data(d) {}
protected:
  RewriteInlineData& m_Data;

  virtual ASTConsumer *CreateASTConsumer(CompilerInstance &CI,
                                         llvm::StringRef InFile)
  {
    return new RewriteInline(CI, m_Data);
  }
};

//------------------------------------------------------------------------- 
class ScoutCompiler
{
public:
  ScoutCompiler(const char *sourceStartPtr, const char *sourceEndPtr, 
                const char *sourceFileName, std::ostream& diagnostics);

  bool processAction(ScoutClangAddonAction& Act);
  CompilerInvocation &getInvocation() { return m_Clang->getInvocation(); }
  DiagnosticsEngine &getDiagnostics() { return m_Clang->getDiagnostics(); }
private:
  llvm::raw_os_ostream              m_DiagnosticStream;
  llvm::OwningPtr<CompilerInstance> m_Clang;

  llvm::MemoryBuffer  *m_SourceBuffer;
  const char          *m_SourceFileName;
};


//--------------------------------------------------------- 
class DelegatingDiagnosticClient : public DiagnosticConsumer {
  llvm::OwningPtr<DiagnosticConsumer> Delegate;

public:
  void setDelegate(DiagnosticConsumer *_Delegate) {
    Delegate.reset(_Delegate);
  }

  virtual void BeginSourceFile(const LangOptions &LO,
                               const Preprocessor *PP) {
    if (Delegate) Delegate->BeginSourceFile(LO, PP);
  }

  virtual void EndSourceFile() {
    if (Delegate) Delegate->EndSourceFile();
  }

  virtual bool IncludeInDiagnosticCounts() const {
    if (Delegate) Delegate->IncludeInDiagnosticCounts();
    return true;
  }

  virtual void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
                                const Diagnostic &Info) {
    if (Delegate) 
    {
      Delegate->HandleDiagnostic(DiagLevel, Info);
      NumWarnings = Delegate->getNumWarnings();
      NumErrors = Delegate->getNumErrors();
    }
  }
};


//--------------------------------------------------------- 
ScoutCompiler::ScoutCompiler(const char *sourceStartPtr, const char *sourceEndPtr, 
                             const char *sourceFileName, std::ostream& diagnostics) :
  m_DiagnosticStream(diagnostics),
  m_Clang(new CompilerInstance()),
  m_SourceFileName(sourceFileName)
{
  DiagnosticOptions* diagOpts = new DiagnosticOptions();
  diagOpts->ShowCarets = 0;
  DiagnosticConsumer* diagClient = new TextDiagnosticPrinter(m_DiagnosticStream, diagOpts);
  m_Clang->setDiagnostics(new DiagnosticsEngine(llvm::IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), diagOpts, diagClient));
  m_SourceBuffer = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(sourceStartPtr, sourceEndPtr-sourceStartPtr), sourceFileName);
}


//--------------------------------------------------------- 
bool ScoutCompiler::processAction(ScoutClangAddonAction& Act)
{
  m_Clang->setTarget(TargetInfo::CreateTargetInfo(m_Clang->getDiagnostics(), &m_Clang->getTargetOpts()));
  m_Clang->createFileManager();
  m_Clang->createSourceManager(m_Clang->getFileManager());
  m_Clang->createPreprocessor();
  m_Clang->getSourceManager().createMainFileIDForMemBuffer(m_SourceBuffer);

  if (!m_Clang->getDiagnostics().hasErrorOccurred()) 
  {
    InputKind IK = FrontendOptions::getInputKindForExtension(
        llvm::StringRef(m_SourceFileName).rsplit('.').second);
    if (IK == IK_None)
    {
      IK = IK_C;
    }

    if (Act.BeginSourceFile(*m_Clang, FrontendInputFile(m_SourceFileName, IK))) 
    {
      Act.Execute();
      Act.EndSourceFile();
    }
  }

  return !m_Clang->getDiagnostics().hasErrorOccurred();
}

//--------------------------------------------------------- 
bool processSource(const char *sourceStartPtr, 
                   const char *sourceEndPtr,
                   const char *sourceFileName,
                   std::vector<const char*> Args,
                   std::ostream& targetFile, 
                   std::ostream& diagnostics,
                   const std::string& prologText,
                   ASTProcessing::Configuration& configuration)
{
  RewriteInlineData data(targetFile, configuration, prologText);

  ScoutCompiler compiler(sourceStartPtr, sourceEndPtr, sourceFileName, diagnostics);
  Args.push_back(sourceFileName);

  for (std::vector<const char*>::iterator i = Args.begin(), e = Args.end(); 
       i != e; ++i)
  {
    if (strcmp(*i, "-scout:test_valueflow") == 0)
    {
      data.m_bTestValueFlow = true;
      Args.erase(i);
      break;
    }
  }

  CompilerInvocation& CI = compiler.getInvocation();
  CompilerInvocation::CreateFromArgs(CI, &Args[0], (&Args[0]) + Args.size(), compiler.getDiagnostics());
  RewriteInlineAction Act(data);
  return compiler.processAction(Act);
}


//--------------------------------------------------------- 
bool retrieveFilesFromArg(std::vector<const char*>& Args, 
                          std::ostream& diagnostics,
                          const char* Extension,
                          tInOutFileList& fileList)
{
  if (Args.size() < 2)
  {
    return true;  // no arguments -> no files, but no error
  }

  llvm::raw_os_ostream DiagnosticStream(diagnostics);
  DiagnosticOptions* diagOpts = new DiagnosticOptions();
  diagOpts->ShowCarets = 0;
  DiagnosticsEngine diag(llvm::IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), diagOpts, new TextDiagnosticPrinter (DiagnosticStream, diagOpts));

  CompilerInvocation CI;
  CompilerInvocation::CreateFromArgs(CI, &Args[1], (&Args[0]) + Args.size(), diag); // don't parse the executable
  FrontendOptions &FO = CI.getFrontendOpts();

  // Honor -help.
  if (FO.ShowHelp) {
    llvm::OwningPtr<llvm::opt::OptTable> Opts(driver::createCC1AsOptTable());
    llvm::raw_os_ostream diagStream(diagnostics);
    Opts->PrintHelp(diagStream, "scout",
                    "LLVM 'Clang' Compiler: http://clang.llvm.org");
    return false;
  }

  // Honor -version.
  //
  // FIXME: Use a better -version message?
  if (FO.ShowVersion) {
    llvm::cl::PrintVersionMessage();
    return false;
  }

  fileList.reserve(FO.Inputs.size());
  for (size_t i = 0, e = FO.Inputs.size(); i < e; ++i)
  {
    switch (FO.Inputs[i].getKind())
    {
      case IK_C:
      case IK_CXX:
      case IK_None:
      {
        std::string OutFile;
        if (!FO.OutputFile.empty()) {
          OutFile = FO.OutputFile;
        } else if (FO.Inputs[i].getFile() == "-") {
          OutFile = "-";
        } else if (Extension != 0) {
          SmallString<128> Path(FO.Inputs[i].getFile());
          llvm::sys::path::replace_extension(Path, Extension);
          OutFile = Path.str();
        } else {
          OutFile = "-";
        }
        fileList.push_back(tInOutFileList::value_type(FO.Inputs[i].getFile(), OutFile));
        break;
      }

      default:
        diagnostics << "scout error: unknown input kind of " << FO.Inputs[i].getFile().str() << "\n"; 
        return false;
    }
  }
  return true;
}

//------------------------------------------------------------------------- 
class ParseConfigurationAction : public ASTFrontendAction 
{
public:
  ParseConfigurationAction(ASTProcessing::Configuration& c) : 
    m_Configuration(c),
    m_Compiler(new CompilerInstance()) {}

  ~ParseConfigurationAction() { m_Compiler.take(); }

  CompilerInstance* getCompiler() { return m_Compiler.get(); }
protected:
  ASTProcessing::Configuration&  m_Configuration;
  llvm::OwningPtr<CompilerInstance> m_Compiler;

  virtual ASTConsumer *CreateASTConsumer(CompilerInstance&, llvm::StringRef)
  {
    return new ASTProcessing::ParseConfigurationConsumer(m_Configuration, m_Compiler);
  }
};

//--------------------------------------------------------- 
bool processConfiguration(const char *sourceFileName,
                          ASTProcessing::Configuration& configuration, 
                          std::ostream& diagnostics)
{
  llvm::raw_os_ostream DiagnosticStream(diagnostics);
  ParseConfigurationAction Act(configuration);
  CompilerInstance* Clang = Act.getCompiler();
  
  DiagnosticOptions& DiagOpts = Clang->getDiagnosticOpts();
  DiagOpts.Warnings.push_back("no-unused-value");
  DiagOpts.Warnings.push_back("no-return-type");
  DiagOpts.ShowCarets = 0;
  DelegatingDiagnosticClient* diagClient = new DelegatingDiagnosticClient();
  diagClient->setDelegate(new TextDiagnosticPrinter(DiagnosticStream, &DiagOpts));

  Clang->setDiagnostics(new DiagnosticsEngine(llvm::IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs()), &DiagOpts, diagClient));
  Clang->getInvocation().getLangOpts()->CPlusPlus = 1;
  Clang->getInvocation().getTargetOpts().Triple = llvm::sys::getDefaultTargetTriple();
  Clang->getFrontendOpts().Inputs.push_back(FrontendInputFile(sourceFileName, IK_CXX));
  ProcessWarningOptions(Clang->getDiagnostics(), DiagOpts);

  unsigned diagID = Clang->getDiagnostics().getCustomDiagID(DiagnosticsEngine::Warning, "load configuration %0");
  Clang->getDiagnostics().Report(FullSourceLoc(), diagID) << sourceFileName;

  bool result = Clang->ExecuteAction(Act);
  diagClient->setDelegate(0);
  return result;
}

//------------------------------------------------------------------------- 
ASTProcessing::Configuration* createConfiguration()
{
  return new ASTProcessing::Configuration();
}

//------------------------------------------------------------------------- 
void destroyConfiguration(ASTProcessing::Configuration* c)
{
  delete c;
}

//------------------------------------------------------------------------- 
std::string getClangAddonsVersion()
{
  return getClangFullVersion();
}

//------------------------------------------------------------------------- 
} // namespace clang
 
//------------------------------------------------------------------------- 

