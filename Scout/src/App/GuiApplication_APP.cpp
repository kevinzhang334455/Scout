#include "App/GuiApplication_APP.h"
#include "App/Preferences_APP.h"
#include "App/ProjectLoader_APP.h"
#include "clang/Interface/Application.h"
#include "clang/Interface/Project.h"
#include <fstream>
#include <assert.h>

//--------------------------------------------------------- 
namespace HICFD {
  namespace APP {
    namespace Application {

//--------------------------------------------------------- 
namespace {

Preferences     s_Preferences;
tGuiFunctions*  s_GuiFunctions = 0;
std::string     s_HomeDir;

} // anon namespace


//--------------------------------------------------------- Preferences
Preferences& getPreferences()
{
  return s_Preferences;
}

//--------------------------------------------------------- 
void loadPreferences()
{
  s_Preferences.load();
}

//--------------------------------------------------------- 
void savePreferences()
{
  s_Preferences.save();
}

//--------------------------------------------------------- 
void setLastLoadedFile(const std::string& srcFile)
{
  s_Preferences.setLastLoadedFile(srcFile);
}

//--------------------------------------------------------- 
bool loadProject(const std::string& projectFile, std::ostream& diagnostics)
{
  Project& P = getCurrentProject();
  if (loadProject(P, projectFile))
  {
    s_Preferences.setLastLoadedProject(projectFile);
    const std::string& lastLoadedFile = P.getLastLoadedFile();
    if (!lastLoadedFile.empty()) 
    {
      s_GuiFunctions->displaySourceFile(lastLoadedFile);
    }
    const std::string& prologTextFile = P.getPrologTextFile();
    bool result = true;
    if (!prologTextFile.empty()) 
    {
      std::ifstream ifs(prologTextFile.c_str());
      if (!ifs)
      {
        diagnostics << "warning: missing prolog text file " << prologTextFile << "\n";
        result = false;
      }
    }

    return loadConfiguration(
        s_GuiFunctions->getConfigFile(s_Preferences.getConfiguration()), 
        diagnostics) && result;
  }
  else
  {
    diagnostics << "error: could not load project " 
                << projectFile << ", file is missing or corrupt\n";
    return false;
  }
}

//--------------------------------------------------------- 
void newProject()
{
  getCurrentProject() = Project();
  s_Preferences.setLastLoadedProject(std::string());
}

//--------------------------------------------------------- 
void saveProject(const std::string& projectFile)
{
  saveProject(getCurrentProject(), projectFile);
  s_Preferences.setLastLoadedProject(projectFile);
}

//--------------------------------------------------------- 
const std::string& getCurrentProjectFile()
{
  return s_Preferences.getLastLoadedProject();
}

//--------------------------------------------------------- 
bool initialize(tGuiFunctions* guiFunctions, const char* homeDir, 
                std::ostream& diagnostics)
{
  assert(guiFunctions);
  initializeConfiguration();
  s_GuiFunctions = guiFunctions;
  s_HomeDir = homeDir;
  loadPreferences();
  const std::string& lastLoadedProject = s_Preferences.getLastLoadedProject();
  if (!lastLoadedProject.empty()) 
  {
    return loadProject(lastLoadedProject, diagnostics);
  }
  else 
  {
    const std::string& lastLoadedFile = s_Preferences.getLastLoadedFile();
    if (!lastLoadedFile.empty()) 
    {
      s_GuiFunctions->displaySourceFile(lastLoadedFile);
    }
    return loadConfiguration(s_GuiFunctions->getConfigFile(s_Preferences.getConfiguration()), diagnostics);
  }
}

//--------------------------------------------------------- 
void destroy()
{
  savePreferences();
  destroyConfiguration();
}

//--------------------------------------------------------- 
void log(const std::string& msg)
{
  if (s_GuiFunctions)
  {
    s_GuiFunctions->log(msg);
  }
}

//--------------------------------------------------------- 
    } // namespace Application 
  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

