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

#include "clang/Vectorizing/Configuration.h"    
#include "clang/Vectorizing/ExpressionMatch.h"    
#include "clang/AST/ASTContext.h"    
#include "clang/AST/DeclCXX.h"    
#include "clang/AST/DeclTemplate.h"    
#include "clang/AST/ASTImporter.h"    
#include "clang/Basic/FileManager.h"    
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallSet.h"
#include <sstream>

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

//--------------------------------------------------------- 
namespace {

//--------------------------------------------------------- 
llvm::StringRef getND_Name(const NamedDecl* ND)
{
  return ND->getIdentifier() != 0 ? ND->getIdentifier()->getName() : "";
}

//--------------------------------------------------------- 
bool isND_Name(NamedDecl* ND, const char* name)
{
  return getND_Name(ND) == name;
}

//--------------------------------------------------------- 
bool equalTypes(QualType cfgType, QualType targetType, ASTImporter &C) 
{
  QualType resultType = C.Import(cfgType);
  return (!resultType.isNull()) &&
           resultType.getDesugaredType(C.getToContext()) == 
           targetType.getDesugaredType(C.getToContext());
}

//--------------------------------------------------------- 
bool argumentsMatch(const FunctionDecl* cfgFD, const FunctionDecl* targetFD, ASTImporter &C) 
{
  unsigned numArgs = cfgFD->getNumParams();
  if (targetFD->getNumParams() != numArgs)
  {
    return false;
  }
  for (size_t i = 0; i < numArgs; ++i)
  {
    if (!equalTypes(cfgFD->getParamDecl(i)->getType(), 
                    targetFD->getParamDecl(i)->getType(), C))
    {
      return false;
    }
  }
  return true;
}

} // anon namespace 


//--------------------------------------------------------- 
Configuration::tTypeInfo Configuration::getTypeInfo(BuiltinType::Kind baseType, 
                                                    unsigned exclMaxSize)
{
  tTypeInfos::const_iterator i = m_TypeIntrinsics.find(baseType);
  if (i != m_TypeIntrinsics.end() && !i->second.empty())
  {
    std::map<unsigned, ArchitectureInfo>::const_iterator result = 
      i->second.lower_bound(exclMaxSize);
    if (result == i->second.end())
    {
      --result;
    }
    else if (result->first >= exclMaxSize)
    {
      if (result == i->second.begin())
      {
        return 0;
      }
      --result;
    }
    return &*result;
  }
  return 0;
}

//--------------------------------------------------------- 
Configuration::tGSTypeInfo Configuration::getGSTypeInfo(tTypeInfo baseType)
{
  const std::string& gsTypeName = baseType->second.m_GSIndexType.getAsString();
  if (!gsTypeName.empty())
  {
    for (tTypeInfos::const_iterator i = m_TypeIntrinsics.begin(), e = m_TypeIntrinsics.end(); i != e; ++i)
    {
      for (tArchitectureInfoMap::const_iterator j = i->second.begin(), ej = i->second.end(); j != ej; ++j)
      {
        if (gsTypeName == j->second.m_CanonicalSimdType.getAsString())
        {
          return tGSTypeInfo(&*j, i->first);
        }
      }
    }
  }
  return tGSTypeInfo(0, BuiltinType::Void);
}

//--------------------------------------------------------- 
QualType Configuration::getSimdType(tTypeInfo info, ASTImporter& target) 
{
  return target.Import(info->second.m_CanonicalSimdType);
}

//--------------------------------------------------------- 
QualType Configuration::getGSIndexType(tTypeInfo info, ASTImporter &target)
{
  return target.Import(info->second.m_GSIndexType);
}

//--------------------------------------------------------- 
unsigned Configuration::getVectorSize(tTypeInfo info)
{
  return info->first;
}

