#include "ProjectLoader_APP.h"
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>
#include <QXmlInputSource>

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace APP {

namespace {

//------------------------------------------------------------------------- 
struct tProjectHandler : QXmlDefaultHandler
{
  QString  m_CurrentText;
  std::list<std::string>  m_CurrentList;
  Project& m_Project;

  tProjectHandler(Project& p) : m_Project(p) {}

  bool startElement(const QString& /* namespaceURI */,
                    const QString& /* localName */,
                    const QString& qName,
                    const QXmlAttributes& /*attributes*/)
  {
    m_CurrentText.clear();
    if (qName == "include_paths" || qName == "preprocessed_files")
    {
      m_CurrentList.clear();
    }
    return true;
  }

  bool endElement(const QString& /* namespaceURI */,
                  const QString& /* localName */,
                  const QString& qName)
  {
    if (qName == "last_loaded_file")
    {
      m_Project.setLastLoadedFile(m_CurrentText.toUtf8().constData());
    }
    else if (qName == "configuration_name")
    {
      //m_Project.setConfiguration(m_CurrentText.toUtf8().constData());
    }
    else if (qName == "commandline")
    {
      m_Project.setCommandLineParameters(m_CurrentText.toUtf8().constData());
    }
    else if (qName == "prologtextfile")
    {
      m_Project.setPrologTextFile(m_CurrentText.toUtf8().constData());
    }
    else if (qName == "item")
    {
      m_CurrentList.push_back(m_CurrentText.toUtf8().constData());
    }
    else if (qName == "include_paths")
    {
      m_Project.swapSetIncludePaths(m_CurrentList);
    }
    else if (qName == "preprocessed_files")
    {
      m_Project.swapSetPreprocessedFiles(m_CurrentList);
    }
    return true;
  }

  bool characters(const QString &str)
  {
    m_CurrentText += str;
    return true;
  }
};

//------------------------------------------------------------------------- 
void writeList(QXmlStreamWriter& stream, const char* listName, 
               const std::list<std::string>& l)
{
  stream.writeStartElement(listName);
  stream.writeTextElement("count", QString::number(l.size()));
  for (std::list<std::string>::const_iterator i = l.begin(),
       e = l.end(); i != e; ++i)
  {
    stream.writeTextElement("item", i->c_str());
  }
  stream.writeEndElement(); // listName
}

} // anon namespace

//------------------------------------------------------------------------- 
bool loadProject(Project& project, const std::string& filename)
{
  //m_ConfigurationName = "sse2";

  QFile file(filename.c_str());
  if (!file.open(QIODevice::ReadOnly))
  {
    return false;
  }
  QXmlInputSource source(&file);
  QXmlSimpleReader reader;
  tProjectHandler handler(project);
  reader.setContentHandler(&handler);
  reader.setErrorHandler(&handler);
  return reader.parse(&source);
}

//------------------------------------------------------------------------- 
void saveProject(const Project& project, const std::string& filename)
{
  QFile file(filename.c_str());
  if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate))
  {
    return;
  }
  QXmlStreamWriter stream(&file);
  stream.setAutoFormatting(true);
  stream.writeStartDocument();
  stream.writeStartElement("project");
    stream.writeTextElement("version", "1");
    stream.writeTextElement("last_loaded_file", project.getLastLoadedFile().c_str());
    //stream.writeTextElement("configuration_name", project.getConfigurationName().c_str());
    stream.writeTextElement("commandline", project.getCommandLineParameters().c_str());
    stream.writeTextElement("prologtextfile", project.getPrologTextFile().c_str());
    writeList(stream, "include_paths", project.getIncludePaths());
    writeList(stream, "preprocessed_files", project.getPreprocessedFiles());
  stream.writeEndElement(); // project
  stream.writeEndDocument();
  file.close();
}


//------------------------------------------------------------------------ 
  } // namespace GUI
} // namespace HICFD
 
//------------------------------------------------------------------------- 
