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

#ifndef SCOUT_CLANGADDONS_INTERFACE_H
#define SCOUT_CLANGADDONS_INTERFACE_H

//------------------------------------------------------------------------- 

#include <ostream>
#include <istream>
#include <list>
#include <vector>
#include <utility>

//------------------------------------------------------------------------- 

namespace clang {

//--------------------------------------------------------- 
namespace ASTProcessing { class Configuration; }

//------------------------------------------------------------------------- 
bool processSource(const char *sourceStartPtr, 
                   const char *sourceEndPtr,
                   const char *sourceFileName,
                   std::vector<const char*> Args,
                   std::ostream& targetFile, 
                   std::ostream& diagnostics,
                   const std::string& prologText,
                   ASTProcessing::Configuration& configuration);

//--------------------------------------------------------- 
typedef std::vector< std::pair<std::string, std::string> > tInOutFileList;

bool retrieveFilesFromArg(std::vector<const char*>& Args, 
                          std::ostream& diagnostics,
                          const char* Extension,
                          tInOutFileList& fileList);

//------------------------------------------------------------------------- 
bool processConfiguration(const char *sourceFileName,
                          ASTProcessing::Configuration& configuration, 
                          std::ostream& diagnostics);

ASTProcessing::Configuration* createConfiguration();
void destroyConfiguration(ASTProcessing::Configuration*);

//------------------------------------------------------------------------- 
std::string getClangAddonsVersion();

//------------------------------------------------------------------------- 
} // namespace clang
 
//------------------------------------------------------------------------- 

#endif // SCOUT_CLANGADDONS_INTERFACE_H