//--------------------------------------------------------- 
unsigned Configuration::getVectorByteAlignment(tTypeInfo info)
{
  return info->second.m_VectorByteAlignment;
}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo::IntrinsicInfo(ASTImporter &C, const IntrinsicInfo& src) :
  m_Type(cast<FunctionDecl>(C.Import(src.m_Type))),
  m_PrintFormat(src.m_PrintFormat)
{}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo Configuration::getGlobalBuiltin(
  const char* pName, ASTImporter &target) const
{
  tIntrinsicInfos::const_iterator i2 = m_GlobalBuiltins.find(pName);
  if (i2 != m_GlobalBuiltins.end())
  {
    return IntrinsicInfo(target, i2->second);
  }
  return IntrinsicInfo();
}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo Configuration::getIntrinsicBuiltin(
  const char* pName, tTypeInfo info, ASTImporter& target) const
{
  tIntrinsicInfos::const_iterator i2 = info->second.m_BuiltinInfos.find(pName);
  if (i2 != info->second.m_BuiltinInfos.end())
  {
    return IntrinsicInfo(target, i2->second);
  }
  return IntrinsicInfo();
}

//--------------------------------------------------------- 
bool Configuration::hasIntrinsicBuiltin(const char* pName, tTypeInfo info) const
{
  return info->second.m_BuiltinInfos.find(pName) != info->second.m_BuiltinInfos.end() ||
         info->second.m_IndexedInfos.find(pName) != info->second.m_IndexedInfos.end();
}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo Configuration::getConvertFn(tTypeInfo targetType, 
  BuiltinType::Kind sourceBaseType, unsigned sourceSize, unsigned index, ASTImporter &C) const
{
  tIntrinsicConvertInfos::const_iterator i = targetType->second.m_ConvertInfos.find(sourceBaseType);
  if (i != targetType->second.m_ConvertInfos.end())
  {
    std::map<unsigned, IntrinsicIndexedInfo>::const_iterator i2 = i->second.find(sourceSize);
    if (i2 != i->second.end())
    {
      return IntrinsicInfo(C, i2->second[index]);
    }
  }
  return IntrinsicInfo();
}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo Configuration::getIndexedFn(const char* pName, 
  tTypeInfo info, unsigned index, ASTImporter &C) const
{
  tIntrinsicIndexedBuiltinInfos::const_iterator i2 = info->second.m_IndexedInfos.find(pName);
  if (i2 != info->second.m_IndexedInfos.end())
  {
    assert(index < i2->second.size());
    return IntrinsicInfo(C, i2->second[index]);
  }
  return IntrinsicInfo();
}

//--------------------------------------------------------- 
bool Configuration::IntrinsicFnInfo::hasCompatibleSourceFnDecl(const FunctionDecl* FD, ASTImporter &C) const
{
  for (llvm::SmallVector<FunctionDecl*, 4>::const_iterator 
       i = m_SourceFnTypes.begin(), e = m_SourceFnTypes.end(); i != e; ++i)
  {
    if (equalTypes((*i)->getResultType(), FD->getResultType(), C) && 
        argumentsMatch(*i, FD, C))
    {
      return true;
    }
  }
  return false;
}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo Configuration::getIntrinsicFn(FunctionDecl* fnDecl, 
  tTypeInfo info, ASTImporter &C) const
{
  llvm::StringRef fnName = getND_Name(fnDecl);
  if (!fnName.empty() )
  {
    tIntrinsicFnInfos::const_iterator i2 = info->second.m_FunctionInfos.find(fnName);
    if (i2 != info->second.m_FunctionInfos.end() && 
        i2->second.hasCompatibleSourceFnDecl(fnDecl, C))
    {
      return IntrinsicInfo(C, i2->second);
    }
  }
  return IntrinsicInfo();
}

//--------------------------------------------------------- 
Configuration::IntrinsicInfo Configuration::getIntrinsicExpr(Expr* E, 
  tTypeInfo info, ASTImporter &C, llvm::SmallVector<Expr*, 4>& subExprs, StmtEditor& editor) const
{
  IntrinsicInfo result;
  std::string bestFit;
  for (tIntrinsicExprInfos::const_iterator i = 
        info->second.m_ExpressionInfos.begin(), 
        e = info->second.m_ExpressionInfos.end(); i != e; ++i)
  {
    llvm::SmallVector<Expr*, 4> tempSubExprs;
    if (matchExpressionString(i->first, E, tempSubExprs, editor))
    {
      // the longest expression is always preferred, 
      // if two have the same length, then the one with 
      // lesser sub expressions (i.e. more merges) is preferred
      if (i->first.size() > bestFit.size() || 
          (i->first.size() == bestFit.size() && 
           subExprs.size() > tempSubExprs.size()))
      {
        bestFit = i->first;
        subExprs.swap(tempSubExprs);
        result = IntrinsicInfo(C, i->second);
      }
    }
  }
  return result;
}

