#ifndef GUIAPPLICATION_APP_H
#define GUIAPPLICATION_APP_H

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
struct tGuiFunctions
{
  virtual void log(const std::string& text) = 0;
  virtual bool displaySourceFile(const std::string& fileName) = 0;
  virtual std::string getConfigFile(const std::string& configName) = 0;
};

bool initialize(tGuiFunctions* guiFunctions, const char* homeDir, std::ostream& diagnostics); 
void destroy();

//--------------------------------------------------------- logging
void log(const std::string& msg);

//--------------------------------------------------------- Preferences
Preferences& getPreferences();
void loadPreferences();
void savePreferences();

//--------------------------------------------------------- Project
const std::string& getCurrentProjectFile();
bool loadProject(const std::string& projectFile, std::ostream& diagnostics);
void saveProject(const std::string& projectFile);
void setLastLoadedFile(const std::string& srcFile);

void newProject();

//--------------------------------------------------------- 

} // namespace Application 

  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // GUIAPPLICATION_APP_H
