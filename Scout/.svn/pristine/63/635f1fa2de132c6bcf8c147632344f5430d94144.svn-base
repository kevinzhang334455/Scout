#include "clang/Interface/Application.h"
#include "clang/Interface/Project.h"
#include "clang/Interface/Interface.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <boost/version.hpp>
#include <boost/config.hpp>
#include <boost/tokenizer.hpp>
#include <string.h>

//---------------------------------------------------------
#ifdef BOOST_NO_EXCEPTIONS
namespace boost {

void throw_exception( std::exception const & e ) {}

}
#endif

//--------------------------------------------------------- 
namespace HICFD {
  namespace APP {
    namespace Application {

Project         s_currentProject;

//--------------------------------------------------------- 
namespace {

clang::ASTProcessing::Configuration* s_Configuration;
std::string     s_ConfigurationPath;

struct tCliParamter
{
  const char* name;
  const char* description;

  enum { eKonfiguration, eProlog, ePreprocess, eExtension, eNumParameters };
};

const tCliParamter s_Paramters[tCliParamter::eNumParameters] = {
  { "scout:configuration", "full_path: Scout configuration file" },
  { "scout:prolog",        "full_path: text file to add in target text (after includes)" },
  { "scout:preprocess",    "full_path: included source file(s) for function definitions" },
  { "scout:extension",     "ext: target file(s) extension" }
};

} // anon namespace

//--------------------------------------------------------- 
bool loadConfiguration(const std::string& configPath, std::ostream& diagnostics)
{
  if (configPath.empty())
  {
    diagnostics << "error: no configuration file defined\n";
    return false;
  }

  if (!clang::processConfiguration(configPath.c_str(), *s_Configuration, diagnostics))
  {
    diagnostics << "error(s) during configuration processing\n";
    return false;
  }
  s_ConfigurationPath = configPath;
  return true;
}

//--------------------------------------------------------- 
Project& getCurrentProject()
{
  return s_currentProject;
}

//--------------------------------------------------------- 
const std::string& getConfigurationPath()
{
  return s_ConfigurationPath;
}

//--------------------------------------------------------- 
void splitCommandline(std::list<std::string>& commandLine, 
                      const std::string& additionalCommandLine)
{
  using namespace boost;
  escaped_list_separator<char> sep('\\', ' ', '\"');
  tokenizer< escaped_list_separator<char> > tok(additionalCommandLine, sep);
  std::copy(tok.begin(), tok.end(), std::back_inserter(commandLine));
}

//--------------------------------------------------------- 
void addCmdArgsFromProject(std::vector<const char*>& Args, 
                           std::list<std::string>& commandLine)
{
  commandLine = s_currentProject.getIncludePaths();
  for (std::list<std::string>::iterator i = commandLine.begin(), 
       e = commandLine.end(); i != e; ++i)
  {
    i->insert(0, "-I");
  }
  splitCommandline(commandLine, s_currentProject.getCommandLineParameters());

  for (std::list<std::string>::iterator i = commandLine.begin(), 
       e = commandLine.end(); i != e; ++i)
  {
    Args.push_back(i->c_str());
  }
}

//--------------------------------------------------------- 
void buildCommandLineFromProject(std::string& cmdLine)
{
  std::stringstream cmdLineStream;

  cmdLineStream << s_currentProject.getCommandLineParameters();
  const std::list<std::string>& incPaths = s_currentProject.getIncludePaths();
  for (std::list<std::string>::const_iterator i = incPaths.begin(), 
       e = incPaths.end(); i != e; ++i)
  {
    cmdLineStream << " -I" << *i;
  }

  cmdLineStream << " -" << s_Paramters[tCliParamter::eKonfiguration].name 
                << "=" << s_ConfigurationPath;

  if (!s_currentProject.getPrologTextFile().empty())
  {
    cmdLineStream << " -" << s_Paramters[tCliParamter::eProlog].name 
                  << "=" << s_currentProject.getPrologTextFile();
  }
  const std::list<std::string>& preprocessedFiles = s_currentProject.getPreprocessedFiles();
  for (std::list<std::string>::const_iterator i = preprocessedFiles.begin(), 
       e = preprocessedFiles.end(); i != e; ++i)
  {
    cmdLineStream << " -" << s_Paramters[tCliParamter::ePreprocess].name 
                  << "=" << *i;
  }
  cmdLine = cmdLineStream.str();
}

//--------------------------------------------------------- 
bool processFile(const char *sourceStartPtr, const char *sourceEndPtr,
                 const char* pFileName, const std::vector<const char*>& Args,
                 std::string& targetFile, std::ostream& diagnostics)
{
  std::string completeSource;
  const std::list<std::string>& preprocessedFiles = s_currentProject.getPreprocessedFiles();
  for (std::list<std::string>::const_iterator i = preprocessedFiles.begin(), 
       e = preprocessedFiles.end(); i != e; ++i)
  {
    if (*i != pFileName)  // [#717]: don't self-preprocess 
    {
      completeSource += "#include \"";
      completeSource += *i;
      completeSource += "\"\n";
    }
  }
  completeSource += "#line 1\n";
  completeSource.append(sourceStartPtr, sourceEndPtr);

  std::string prologText;
  if (!s_currentProject.getPrologTextFile().empty())
  {
    std::ifstream ifs(s_currentProject.getPrologTextFile().c_str());
    if (ifs)
    {
      std::getline(ifs, prologText, '\0'); 	
    }
  }

  std::stringstream processedFile;
  bool result = clang::processSource(//sourceStartPtr, sourceEndPtr,
                       completeSource.data(), 
                       completeSource.data() + completeSource.size(), 
                       pFileName, 
                       Args,
                       processedFile, diagnostics, prologText, *s_Configuration);

  const std::string& processedStr = processedFile.str();
  const char* pMsg = processedStr.c_str();
  const char* pEnd = pMsg + processedStr.size();
  unsigned linesToIgnore = preprocessedFiles.size() + 1;
  for (; *pMsg != 0 && linesToIgnore > 0; ++pMsg)
  {
    if (*pMsg == '\n')
    {
      --linesToIgnore;
    }
  }
  targetFile.assign(pMsg, pEnd);
  return result;
}


//--------------------------------------------------------- 
const char* getCliParameter(const char* arg, const char* name)
{
  if (arg[0] == '-')
  {
    size_t name_len = strlen(name); 
    if (strncmp(arg+1, name, name_len) == 0 && 
        arg[name_len+1] == '=')
    {
      return arg + name_len + 2;
    }
  }
  return 0;
}


//--------------------------------------------------------- 
void printHelp_CLI()
{
  std::cerr << "\nscout command line paramters:\n";
  for (int j = 0; j < tCliParamter::eNumParameters; ++j)
  {
    std::cerr << "-" << s_Paramters[j].name << 
                 "=" << s_Paramters[j].description << "\n";
  }
  std::cerr << "\nAll other command line paramters are redirected to clang.";
  std::cerr << "\nFor clang command line paramters type \"scout -help\"\n";
}

//--------------------------------------------------------- 
bool retrieveParameter(const char *argv, const char*& extension)
{
  for (int j = 0; j < tCliParamter::eNumParameters; ++j)
  {
    if (const char* parameter = getCliParameter(argv, s_Paramters[j].name))
    {
      switch (j)
      {
      case tCliParamter::eKonfiguration:
        s_ConfigurationPath = parameter;
        //s_currentProject.setConfiguration(parameter);
        return true;
      case tCliParamter::eProlog:
        s_currentProject.setPrologTextFile(parameter);
        return true;
      case tCliParamter::ePreprocess:
        s_currentProject.addPreprocessedFile(parameter);
        return true;
      case tCliParamter::eExtension:
        extension = parameter;
        return true;
      default:
        assert(0);
        break;
      }
    }
  }
  return false;
}

//--------------------------------------------------------- 
bool initialize_CLI(int argc, char *argv[],
                    std::vector<const char*>& cmdLine,
                    const char*& extension)
{
  assert(argc > 0);
  cmdLine.push_back(argv[0]);

  initializeConfiguration();

  extension = 0;
  for (int i = 1; i < argc; ++i)
  {
    if (!retrieveParameter(argv[i], extension))
    {
      cmdLine.push_back(argv[i]);
    }
  }

  //return loadConfiguration(s_currentProject.getConfiguration(), std::cerr);
  return loadConfiguration(s_ConfigurationPath, std::cerr);
}

//--------------------------------------------------------- 
void initializeConfiguration()
{
  s_Configuration = clang::createConfiguration();
}

//--------------------------------------------------------- 
void destroyConfiguration()
{
  clang::destroyConfiguration(s_Configuration);
}

//  SCOUT_VERSION % 100 is the patch level
//  SCOUT_VERSION / 100 % 1000 is the minor version
//  SCOUT_VERSION / 100000 is the major version

#define SCOUT_VERSION 100600

//--------------------------------------------------------- 
std::stringstream& version_to_stream (std::stringstream& versionInfo, unsigned int version)
{
  versionInfo << version / 100000 << "." << version / 100 % 1000 << "." << version % 100;
  return versionInfo;
}

//--------------------------------------------------------- 
std::string getScoutFullVersionInfo()
{
  std::stringstream versionInfo;
  versionInfo << "Scout\nversion ";
  version_to_stream(versionInfo, SCOUT_VERSION) << "\nBoost version ";
  version_to_stream(versionInfo, BOOST_VERSION) << "\n"
              << clang::getClangAddonsVersion();
  return versionInfo.str();
}

//--------------------------------------------------------- 
    } // namespace Application 
  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