//--------------------------------------------------------- 
bool Configuration::hasIntrinsicFn(const FunctionDecl* fnDecl, ASTImporter &C) const
{
  llvm::StringRef fnName = getND_Name(fnDecl);
  if (!fnName.empty() )
  {
    for (tTypeInfos::const_iterator i = m_TypeIntrinsics.begin(), 
         e = m_TypeIntrinsics.end(); i != e; ++i)
    {
      for (std::map<unsigned, ArchitectureInfo>::const_iterator i2 = i->second.begin(), 
           e2 = i->second.end(); i2 != e2; ++i2)
      {
        tIntrinsicFnInfos::const_iterator i3 = i2->second.m_FunctionInfos.find(fnName);
        if (i3 != i2->second.m_FunctionInfos.end() && 
            i3->second.hasCompatibleSourceFnDecl(fnDecl, C))
        {
          return true;
        }
      }
    }
  }
  return false;
}

//--------------------------------------------------------- 
class ConfigurationBuilder
{
public:
  ConfigurationBuilder(ASTContext &C, llvm::OwningPtr<CompilerInstance>& Compiler);
  void addFromTranslationUnit(Configuration& C);
private:
  void addFromTemplateSpecialization(ClassTemplateSpecializationDecl* RD);
  void addFromConvert(FunctionDecl* FD);
  void addFromGlobalBuiltin(FunctionDecl* FD);
  bool getConvertTypeInfo(QualType T, BuiltinType::Kind& type, unsigned& size) const;
  void cloneParamDecls(FunctionDecl* FD, llvm::SmallVector<ParmVarDecl*, 4>& params);

  DiagnosticBuilder Warn(const SourceRange& r, const char* msg, 
    DiagnosticsEngine::Level L = DiagnosticsEngine::Warning);

  void fillFromEnumDecl(EnumDecl* ED, Configuration::ArchitectureInfo& info);
  CompoundStmt* getBodyStmt(FunctionDecl* MD);
  void fillBuiltin(FunctionDecl* MD, StringLiteral* SL, Configuration::tIntrinsicInfos& info);
  void fillIndexedBuiltin(CXXMethodDecl* MD, CompoundStmt* CS, unsigned uVectorSize, Configuration::ArchitectureInfo& info);

  void fillCustomized(CXXMethodDecl* MD, unsigned uVectorSize, Configuration::ArchitectureInfo& info);

  ASTContext& Ctx;
  DiagnosticsEngine& Diagnostics;
  llvm::OwningPtr<CompilerInstance>& m_Compiler;
  Configuration::tTypeInfos       m_TypeIntrinsics; 
  Configuration::tIntrinsicInfos  m_GlobalBuiltins;
};

//--------------------------------------------------------- 
ConfigurationBuilder::ConfigurationBuilder(ASTContext &C, llvm::OwningPtr<CompilerInstance>& Compiler) :
  Ctx(C),
  Diagnostics(Compiler->getDiagnostics()),
  m_Compiler(Compiler)
{}

//--------------------------------------------------------- 
void ConfigurationBuilder::fillFromEnumDecl(EnumDecl* ED, Configuration::ArchitectureInfo& info)
{
  for (EnumDecl::enumerator_iterator i = ED->enumerator_begin(), 
       e = ED->enumerator_end(); i != e; ++i)
  {
    if (isND_Name(*i, "align"))
    {
      info.m_VectorByteAlignment = (*i)->getInitVal().getLimitedValue();
    }
  }
}

//--------------------------------------------------------- 
DiagnosticBuilder ConfigurationBuilder::Warn(const SourceRange& r, const char* msg, DiagnosticsEngine::Level L)
{
  unsigned diagID = Diagnostics.getCustomDiagID(L, msg);
  return Diagnostics.Report(r.getBegin(), diagID);
}

