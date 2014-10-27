#ifndef PROJECT_APP_H
#define PROJECT_APP_H

//------------------------------------------------------------------------- 

#include <string>
#include <list>

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace APP {

//------------------------------------------------------------------------- 

class Project
{
public:
  const std::string& getLastLoadedFile() const;
  //const std::string& getConfiguration() const;
  const std::list<std::string>& getIncludePaths() const;
  const std::list<std::string>& getPreprocessedFiles() const;
  const std::string& getCommandLineParameters() const;
  const std::string& getPrologTextFile() const;

  void setLastLoadedFile(const std::string& fileName);
  //void setConfiguration(const std::string& configName);
  void swapSetIncludePaths(std::list<std::string>& includePaths);
  void swapSetPreprocessedFiles(std::list<std::string>& preprocessedFiles);
  void addPreprocessedFile(const std::string& preprocessedFiles);
  void setCommandLineParameters(const std::string& cmdP);
  void setPrologTextFile(const std::string& prologTextFile);

private:
  std::string             m_LastLoadedFile;
  //std::string             m_ConfigurationName;
  std::list<std::string>  m_IncludePaths;
  std::list<std::string>  m_PreprocessedFiles;
  std::string             m_CommandLineParameters;
  std::string             m_PrologTextFile;
};


//-------------------------------------------------------------------------
inline const std::string& Project::getLastLoadedFile() const
{
  return m_LastLoadedFile; 
}

//------------------------------------------------------------------------- 
inline const std::string& Project::getCommandLineParameters() const
{
  return m_CommandLineParameters; 
}

//------------------------------------------------------------------------- 
/*inline const std::string& Project::getConfiguration() const
{
  return m_ConfigurationName; 
}*/

//------------------------------------------------------------------------- 
inline const std::list<std::string>& Project::getIncludePaths() const
{
  return m_IncludePaths;
}

//------------------------------------------------------------------------- 
inline const std::list<std::string>& Project::getPreprocessedFiles() const
{
  return m_PreprocessedFiles;
}

//------------------------------------------------------------------------- 
inline const std::string& Project::getPrologTextFile() const
{
  return m_PrologTextFile;
}

//------------------------------------------------------------------------- 
inline void Project::setLastLoadedFile(const std::string& fileName) 
{
  m_LastLoadedFile = fileName;
}

//------------------------------------------------------------------------- 
/*inline void Project::setConfiguration(const std::string& configName)
{
  m_ConfigurationName = configName;
}*/

//------------------------------------------------------------------------- 
inline void Project::setCommandLineParameters(const std::string& cmdP)
{
  m_CommandLineParameters = cmdP;
}

//------------------------------------------------------------------------- 
inline void Project::setPrologTextFile(const std::string& prologTextFile)
{
  m_PrologTextFile = prologTextFile;
}

//------------------------------------------------------------------------- 
inline void Project::swapSetIncludePaths(std::list<std::string>& includePaths)
{
  m_IncludePaths.swap(includePaths);
}

//------------------------------------------------------------------------- 
inline void Project::swapSetPreprocessedFiles(std::list<std::string>& preprocessedFiles)
{
  m_PreprocessedFiles.swap(preprocessedFiles);
}

//------------------------------------------------------------------------- 
inline void Project::addPreprocessedFile(const std::string& preprocessedFiles)
{
  m_PreprocessedFiles.push_back(preprocessedFiles);
}
//------------------------------------------------------------------------- 

  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // PROJECT_APP_H
