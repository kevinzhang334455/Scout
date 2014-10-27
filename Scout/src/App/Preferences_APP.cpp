#include "Preferences_APP.h"
#include <QSettings>

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace APP {


//------------------------------------------------------------------------- 
void Preferences::load()
{
  QSettings settings;
  m_LastLoadedFile = settings.value("last_loaded_file").toString().toUtf8().constData();
  m_LastLoadedProject = settings.value("last_loaded_project").toString().toUtf8().constData();
  m_ConfigurationName = settings.value("configuration_name", "sse2").toString().toUtf8().constData();
  m_ExtProcessorFile = settings.value("ext_program_location", "/bin/cat").toString().toUtf8().constData();
  m_ExtProcessorOption = settings.value("ext_program_options", "%f").toString().toUtf8().constData();
}


//------------------------------------------------------------------------- 
void Preferences::save()
{

  //QSettings settings("TU-Dresden", "Scout-GUI");
  QSettings settings;
  settings.setValue("last_loaded_file", QString::fromUtf8(m_LastLoadedFile.c_str()));
  settings.setValue("last_loaded_project", QString::fromUtf8(m_LastLoadedProject.c_str()));
  settings.setValue("configuration_name", QString::fromUtf8(m_ConfigurationName.c_str()));
  settings.setValue("ext_program_location", QString::fromUtf8(m_ExtProcessorFile.c_str()));
  settings.setValue("ext_program_options", QString::fromUtf8(m_ExtProcessorOption.c_str()));
}


//------------------------------------------------------------------------ 
  } // namespace GUI
} // namespace HICFD
 
//------------------------------------------------------------------------- 