//--------------------------------------------------------- 
CompoundStmt* ConfigurationBuilder::getBodyStmt(FunctionDecl* MD)
{
  Stmt* S = MD->getBody();
  if (S == 0)
  {
    Warn(MD->getSourceRange(), "function %0 is not a definition, ignored") << MD->getName();
    return 0;
  }
  CompoundStmt* CS = dyn_cast<CompoundStmt>(S);
  if (CS == 0 || CS->size() == 0)
  {
    Warn(S->getSourceRange(), "unknown function definition %0, ignored") << MD->getName();
    return 0;
  }
  return CS;
}

//--------------------------------------------------------- 
void ConfigurationBuilder::cloneParamDecls(FunctionDecl* FD, llvm::SmallVector<ParmVarDecl*, 4>& params)
{
  for (FunctionDecl::param_iterator i = FD->param_begin(), e = FD->param_end(); 
       i != e; ++i)
  {
    params.push_back(ParmVarDecl::Create(Ctx, (*i)->getDeclContext(), 
      (*i)->getLocStart(), (*i)->getLocation(),
      (*i)->getIdentifier(), (*i)->getType(), (*i)->getTypeSourceInfo(), 
      (*i)->getStorageClass(), 0));
  }
}

//--------------------------------------------------------- 
void ConfigurationBuilder::fillIndexedBuiltin(CXXMethodDecl* MD, CompoundStmt* CS, 
  unsigned uVectorSize, Configuration::ArchitectureInfo& info)
{
  llvm::SmallVector<llvm::StringRef, 4> printFormats;
  for (CompoundStmt::body_iterator i = CS->body_begin(), 
       e = CS->body_end(); i != e; ++i)
  {
    if (StringLiteral* SL = dyn_cast<StringLiteral>(*i))
    {
      printFormats.push_back(SL->getString());
    }
    else
    {
      Warn((*i)->getSourceRange(), "unrecognized statement in indexed builtin function, ignored");
    }
  }
  if (printFormats.size() != uVectorSize)
  {
    Warn(CS->getSourceRange(), "number of literals does not match the vector size, ignored");
    return;
  }

  llvm::StringRef name = MD->getName();
  Configuration::IntrinsicIndexedInfo& convertInfo = info.m_IndexedInfos[name];
  convertInfo.resize(printFormats.size());
  for (unsigned i = 0; i < printFormats.size(); ++i)
  {
    convertInfo[i].m_PrintFormat = printFormats[i];
    if (i == 0)
    {
      convertInfo[i].m_Type = MD;
    }
    else
    {
      convertInfo[i].m_Type = CXXMethodDecl::Create(Ctx, MD->getParent(), 
          MD->getSourceRange().getBegin(), MD->getNameInfo(), 
          MD->getType(), MD->getTypeSourceInfo(), SC_None, true, false, 
          MD->getSourceRange().getEnd());

      llvm::SmallVector<ParmVarDecl*, 4> params;
      cloneParamDecls(MD, params);
      convertInfo[i].m_Type->setParams(llvm::ArrayRef<ParmVarDecl*>(params));
      std::stringstream newName;
      newName << name.str() << i;
      const IdentifierInfo *II = &Ctx.Idents.get(llvm::StringRef(newName.str()));
      convertInfo[i].m_Type->setDeclName(DeclarationName(II));
      convertInfo[i].m_Type->setAccess(AS_public);
    }
  }
}

//--------------------------------------------------------- 
void ConfigurationBuilder::fillBuiltin(FunctionDecl* MD, StringLiteral* SL, Configuration::tIntrinsicInfos& info)
{
  Configuration::IntrinsicInfo& entry = info[MD->getName()];
  entry.m_PrintFormat = SL->getString();
  entry.m_Type = MD;
}

