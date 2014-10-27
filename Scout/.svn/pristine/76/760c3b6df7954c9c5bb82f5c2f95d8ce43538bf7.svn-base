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

#ifndef SCOUT_CLANGADDONS_VECTORIZING_CONFIGURATION_H
#define SCOUT_CLANGADDONS_VECTORIZING_CONFIGURATION_H

#include "clang/AST/ASTConsumer.h"    
#include "clang/AST/Type.h"    
#include "clang/Basic/FileSystemOptions.h"
#include "llvm/ADT/DenseMap.h"    
#include "llvm/ADT/OwningPtr.h"    
#include <string>
#include <utility>
#include <map>

//--------------------------------------------------------- 
namespace clang {

class Diagnostic;
class ClassTemplateSpecializationDecl;
class FunctionDecl;
class Expr;
class CompilerInstance;
class ASTImporter;
class FileManager;

//--------------------------------------------------------- 
namespace ASTProcessing {

//------------------------------------------------------------------------- 
class ConfigurationBuilder;
class StmtEditor;

//------------------------------------------------------------------------- 
class Configuration
{
public:

  struct IntrinsicInfo
  {
    FunctionDecl*   m_Type;         // target function declaration
    llvm::StringRef m_PrintFormat;  // string literal from cfg  
    IntrinsicInfo() : m_Type(0) {}
    IntrinsicInfo(ASTImporter &C, const IntrinsicInfo& src);
  };

private:
  struct IntrinsicFnInfo : IntrinsicInfo
  {
    llvm::SmallVector<FunctionDecl*, 4> m_SourceFnTypes; // source function types
    bool hasCompatibleSourceFnDecl(const FunctionDecl* fnDecl, ASTImporter &C) const;
  }; 

  typedef llvm::SmallVector<IntrinsicInfo, 4> IntrinsicIndexedInfo;

  typedef std::map<llvm::StringRef, IntrinsicInfo> tIntrinsicInfos;
  typedef std::map<llvm::StringRef, IntrinsicFnInfo> tIntrinsicFnInfos;
  typedef std::map<std::string, IntrinsicInfo> tIntrinsicExprInfos;
  typedef std::map<BuiltinType::Kind, std::map<unsigned, IntrinsicIndexedInfo> > tIntrinsicConvertInfos;
  typedef std::map<llvm::StringRef, IntrinsicIndexedInfo> tIntrinsicIndexedBuiltinInfos;


  struct ArchitectureInfo
  {
    unsigned m_VectorByteAlignment;
    QualType m_CanonicalSimdType;           // type of vectorized variables
    QualType m_GSIndexType;                 // type of gather/scatter index

    tIntrinsicInfos m_BuiltinInfos;         // key is builtin name
    tIntrinsicExprInfos m_ExpressionInfos;  // key is expression AST in internal text format 
    tIntrinsicFnInfos m_FunctionInfos;      // key is function name
    tIntrinsicConvertInfos m_ConvertInfos;  
    tIntrinsicIndexedBuiltinInfos m_IndexedInfos; // key is builtin name
  };

public:
  typedef std::map<unsigned, ArchitectureInfo> tArchitectureInfoMap;
  typedef const tArchitectureInfoMap::value_type* tTypeInfo;
  typedef std::pair<tTypeInfo, BuiltinType::Kind> tGSTypeInfo;
  tTypeInfo getTypeInfo(BuiltinType::Kind baseType, unsigned exclMaxSize);
  tGSTypeInfo getGSTypeInfo(tTypeInfo baseType);

  static QualType getSimdType(tTypeInfo info, ASTImporter &target);
  static QualType getGSIndexType(tTypeInfo info, ASTImporter &target);

  static unsigned getVectorSize(tTypeInfo info);
  static unsigned getVectorByteAlignment(tTypeInfo info);

  IntrinsicInfo getGlobalBuiltin(const char* pName, ASTImporter &C) const;
  IntrinsicInfo getIntrinsicBuiltin(const char* pName, tTypeInfo info, ASTImporter &C) const;
  IntrinsicInfo getIntrinsicFn(FunctionDecl* fnDecl, tTypeInfo info, ASTImporter &C) const;
  IntrinsicInfo getIntrinsicExpr(Expr* E, tTypeInfo info, ASTImporter &C, llvm::SmallVector<Expr*, 4>& subExprs, StmtEditor& e) const;
  IntrinsicInfo getConvertFn(tTypeInfo targetType, BuiltinType::Kind sourceBaseType, 
                             unsigned sourceSize, unsigned index, ASTImporter &C) const;
  IntrinsicInfo getIndexedFn(const char* pFnName, tTypeInfo info, unsigned index, ASTImporter &C) const;

  ASTContext* getASTContext() { return m_ASTContext.get(); }
  FileManager* getFileManager() { return m_FileManager.get(); }
  const FileSystemOptions &getFileSystemOpts() const { return m_FileSystemOpts; }
  bool hasIntrinsicFn(const FunctionDecl* fnDecl, ASTImporter &C) const;
  bool hasIntrinsicBuiltin(const char* pName, tTypeInfo info) const;

private:

  typedef std::map<BuiltinType::Kind, tArchitectureInfoMap> tTypeInfos;

  tTypeInfos      m_TypeIntrinsics; // key is base type
  tIntrinsicInfos m_GlobalBuiltins; // key is function name

  llvm::OwningPtr<ASTContext>  m_ASTContext;
  llvm::OwningPtr<FileManager> m_FileManager;
  FileSystemOptions            m_FileSystemOpts;

  friend class ConfigurationBuilder;
};

//------------------------------------------------------------------------- 
class ParseConfigurationConsumer : public ASTConsumer 
{
public:
  ParseConfigurationConsumer(Configuration& config, 
                             llvm::OwningPtr<CompilerInstance>& c) : 
    m_Configuration(config),
    m_Compiler(c) {}
private:
  virtual void HandleTranslationUnit(ASTContext &C);

  Configuration&  m_Configuration;
  llvm::OwningPtr<CompilerInstance>&  m_Compiler;
};


//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_VECTORIZING_CONFIGURATION_H
