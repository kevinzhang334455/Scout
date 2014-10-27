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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_ARTIFICIALIDENTIFIERPOLICY_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_ARTIFICIALIDENTIFIERPOLICY_H

#include <string>
#include <sstream>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- forwards
class ASTContext;
class IdentifierInfo;
class LabelStmt;
class NamedDecl;

//--------------------------------------------------------- 
class ArtificialIdentifierPolicy
{
public:
  std::string LabelPrefix, LabelAlternate, LabelSuffix;
  std::string VarPrefix, VarAlternate, VarSuffix;

  // if set and a original variable is given, the result is Prefix_VarName_PostFix_Count (without underscores)
  // otherwise the result is VarAlternate_Count (without underscores)
  unsigned UseOriginalLabelName : 1;

  // if set and a original label is given, the result is Prefix_LabelName_PostFix_Count (without underscores)
  // otherwise the result is LabelAlternate_Count (without underscores)
  unsigned UseOriginalVarName : 1;

  // initializes all suffix with commonSuffix, all alternates with "art_" + commonSuffix, 
  // leaves prefix empty and uses original names, when possible:
  explicit ArtificialIdentifierPolicy(const std::string& commonSuffix);

  // the pointers may be 0 meaning there is no appropriate original variable 
  IdentifierInfo *createVariable(ASTContext& Ctx, const NamedDecl* originalVar) const;
  IdentifierInfo *createVariable(ASTContext& Ctx, const std::string& originalSuffix) const;
  IdentifierInfo *createLabel(ASTContext& Ctx, const LabelStmt* originalLabel) const;

  static void resetCounter();

private:
  IdentifierInfo *createInfo(ASTContext& Ctx, std::stringstream& nameStream) const;

  static unsigned s_Count;
};

//--------------------------------------------------------- 
} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_ARTIFICIALIDENTIFIERGENERATOR_H