//--------------------------------------------------------- 
void ConfigurationBuilder::fillCustomized(CXXMethodDecl* MD, unsigned uVectorSize, Configuration::ArchitectureInfo& info)
{
  DeclStmt* DS;
  FunctionDecl* FD;
  StringLiteral* SL;
  Expr* E;

  CompoundStmt* CS = getBodyStmt(MD);
  if (CS == 0)
  {
    return;
  }
  if ((SL = dyn_cast<StringLiteral>(*CS->child_begin())) != 0)
  {
    CS->size() > 1 ? fillIndexedBuiltin(MD, CS, uVectorSize, info) : 
                     fillBuiltin(MD, SL, info.m_BuiltinInfos);
    return;
  }
  if ((SL = dyn_cast<StringLiteral>(CS->body_back())) == 0)
  {
    Warn(CS->getSourceRange(), "last statement in %0 is not a string literal, ignored") << MD->getName();
    return;
  }
  
  for (CompoundStmt::body_iterator i = CS->body_begin(), 
       e = CS->body_end() - 1; i != e; ++i)
  {
    if ((DS = dyn_cast<DeclStmt>(*i)) != 0 &&
        DS->isSingleDecl() &&
        (FD = dyn_cast<FunctionDecl>(DS->getSingleDecl())) != 0)
    {
      llvm::StringRef sourceFnName = getND_Name(FD);
      if (sourceFnName.empty())
      {
        Warn((*i)->getSourceRange(), "function declaration name not recognized, ignored");
        continue;
      }
      Configuration::IntrinsicFnInfo& entry = info.m_FunctionInfos[sourceFnName];
      entry.m_PrintFormat = SL->getString();
      entry.m_Type = MD;
      entry.m_SourceFnTypes.push_back(FD);
    }
    else if ((E = dyn_cast<Expr>(*i)) != 0)
    {
      const std::string& exprString = buildExpressionString(MD, E);
      if (!exprString.empty())
      {
        Configuration::IntrinsicInfo& entry = info.m_ExpressionInfos[exprString];
        entry.m_PrintFormat = SL->getString();
        entry.m_Type = MD;
      }
      else
      {
        Warn(E->getSourceRange(), "unknown expression, ignored");
      }
    }
    else 
    {
      Warn((*i)->getSourceRange(), "neither function declaration nor expression, ignored");
      continue;
    }
  }
}

//--------------------------------------------------------- 
void ConfigurationBuilder::addFromTemplateSpecialization(ClassTemplateSpecializationDecl* RD)
{
  const TemplateArgumentList& TAL = RD->getTemplateInstantiationArgs();
  if (TAL.size() == 2 && 
      TAL[0].getKind() == TemplateArgument::Type && 
      TAL[1].getKind() == TemplateArgument::Integral)
  {
    QualType vectorType = TAL[0].getAsType();
    const BuiltinType* BT = vectorType.getTypePtr()->getAs<BuiltinType>();
    if (BT == 0)
    {
      Warn(RD->getSourceRange(), "specialization type is not builtin, ignored");
      return;
    }
    
    unsigned uVectorSize = TAL[1].getAsIntegral().getLimitedValue();
    if (uVectorSize == 0)
    {
      Warn(RD->getSourceRange(), "vector size is 0, ignored");
      return;
    }

    Configuration::ArchitectureInfo& typeInfo = m_TypeIntrinsics[BT->getKind()][uVectorSize];

    for (DeclContext::decl_iterator i = RD->decls_begin(),
         e = RD->decls_end(); i != e; ++i)
    {
      if (TypedefDecl* TD = dyn_cast<TypedefDecl>(*i))
      {
        if (isND_Name(TD, "type"))
        {
          Warn(TD->getSourceRange(), "vectorized type name is %0", DiagnosticsEngine::Note) 
            << TD->getTypeSourceInfo()->getType().getAsString();
          typeInfo.m_CanonicalSimdType = TD->getUnderlyingType();
        }
        else if (isND_Name(TD, "gs_index_type"))
        {
          typeInfo.m_GSIndexType = TD->getUnderlyingType();
        }
      }
      else if (EnumDecl* ED = dyn_cast<EnumDecl>(*i))
      {
        fillFromEnumDecl(ED, typeInfo);
      }
      else if (CXXMethodDecl* MD = dyn_cast<CXXMethodDecl>(*i))
      {
        if (MD->getIdentifier() != 0)
        {
          if (MD->isStatic() && !getND_Name(MD).empty())
          {
            fillCustomized(MD, uVectorSize, typeInfo);
          }
          else
          {
            Warn(MD->getSourceRange(), "non-static method definition %0, ignored") << MD->getName();
          }
        }
      }
    }
  }
}

