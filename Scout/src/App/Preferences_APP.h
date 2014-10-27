#ifndef PREFERENCES_APP_H
#define PREFERENCES_APP_H

//------------------------------------------------------------------------- 

#include <string>
#include <list>

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace APP {

//------------------------------------------------------------------------- 

class Preferences
{
public:
  void load();
  void save();

  const std::string& getLastLoadedFile() const;
  const std::string& getLastLoadedProject() const;
  const std::string& getConfiguration() const;
  const std::string& getExtProcessorFile() const;
  const std::string& getExtProcessorOption() const;

  void setLastLoadedFile(const std::string& fileName);
  void setLastLoadedProject(const std::string& fileName);
  void setConfiguration(const std::string& configName);
  void setExtProcessorFile(const std::string& fileName);
  void setExtProcessorOption(const std::string& configName);
private:
  std::string             m_LastLoadedFile;
  std::string             m_LastLoadedProject;
  std::string             m_ConfigurationName;
  std::string             m_ExtProcessorFile;
  std::string             m_ExtProcFile;
  std::string             m_ExtProcessorOption;
};


//-------------------------------------------------------------------------
inline const std::string& Preferences::getLastLoadedFile() const
{
  return m_LastLoadedFile; 
}

//------------------------------------------------------------------------- 
inline const std::string& Preferences::getConfiguration() const
{
  return m_ConfigurationName; 
}

//-------------------------------------------------------------------------
inline const std::string& Preferences::getExtProcessorFile() const
{
  return m_ExtProcessorFile;
}

//-------------------------------------------------------------------------
inline const std::string& Preferences::getExtProcessorOption() const
{
  return m_ExtProcessorOption;
}

//------------------------------------------------------------------------- 
inline void Preferences::setConfiguration(const std::string& configName)
{
  m_ConfigurationName = configName;
}

//------------------------------------------------------------------------- 
inline void Preferences::setLastLoadedFile(const std::string& fileName) 
{
  m_LastLoadedFile = fileName;
}

//-------------------------------------------------------------------------
inline const std::string& Preferences::getLastLoadedProject() const
{
  return m_LastLoadedProject; 
}

//------------------------------------------------------------------------- 
inline void Preferences::setLastLoadedProject(const std::string& fileName) 
{
  m_LastLoadedProject = fileName;
}

//-------------------------------------------------------------------------
inline void Preferences::setExtProcessorFile(const std::string& fileName)
{
  m_ExtProcessorFile = fileName;
}

//-------------------------------------------------------------------------
inline void Preferences::setExtProcessorOption(const std::string& configName)
{
  m_ExtProcessorOption = configName;
}

//------------------------------------------------------------------------- 

  } // namespace GUI
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // PREFERENCES_APP_H
