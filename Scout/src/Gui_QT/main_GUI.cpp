#include "MainWindow_GUI.h"
#include <QtGui/QApplication>
#include <QProcess>
#include <QDir>
#include <QMessageBox>
#include "App/GuiApplication_APP.h"
#include <sstream>

//---------------------------------------------------------
namespace HICFD {

//---------------------------------------------------------
struct tQtGuiFunctionsGlue : APP::Application::tGuiFunctions
{
  GUI::MainWindow mainWindow;

  virtual void log(const std::string& text)
  {
    mainWindow.log(text);
  }

  virtual bool displaySourceFile(const std::string& fileName)
  {
    mainWindow.removeAllTabs();
    return mainWindow.displaySourceFile(QString::fromUtf8(fileName.c_str()), true);
  }

  virtual std::string getConfigFile(const std::string& configName)
  {
    {
      QFileInfo configFile(configName.c_str());
      if (configFile.exists())
      {
        return configFile.canonicalFilePath().toUtf8().constData();
      }
    }

    {
      QFileInfo configFile(QDir::currentPath(), (configName + ".cpp").c_str());
      if (configFile.exists())
      {
        return configFile.canonicalFilePath().toUtf8().constData();
      }
    }

    {
      QDir appDir(QDir::home());
      if (appDir.cd("Scout") && appDir.cd("config"))
      {
        QFileInfo configFile(appDir, (configName + ".cpp").c_str());
        if (configFile.exists())
        {
          return configFile.canonicalFilePath().toUtf8().constData();
        }
      }
    }
    return std::string();
  }

};

} // namespace HICFD
//---------------------------------------------------------

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setApplicationName("Scout");
  a.setOrganizationName("ZIH_TU_Dresden");
  HICFD::tQtGuiFunctionsGlue w;
  w.mainWindow.show();
  std::stringstream diagnostics;
  if (!HICFD::APP::Application::initialize(&w, (QDir::homePath() + '/').toUtf8().constData(), diagnostics))
  {
    w.mainWindow.displayError("Error during initialization", diagnostics.str().c_str());
  }
  else
  {
    w.mainWindow.log(diagnostics.str());
  }
  int result = a.exec();
  HICFD::APP::Application::destroy();
  return result;
}