//--------------------------------------------------------- 
bool ConfigurationBuilder::getConvertTypeInfo(QualType T, BuiltinType::Kind& type, unsigned& size) const
{
  const ElaboratedType* targetType = T->getAs<ElaboratedType>();
  if (targetType)
  {
    const Type* qualifierClass = targetType->getQualifier()->getAsType();
    if (const TemplateSpecializationType* TS = dyn_cast<TemplateSpecializationType>(qualifierClass))
    {
      if (TS->getNumArgs() == 2 && 
          TS->getArg(0).getKind() == TemplateArgument::Type && 
          (TS->getArg(1).getKind() == TemplateArgument::Integral ||
           TS->getArg(1).getKind() == TemplateArgument::Expression))
      {
        const BuiltinType* BT = TS->getArg(0).getAsType()->getAs<BuiltinType>();
        if (BT != 0)
        {
          type = BT->getKind();
          if (TS->getArg(1).getKind() == TemplateArgument::Integral)
          {
            size = TS->getArg(1).getAsIntegral().getLimitedValue();
          }
          else
          {
            llvm::APSInt Result;
            if (TS->getArg(1).getAsExpr()->EvaluateAsInt(Result, Ctx))
            {
              size = Result.getLimitedValue();
            }
            else
            {
              size = 0;
            }
          }
          return size > 0;
        }
      }
    }
  }
  return false;
}

//--------------------------------------------------------- 
void ConfigurationBuilder::addFromGlobalBuiltin(FunctionDecl* FD)
{
  llvm::StringRef fnName = getND_Name(FD);
  if (fnName.empty())
  {
    Warn(FD->getSourceRange(), "unrecognized function name, ignored");
    return;
  }
  CompoundStmt* CS = getBodyStmt(FD);
  if (CS != 0)
  {
    if (StringLiteral* SL = dyn_cast<StringLiteral>(*CS->child_begin()))
    {
      fillBuiltin(FD, SL, m_GlobalBuiltins);
    }
    else
    {
      Warn(CS->getSourceRange(), "first statement in %0 is not a string literal, ignored") << FD->getName();
    }
  }
}

//--------------------------------------------------------- 
void ConfigurationBuilder::addFromConvert(FunctionDecl* FD)
{
  CompoundStmt* CS = getBodyStmt(FD);
  if (CS == 0)
  {
    Warn(FD->getSourceRange(), "unrecognized body for %0, ignored") << FD->getName();
    return;
  }

  BuiltinType::Kind sourceType, targetType, checkType;
  unsigned uSourceVectorSize, uTargetVectorSize, uCheckSize;
  if (!getConvertTypeInfo(FD->getResultType(), targetType, uTargetVectorSize))
  {
    Warn(FD->getSourceRange(), "could not retrieve vector type infos from return type, ignored");
    return;
  }
  unsigned numParams = FD->getNumParams();
  if (numParams == 0)
  {
    Warn(FD->getSourceRange(), "no arguments in conversion function, ignored");
    return;
  }

  QualType argType = FD->getParamDecl(0)->getType();
  if (!getConvertTypeInfo(argType, sourceType, uSourceVectorSize))
  {
    Warn(FD->getSourceRange(), "could not retrieve vector type infos from argument type, ignored");
    return;
  }

  for (unsigned i = 1; i < numParams; ++i)
  {
    if ((!getConvertTypeInfo(FD->getParamDecl(i)->getType(), checkType, uCheckSize)) ||
        sourceType != checkType ||
        uSourceVectorSize != uCheckSize)
    {
      Warn(FD->getSourceRange(), "arguments in conversion function have different types, ignored");
      return;
    }
  }

  llvm::SmallVector<llvm::StringRef, 4> printFormats;
  for (CompoundStmt::body_iterator i = CS->body_begin(), 
       e = CS->body_end(); i != e; ++i)
  {
    if (StringLiteral* SL = dyn_cast<StringLiteral>(*i))
    {
      printFormats.push_back(SL->getString());
    }
    else
    {
      Warn((*i)->getSourceRange(), "unrecognized statement in convert function, ignored");
    }
  }

  if (uTargetVectorSize >= uSourceVectorSize)
  {
    if (uTargetVectorSize % uSourceVectorSize != 0)
    {
      Warn(FD->getSourceRange(), "source and target vector size must be multiples, ignored");
      return;
    }
  }
  else
  {
    if (uSourceVectorSize % uTargetVectorSize != 0)
    {
      Warn(FD->getSourceRange(), "source and target vector size must be multiples, ignored");
      return;
    }
  }

  if (printFormats.size() != std::max(uSourceVectorSize / uTargetVectorSize, 1u))
  {
    Warn(FD->getSourceRange(), "number of literals does not match the source and target vector size, ignored");
    return;
  }

  if (numParams != std::max(uTargetVectorSize / uSourceVectorSize, 1u))
  {
    Warn(FD->getSourceRange(), "number of arguments does not match the source and target vector size, ignored");
    return;
  }

  Configuration::IntrinsicIndexedInfo& convertInfo = 
    m_TypeIntrinsics[targetType][uTargetVectorSize].
    m_ConvertInfos[sourceType][uSourceVectorSize];

  convertInfo.resize(printFormats.size());
  for (unsigned i = 0; i < printFormats.size(); ++i)
  {
    convertInfo[i].m_PrintFormat = printFormats[i];
    if (i == 0)
    {
      convertInfo[i].m_Type = FD;
    }
    else
    {
      convertInfo[i].m_Type = FunctionDecl::Create(Ctx, FD->getDeclContext(), 
          FD->getSourceRange().getBegin(), FD->getNameInfo(), 
          FD->getType(), FD->getTypeSourceInfo(), SC_None, false, false);

      llvm::SmallVector<ParmVarDecl*, 4> params;
      cloneParamDecls(FD, params);
      convertInfo[i].m_Type->setParams(llvm::ArrayRef<ParmVarDecl*>(params));
    }
  }
}

