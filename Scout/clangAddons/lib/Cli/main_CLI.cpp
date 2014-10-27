#include "clang/Interface/Application.h"
#include "clang/Interface/Interface.h"
#include <sstream>
#include <iostream>
#include <fstream>


//--------------------------------------------------------- 
int main(int argc, char *argv[])
{
  using namespace HICFD;

  std::cerr << "This is " << APP::Application::getScoutFullVersionInfo() << "\n";

  std::vector<const char*> cmdLine;
  const char* extension;
  clang::tInOutFileList fileList;

  bool scoutCmdLineOK = APP::Application::initialize_CLI(argc, argv, cmdLine, extension);
  bool clangCmdLineOK = clang::retrieveFilesFromArg(cmdLine, std::cerr, extension, fileList);
  if (!(scoutCmdLineOK && clangCmdLineOK))
  {
    APP::Application::printHelp_CLI();
    return 1;
  }

  int result = 0;
  for (clang::tInOutFileList::const_iterator i = fileList.begin(), 
       e = fileList.end(); i != e; ++i)
  {
    std::string sourceText;
    if (i->first == "-")
    {
      std::getline(std::cin, sourceText, '\0'); 	
    }
    else
    {
      std::ifstream ifs(i->first.c_str());
      if (ifs)
      {
        std::getline(ifs, sourceText, '\0'); 	
      }
      else
      {
        std::cerr << "scout error: could not load " << i->first << "\n";
        result = 1;
        continue;
      }
    }

    std::string targetText;
    if (!APP::Application::processFile(sourceText.c_str(), 
      sourceText.c_str() + sourceText.size(),
      i->first.c_str(), cmdLine, targetText, std::cerr))
    {
      result = 1;
      continue;
    }

    if (i->second == "-")
    {
      std::cout << targetText;
    }
    else
    {
      std::ofstream ofs(i->second.c_str());
      if (ofs)
      {
        ofs << targetText;
      }
      else
      {
        std::cerr << "scout error: could not write to " << i->second << "\n";
        result = 1;
      }
    }
  }

  return result;
}
