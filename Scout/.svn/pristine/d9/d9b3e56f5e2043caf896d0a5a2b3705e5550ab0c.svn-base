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

#include "clang/ASTProcessing/ArtificialIdentifierPolicy.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/ASTContext.h"

//--------------------------------------------------------- 
namespace clang {

unsigned ArtificialIdentifierPolicy::s_Count = 0;

//--------------------------------------------------------- 
ArtificialIdentifierPolicy::ArtificialIdentifierPolicy(const std::string& commonSuffix) :
  LabelSuffix(commonSuffix),
  VarSuffix(commonSuffix),
  UseOriginalLabelName(1),
  UseOriginalVarName(1)
{
  LabelAlternate = "art_" + commonSuffix;
  VarAlternate = "art_" + commonSuffix;
}

//--------------------------------------------------------- 
IdentifierInfo *ArtificialIdentifierPolicy::createInfo(ASTContext& Ctx, std::stringstream& nameStream) const
{

  nameStream << s_Count++;
  return &Ctx.Idents.get(llvm::StringRef(nameStream.str()));
}

//--------------------------------------------------------- 
void ArtificialIdentifierPolicy::resetCounter()
{
  s_Count = 0;
}

//--------------------------------------------------------- 
namespace {

std::string getOriginalNameWithSuffix(const std::string& originalName, const std::string& suffix)
{
  std::string::size_type pos = originalName.rfind(suffix);
  if (pos != std::string::npos && 
      pos + suffix.size() < originalName.size() &&
      originalName.find_first_not_of("0123456789", pos + suffix.size()) == std::string::npos)
  {  
    return originalName.substr(0, pos + suffix.size());
  }
  std::string result = originalName;
  if (result.empty() || (*result.rbegin() != '_'))
  {
    result.push_back('_');
  }
  return result + suffix;
}

}
//--------------------------------------------------------- 
IdentifierInfo *ArtificialIdentifierPolicy::createVariable(ASTContext& Ctx, const NamedDecl* originalVar) const
{
  std::stringstream newName;
  if (UseOriginalVarName != 0 && originalVar != NULL)
  {
    newName << VarPrefix << getOriginalNameWithSuffix(originalVar->getName().str(), VarSuffix);
  }
  else
  {
    newName << VarAlternate;
  }
  return createInfo(Ctx, newName);     
}

//--------------------------------------------------------- 
IdentifierInfo *ArtificialIdentifierPolicy::createVariable(ASTContext& Ctx, const std::string& originalSuffix) const
{
  std::stringstream newName;
  if (UseOriginalVarName != 0)
  {
    newName << VarPrefix << getOriginalNameWithSuffix(originalSuffix, VarSuffix);
  }
  else
  {
    newName << VarAlternate;
  }
  return createInfo(Ctx, newName);     
}

//--------------------------------------------------------- 
IdentifierInfo *ArtificialIdentifierPolicy::createLabel(ASTContext& Ctx, const LabelStmt* originalLabel) const
{
  std::stringstream newName;
  if (UseOriginalLabelName != 0 && originalLabel != NULL)
  {
    newName << LabelPrefix << getOriginalNameWithSuffix(originalLabel->getName(), LabelSuffix);
  }
  else
  {
    newName << LabelAlternate;
  }
  return createInfo(Ctx, newName);     
}

//--------------------------------------------------------- 
} // ns clang