//--------------------------------------------------------- 
void ConfigurationBuilder::addFromTranslationUnit(Configuration& C)
{
  NamespaceDecl* ND;
  ClassTemplateSpecializationDecl* RD;
  FunctionDecl* FD;
  for (DeclContext::decl_iterator iNS = Ctx.getTranslationUnitDecl()->decls_begin(), 
       eNS = Ctx.getTranslationUnitDecl()->decls_end(); iNS != eNS; ++iNS)
  {
    if ((ND = dyn_cast<NamespaceDecl>(*iNS)) != 0 && 
        isND_Name(ND, "scout"))
    {
      for (DeclContext::decl_iterator i = ND->decls_begin(),
           e = ND->decls_end(); i != e; ++i)
      {
        if ((RD = dyn_cast<ClassTemplateSpecializationDecl>(*i)) != 0 && 
            RD->getDefinition() == RD && 
            RD->getTemplateSpecializationKind() == TSK_ExplicitSpecialization &&
            isND_Name(RD, "config"))
        {
          addFromTemplateSpecialization(RD);
        }
        else if ((FD = dyn_cast<FunctionDecl>(*i)) != 0 && 
                 FD->isThisDeclarationADefinition())
        {
          if (isND_Name(FD, "convert"))
          {
            addFromConvert(FD);
          }
          else
          {
            addFromGlobalBuiltin(FD);
          }
        }
      }
    }
  }
  if (m_TypeIntrinsics.empty())
  {
    unsigned diagID = Diagnostics.getCustomDiagID(
      DiagnosticsEngine::Error, "no valid intrinsic info in configuration found");
    Diagnostics.Report(SourceLocation(), diagID);
    return;
  }

  if (!Diagnostics.hasErrorOccurred())
  {
    C.m_TypeIntrinsics.swap(m_TypeIntrinsics); 
    C.m_GlobalBuiltins.swap(m_GlobalBuiltins);
    C.m_ASTContext.reset(&m_Compiler->getASTContext());
    C.m_FileManager.reset(&m_Compiler->getFileManager());
    C.m_FileSystemOpts = m_Compiler->getFileSystemOpts();

    m_Compiler->resetAndLeakASTContext();
    m_Compiler->resetAndLeakFileManager();
  }
}

//--------------------------------------------------------- 
void ParseConfigurationConsumer::HandleTranslationUnit(ASTContext &C)
{
  ConfigurationBuilder builder(C, m_Compiler);
  builder.addFromTranslationUnit(m_Configuration);
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang
