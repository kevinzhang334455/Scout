#include "SetupDialog_GUI.h"
#include <QFileDialog>
#include "clang/Interface/Application.h"
#include "clang/Interface/Project.h"

//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------
SetupDialog::SetupDialog(QWidget *parent) :
  QDialog (parent)
{
  ui.setupUi(this);

  ui.listWidget->setDragDropMode(QAbstractItemView::InternalMove);
  ui.listWidget->setDragDropOverwriteMode(false);
  ui.listWidget->setMovement(QListView::Snap);

  QObject::connect(ui.open_clang_location, SIGNAL(clicked()), this, SLOT(locatePreloadFile()));
  QObject::connect(ui.open_prolog_location, SIGNAL(clicked()), this, SLOT(locatePrologTextFile()));
  QObject::connect(ui.add_include_path, SIGNAL(clicked()), this, SLOT(addIncludePath()));
  QObject::connect(ui.remove_include_path, SIGNAL(clicked()), this, SLOT(removeIncludePath()));
  QObject::connect(ui.buttonBox->button(QDialogButtonBox::Apply), SIGNAL(clicked()), this, SLOT(apply()));

  QObject::connect(ui.addPreprocessedFile, SIGNAL(clicked()), this, SLOT(addPreprocessedFile()));
  QObject::connect(ui.removePreprocessedFile, SIGNAL(clicked()), this, SLOT(removePreprocessedFile()));

  const APP::Project& project = APP::Application::getCurrentProject();
  ui.input_clang_location->setText(QString::fromUtf8(project.getLastLoadedFile().c_str()));
  ui.input_defines->setText(QString::fromUtf8(project.getCommandLineParameters().c_str()));
  ui.input_prolog_location->setText(QString::fromUtf8(project.getPrologTextFile().c_str()));

  const std::list<std::string>& includePaths = project.getIncludePaths();
  for (std::list<std::string>::const_iterator i = includePaths.begin(),
       e = includePaths.end(); i != e; ++i)
  {
    ui.listWidget->addItem(QString::fromUtf8(i->c_str()));
  }

  const std::list<std::string>& preprocessedFiles = project.getPreprocessedFiles();
  for (std::list<std::string>::const_iterator i = preprocessedFiles.begin(),
       e = preprocessedFiles.end(); i != e; ++i)
  {
    ui.preprocessedFiles->addItem(QString::fromUtf8(i->c_str()));
  }

  std::string cmdLine;
  APP::Application::buildCommandLineFromProject(cmdLine);
  ui.command_line->setText(QString::fromUtf8(cmdLine.c_str()));

}

//-------------------------------------------------------------------------
void SetupDialog::locatePreloadFile() // ->file to load
{
  QString fileSource = QFileDialog::getOpenFileName(this, "Source file", ui.input_clang_location->text(), "C-Source (*.c *.cc *.cpp);;C-Header (*.h *.hpp);;All Files (*.*)");
  if (!fileSource.isEmpty())
  {
    ui.input_clang_location->setText(fileSource);
  }
}

//-------------------------------------------------------------------------
void SetupDialog::locatePrologTextFile()
{
  QString fileSource = QFileDialog::getOpenFileName(this, "Prolog file", ui.input_prolog_location->text(), "Text (*.txt *.inc *.c);;All Files (*.*)");
  if (!fileSource.isEmpty())
  {
    ui.input_prolog_location->setText(fileSource);
  }
}

//-------------------------------------------------------------------------
void SetupDialog::addIncludePath()
{
  QString fileSource = QFileDialog::getExistingDirectory(this, "Include path");
  if (!fileSource.isEmpty())
  {
    ui.listWidget->addItem(fileSource);
  }
}

//-------------------------------------------------------------------------
void SetupDialog::removeIncludePath()
{
  QList<QListWidgetItem*> selectedItems (ui.listWidget->selectedItems());
  foreach (QListWidgetItem* item, selectedItems)
  {
    delete item;
  }
}

//-------------------------------------------------------------------------
void SetupDialog::addPreprocessedFile()
{
  QString sourceLocation = ui.preprocessedFiles->count() == 0 ? ui.input_clang_location->text() : ui.preprocessedFiles->item(ui.preprocessedFiles->count()-1)->text();

  QString fileSource = QFileDialog::getOpenFileName(this, "Source file", sourceLocation, "C-Source (*.c *.cc *.cpp);;C-Header (*.h *.hpp);;All Files (*.*)");
  if (!fileSource.isEmpty())
  {
    ui.preprocessedFiles->addItem(fileSource);
  }
}

//-------------------------------------------------------------------------
void SetupDialog::removePreprocessedFile()
{
  QList<QListWidgetItem*> selectedItems (ui.preprocessedFiles->selectedItems());
  foreach (QListWidgetItem* item, selectedItems)
  {
    delete item;
  }
}

//-------------------------------------------------------------------------
void SetupDialog::apply()
{
  APP::Project& project = APP::Application::getCurrentProject();
  project.setLastLoadedFile(ui.input_clang_location->text().toUtf8().constData());
  std::list<std::string> includePaths;
  for (int i = 0; i < ui.listWidget->count(); ++i)
  {
    includePaths.push_back(ui.listWidget->item(i)->text().toUtf8().constData());
  }
  project.swapSetIncludePaths(includePaths);

  includePaths.clear();
  for (int i = 0; i < ui.preprocessedFiles->count(); ++i)
  {
    includePaths.push_back(ui.preprocessedFiles->item(i)->text().toUtf8().constData());
  }
  project.swapSetPreprocessedFiles(includePaths);
  project.setCommandLineParameters(ui.input_defines->text().toUtf8().constData());
  project.setPrologTextFile(ui.input_prolog_location->text().toUtf8().constData());
}

//-------------------------------------------------------------------------
void SetupDialog::accept()
{
  apply();
  tBase::accept();
}

//-------------------------------------------------------------------------
  } // namespace GUI
} // namespace HICFD
