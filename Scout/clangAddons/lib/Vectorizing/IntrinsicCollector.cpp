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

#include "clang/Vectorizing/IntrinsicCollector.h"
#include "clang/Vectorizing/Configuration.h"
#include "clang/Vectorizing/IntrinsicEditor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/AST/ASTImporter.h"
#include "clang/AST/Expr.h"
#include <boost/format.hpp>

//--------------------------------------------------------- 
namespace clang {
  namespace ASTProcessing {

//--------------------------------------------------------- 
namespace {

inline Configuration::tTypeInfo getTypeInfo(const SimdType& s)
{
  return static_cast<Configuration::tTypeInfo>(s.getPtr());
}

}

//--------------------------------------------------------- 
class IntrinsicCollector::IntrinsicPrinter : public StmtCollector::LinkedPrinterHelper
{
public:
  IntrinsicPrinter(const IntrinsicCollector& c) : m_Collector(c) {}
  virtual bool handledStmt(Stmt* E, llvm::raw_ostream& OS, unsigned IndentLevel);
  virtual bool handledStmt(Stmt* E, llvm::raw_ostream& OS)
  {
    return handledStmt(E, OS, 0);
  }

  const IntrinsicCollector& m_Collector;
};

//--------------------------------------------------------- 
IntrinsicCollector::IntrinsicCollector(CompilerInstance& compiler, 
  const ArtificialIdentifierPolicy& identifierPolicy,  
  FunctionDecl& fnContext, Configuration& config) :
  StmtCollector(compiler, identifierPolicy, fnContext),
  m_Configuration(config),
  m_Importer(new ASTImporter(
    compiler.getASTContext(), compiler.getFileManager(),
    *config.getASTContext(), *config.getFileManager(), true))
{
  setNextPrinterHelper(new IntrinsicPrinter(*this));
}

//--------------------------------------------------------- 
IntrinsicCollector::~IntrinsicCollector()
{}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicCollector::getGlobalFn(const char* pFnName)
{
  Configuration::IntrinsicInfo info = 
    m_Configuration.getGlobalBuiltin(pFnName, *m_Importer);
  if (info.m_Type != 0)
  {
    m_EmittedIntrinsics[info.m_Type] = info.m_PrintFormat;
  }
  return info.m_Type;
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicCollector::getIntrinsicFn(const char* pFnName, SimdType i)
{
  Configuration::IntrinsicInfo info = 
    m_Configuration.getIntrinsicBuiltin(pFnName, getTypeInfo(i), *m_Importer);
  if (info.m_Type != 0)
  {
    m_EmittedIntrinsics[info.m_Type] = info.m_PrintFormat;
  }
  return info.m_Type;
}

//--------------------------------------------------------- 
bool IntrinsicCollector::hasIntrinsicBuiltin(const char* pName, SimdType baseType) const
{
  return m_Configuration.hasIntrinsicBuiltin(pName, getTypeInfo(baseType));
}

//--------------------------------------------------------- 
bool IntrinsicCollector::isIntrinsicFn(FunctionDecl* fnDecl) const
{
  return m_EmittedIntrinsics.count(fnDecl);
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicCollector::getIntrinsicFn(FunctionDecl* fnDecl, SimdType i)
{
  Configuration::IntrinsicInfo info = 
    m_Configuration.getIntrinsicFn(fnDecl, getTypeInfo(i), *m_Importer);
  if (info.m_Type != 0)
  {
    m_EmittedIntrinsics[info.m_Type] = info.m_PrintFormat;
  }
  return info.m_Type;
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicCollector::getConvertFn(SimdType sourceType, SimdType targetType, unsigned index)
{
  Configuration::IntrinsicInfo info = 
    m_Configuration.getConvertFn(getTypeInfo(targetType), 
      sourceType.getBuiltinType(), sourceType.getVectorSize(), index, *m_Importer);
  if (info.m_Type != 0)
  {
    m_EmittedIntrinsics[info.m_Type] = info.m_PrintFormat;
  }
  return info.m_Type;
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicCollector::getIndexedIntrinsicFn(const char* pFnName, SimdType baseType, unsigned index)
{
  Configuration::IntrinsicInfo info = 
    m_Configuration.getIndexedFn(pFnName, getTypeInfo(baseType), index, *m_Importer);
  if (info.m_Type != 0)
  {
    m_EmittedIntrinsics[info.m_Type] = info.m_PrintFormat;
  }
  return info.m_Type;
}

//--------------------------------------------------------- 
FunctionDecl* IntrinsicCollector::getIntrinsicExpr(Expr* E, 
  SimdType i, llvm::SmallVector<Expr*, 4>& subExprs) 
{
  IntrinsicEditor editor(*this);
  Configuration::IntrinsicInfo info = 
    m_Configuration.getIntrinsicExpr(E, getTypeInfo(i), *m_Importer, subExprs, editor);
  if (info.m_Type != 0)
  {
    m_EmittedIntrinsics[info.m_Type] = info.m_PrintFormat;
  }
  return info.m_Type;
}

//--------------------------------------------------------- 
bool IntrinsicCollector::hasNoSideEffects(const FunctionDecl* FD) const
{
  kSideEffectCacheValue& value = m_SideEffectCache[FD];
  if (value == UNKNOWN)
  {
    value = m_Configuration.hasIntrinsicFn(FD, *m_Importer) ? 
              HAS_INTRINSIC : HAS_NO_INTRINSIC;
  }
  return value == HAS_INTRINSIC;
}

//--------------------------------------------------------- 
bool IntrinsicCollector::inlineable(const FunctionDecl* FD) const
{
  kSideEffectCacheValue& value = m_SideEffectCache[FD];
  if (value == UNKNOWN)
  {
    value = m_Configuration.hasIntrinsicFn(FD, *m_Importer) ? 
              HAS_INTRINSIC : HAS_NO_INTRINSIC;
  }
  return value == HAS_NO_INTRINSIC;
}

//--------------------------------------------------------- 
bool IntrinsicCollector::IntrinsicPrinter::handledStmt(Stmt* E, llvm::raw_ostream& OS, unsigned IndentLevel)
{
  if (CallExpr* CE = dyn_cast<CallExpr>(E))
  {
    if (DeclRefExpr* fnRef = dyn_cast<DeclRefExpr>(CE->getCallee()))
    {
      if (FunctionDecl* FD = dyn_cast<FunctionDecl>(fnRef->getDecl()))
      {
        tEmittedIntrinsics::const_iterator stmtIntrinsic = 
          m_Collector.m_EmittedIntrinsics.find(FD);
        if (stmtIntrinsic != m_Collector.m_EmittedIntrinsics.end())
        {
          boost::format intrinsic(stmtIntrinsic->second.str());
          for (unsigned i = 0, e = CE->getNumArgs(); i != e; ++i)
          {
            std::string argument;
            llvm::raw_string_ostream printStream(argument);
            CE->getArg(i)->printPretty(printStream, 
              m_Collector.getPrinterHelper(), m_Collector.getPrintingPolicy());
            intrinsic % printStream.str();
          }
          OS << intrinsic.str();
          return true;  // don't chain?
        }
      }
    }
  }
  return LinkedPrinterHelper::handledStmt(E, OS, IndentLevel);
}

//--------------------------------------------------------- 
QualType IntrinsicCollector::getSimdType(SimdType i) const
{
  return m_Configuration.getSimdType(getTypeInfo(i), *m_Importer);
}

//--------------------------------------------------------- 
QualType IntrinsicCollector::getGSIndexType(SimdType i) const
{
  return m_Configuration.getGSIndexType(getTypeInfo(i), *m_Importer);
}

//--------------------------------------------------------- 
unsigned IntrinsicCollector::getVectorByteAlignment(SimdType i) const
{
  return m_Configuration.getVectorByteAlignment(getTypeInfo(i));
}

//--------------------------------------------------------- 
SimdType IntrinsicCollector::getSimdType(BuiltinType::Kind baseType, unsigned exclMaxSize)
{
  SimdType result;
  result.m_BuiltinType = baseType;
  result.m_VectorSize = 0;
  Configuration::tTypeInfo info = m_Configuration.getTypeInfo(baseType, exclMaxSize);
  if (info != 0)
  {
    result.m_VectorSize = m_Configuration.getVectorSize(info);
    result.m_OpaqueConfigEntry = info;
  }
  return result;
}

//--------------------------------------------------------- 
SimdType IntrinsicCollector::retrieveGSIndexType(const SimdType& baseType)
{
  SimdType result;
  Configuration::tGSTypeInfo info = m_Configuration.getGSTypeInfo(getTypeInfo(baseType));
  if (info.first != 0)
  {
    result.m_OpaqueConfigEntry = info.first;
    result.m_BuiltinType = info.second;
    result.m_VectorSize = m_Configuration.getVectorSize(info.first);
  }
  return result;
}

//--------------------------------------------------------- 
  } // namespace ASTProcessing
} // ns clang
