#ifndef APPLICATION_APP_H
#define APPLICATION_APP_H

#include <iosfwd>
#include <vector>
#include <list>

//--------------------------------------------------------- 
namespace HICFD {
  namespace APP {

//--------------------------------------------------------- forwards
class Preferences;
class Project;

// the Application NS mimics a singleton class Application
// the approach used here enables us to move member variables in
// the .cpp file and thus minimize the dependencies
namespace Application {

//--------------------------------------------------------- 
bool initialize_CLI(int argc, char *argv[],
                    std::vector<const char*>& cmdLine,
                    const char*& extension);
void  printHelp_CLI();

void initializeConfiguration();
void destroyConfiguration();

bool loadConfiguration(const std::string& configPath, std::ostream& diagnostics);

//--------------------------------------------------------- 
std::string getScoutFullVersionInfo();
Project& getCurrentProject();
const std::string& getConfigurationPath();

//--------------------------------------------------------- source processing

bool processFile(const char *sourceStartPtr, const char *sourceEndPtr,
                 const char* pFileName, const std::vector<const char*>& Args, 
                 std::string& targetFile, std::ostream& diagnostics);

void addCmdArgsFromProject(std::vector<const char*>& Args, std::list<std::string>& commandLine);
void buildCommandLineFromProject(std::string& cmdLine);

//--------------------------------------------------------- 

} // namespace Application 

  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // APPLICATION_APP_H
