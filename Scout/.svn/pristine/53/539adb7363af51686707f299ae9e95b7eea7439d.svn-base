#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QtGlobal>
#include <boost/algorithm/string.hpp>
#include "MainWindow_GUI.h"
#include "SetupDialog_GUI.h"
#include "FindDialog_GUI.h"
#include "GoToLine_GUI.h"
#include "CPPHighlighter_GUI.h"
#include "LogWidget_GUI.h"
#include "App/GuiApplication_APP.h"
#include "App/Preferences_APP.h"
#include "ui_AboutDialog.h"
#include <sstream>
#include "clang/Interface/Application.h"
#include "clang/Interface/Project.h"
#include "SetupExtProcessorDialog_GUI.h"


//Todo move to new class
#include <stdio.h>
#include <QProcess>

//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent, Qt::WFlags flags) :
    QMainWindow(parent, flags)
{
  ui.setupUi(this);
  initTextTab();
  setWindowIcon(QIcon(":/hicfd_main/Pictures/Logo24.png"));

  m_FindDialog = new FindDialog(this);
  m_GoToLine = new GoToLine(this);

  QObject::connect(ui.tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

  QObject::connect(ui.menuEdit, SIGNAL(aboutToShow()), this, SLOT(checkSelection()));
  QObject::connect(ui.menuFile, SIGNAL(aboutToShow()), this, SLOT(checkFileMenu()));

  QObject::connect(ui.actionProject, SIGNAL(triggered()), this, SLOT(openSetupDialog()));
  QObject::connect(ui.actionSetupExtProcessor, SIGNAL(triggered()), this, SLOT(openSetupExtProcessorDialog()));
  QObject::connect(ui.actionSetup, SIGNAL(triggered()), this, SLOT(openConfigurationDialog()));
  QObject::connect(ui.actionAbout_Scout, SIGNAL(triggered()), this, SLOT(openAboutDialog()));
  QObject::connect(ui.actionOpen, SIGNAL(triggered()), this, SLOT(openFile()));
  QObject::connect(ui.actionSave_source_file, SIGNAL(triggered()), this, SLOT(saveSourceFile()));
  QObject::connect(ui.actionSave_source_file_under, SIGNAL(triggered()), this, SLOT(saveSourceFileUnder()));
  QObject::connect(ui.actionSave_target_file_under, SIGNAL(triggered()), this, SLOT(saveTargetFileUnder()));
  QObject::connect(ui.actionOpen_Project, SIGNAL(triggered()), this, SLOT(openProject()));
  QObject::connect(ui.actionSave_Project, SIGNAL(triggered()), this, SLOT(saveProject()));
  QObject::connect(ui.actionSave_Project_under, SIGNAL(triggered()), this, SLOT(saveProjectUnder()));
  QObject::connect(ui.actionSave_Project_With_Current_Files, SIGNAL(triggered()), this, SLOT(saveProjectWithCurrentFiles()));
  QObject::connect(ui.actionSave_Project_with_current_Files_As, SIGNAL(triggered()), this, SLOT(saveProjectWithCurrentFilesAs()));
  QObject::connect(ui.actionNew_Project, SIGNAL(triggered()), this, SLOT(newProject()));
  QObject::connect(ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));
  QObject::connect(ui.actionProcess, SIGNAL(triggered()), this, SLOT(processContent()));
  QObject::connect(ui.actionProcess2, SIGNAL(triggered()), this, SLOT(processContentExt()));
  QObject::connect(ui.output, SIGNAL(cursorPositionChanged ()), this, SLOT(selectLogLine()));
  QObject::connect(ui.output, SIGNAL(cursorOnDblClickChanged()), this, SLOT(addTabFromLog()));

  QObject::connect(ui.actionMark_for_Vectorize, SIGNAL(triggered()), this, SLOT(markForVectorize()));
  QObject::connect(ui.actionMark_Condition_loop_invariant, SIGNAL(triggered()), this, SLOT(markConditionInvariant()));
  QObject::connect(ui.actionMark_Function_for_Expand, SIGNAL(triggered()), this, SLOT(markFunctionExpand()));
  QObject::connect(ui.actionFind_Dialog, SIGNAL(triggered()), this, SLOT(openFindDialog()));
  QObject::connect(ui.actionFind_again, SIGNAL(triggered()), this, SLOT(findAgain()));
  QObject::connect(ui.actionGo_to_Line, SIGNAL(triggered()), this, SLOT(openGoToLine()));
  QObject::connect(ui.actionSelect_All, SIGNAL(triggered()), this, SLOT(selectAll()));
  QObject::connect(ui.actionCopy, SIGNAL(triggered()), this, SLOT(copy()));
  QObject::connect(ui.actionCut, SIGNAL(triggered()), this, SLOT(cut()));
  QObject::connect(ui.actionPaste, SIGNAL(triggered()), this, SLOT(paste()));
  QObject::connect(ui.actionDelete, SIGNAL(triggered()), this, SLOT(deleteSourceCode()));
  QObject::connect(ui.actionUndo, SIGNAL(triggered()), this, SLOT(undo()));
  QObject::connect(ui.actionRedo, SIGNAL(triggered()), this, SLOT(redo()));

  QPlainTextEdit *sourceCode = getCurrentSourceCode();
  m_SourceHighlighter.reset(new CPPHighlighter(sourceCode->document()));
  m_TargetHighlighter.reset(new CPPHighlighter(ui.transformedCode->document()));

  ui.transformedCode->setCenterOnScroll(true);
  ui.transformedCode->setReadOnly (false);

  ui.actionUndo->setEnabled(false);
  ui.actionRedo->setEnabled(false);
  ui.actionSave_source_file->setEnabled(false);
  ui.actionFind_again->setEnabled(false);

  // hidden in the trunk in order to avoid confusion
  ui.actionSetupExtProcessor->setVisible(false);
  ui.actionProcess2->setVisible(false);
}

//-------------------------------------------------------------------------
MainWindow::~MainWindow()
{
}

//-------------------------------------------------------------------------
QPlainTextEdit* MainWindow::createSourceTextEdit()
{
  QPlainTextEdit* result = new QPlainTextEdit(ui.tabWidget);
  result->setFont(ui.transformedCode->font());
  result->setCenterOnScroll(true);
  result->setLineWrapMode(QPlainTextEdit::NoWrap);
  return result;
}

//-------------------------------------------------------------------------
void MainWindow::initTextTab()
{
  ui.tabWidget->removeTab(1);
  ui.tabWidget->removeTab(0);
  ui.tabWidget_2->setTabText(0, "Target file");
  ui.tabWidget_2->removeTab(1);
  ui.tabWidget->addTab(createSourceTextEdit(), "unnamed");
}

//-------------------------------------------------------------------------
void MainWindow::removeAllTabs()
{
  for(int i=1; i < ui.tabWidget->count(); i++)
  {
    ui.tabWidget->removeTab(i);
  }
  ui.tabWidget->setTabToolTip(0, QString());
  ui.tabWidget->setTabText(0, "unnamed");
}

//-------------------------------------------------------------------------
void MainWindow::openFile()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Source file", getCurrentSourceFile(), "C-Source (*.c *.cc *.cpp);;C-Header (*.h *.hpp);;All Files (*.*)");

  if(!fileName.isEmpty())
  {
    displaySourceFile(fileName, true);
  }
}

//-------------------------------------------------------------------------
bool MainWindow::displaySourceFile(const QString& fileName, bool alwaysLoad)
{
  QPlainTextEdit *tab = getTab(fileName);
  bool result = true;

  if(alwaysLoad || tab->toPlainText().isEmpty())
  {
    result = loadSourceFile(fileName, tab);
  }
  ui.tabWidget->setCurrentWidget(tab);
  return result;
}

//-------------------------------------------------------------------------
QPlainTextEdit* MainWindow::getTab(const QString& fileName)
{
  QPlainTextEdit *result = findTab(fileName);
  return result != 0 ? result : addNewTab(fileName);
}

//-------------------------------------------------------------------------
QPlainTextEdit* MainWindow::findTab(const QString& fileName)
{
  for(int i=0; i < ui.tabWidget->count(); ++i)
  {
    if(ui.tabWidget->tabToolTip(i) == fileName)
    {
      return qobject_cast<QPlainTextEdit *>(ui.tabWidget->widget(i));
    }
  }
  return 0;
}

//-------------------------------------------------------------------------
void MainWindow::renameCurrentTab(const QString& fileName)
{
  QFileInfo file(fileName);
  QString base = file.fileName();
  int tabIndex = ui.tabWidget->currentIndex();

  ui.tabWidget->setTabText(tabIndex, base);
  ui.tabWidget->setTabToolTip(tabIndex, fileName);
}
  
//-------------------------------------------------------------------------
QPlainTextEdit* MainWindow::addNewTab(const QString& fileName)
{
  QFileInfo file(fileName);
  QString base = file.fileName();

  int tabIndex = 0;
  if (ui.tabWidget->count() == 1 && ui.tabWidget->tabToolTip(0).isEmpty())
  {
    ui.tabWidget->setTabText(0, base);
  }
  else
  {
    tabIndex = ui.tabWidget->addTab(createSourceTextEdit(), base);
  }

  ui.tabWidget->setCurrentIndex(tabIndex);
  ui.tabWidget->setTabToolTip(tabIndex, fileName);
  QPlainTextEdit *result = getCurrentSourceCode();
  new CPPHighlighter(result->document());
  return result;
}

//-------------------------------------------------------------------------
QString MainWindow::getCurrentSourceFile()
{
  return ui.tabWidget->tabToolTip(ui.tabWidget->currentIndex());
}

//-------------------------------------------------------------------------
QString MainWindow::getCurrentTabLabel()
{
  return ui.tabWidget->tabText(ui.tabWidget->currentIndex());
}

//-------------------------------------------------------------------------
bool MainWindow::loadSourceFile(const QString& fileSource, QPlainTextEdit *sourceCode)
{
  QFile data(fileSource);
  if (data.open(QFile::ReadOnly|QFile::Text))
  {
    QTextStream in(&data);
    QString content = in.readAll();
    sourceCode->setPlainText(content);
    sourceCode->setExtraSelections(QList<QTextEdit::ExtraSelection>());
    data.close();
    setWindowFilePath(fileSource);
    return true;
  }
  return false;
}

//-------------------------------------------------------------------------
void MainWindow::closeTab(int tabIndex)
{
  ui.tabWidget->removeTab(tabIndex);
  if (ui.tabWidget->count() == 0)
  {
    ui.tabWidget->addTab(createSourceTextEdit(), "unnamed");
  }
}

//-------------------------------------------------------------------------
void MainWindow::checkFileMenu()
{
  QString fileName = getCurrentTabLabel();
  ui.actionSave_source_file->setText(QString("Save %1").arg(fileName));
  ui.actionSave_source_file_under->setText(QString("Save %1 as...").arg(fileName));
  ui.actionSave_source_file->setEnabled(getCurrentSourceCode()->document()->isModified());
}

//-------------------------------------------------------------------------
void MainWindow::checkSelection()
{
  QPlainTextEdit *sourceCode = getCurrentText();
  bool bHasSelect = sourceCode->textCursor().hasSelection();
  ui.actionCopy->setEnabled(bHasSelect);
  ui.actionCut->setEnabled(bHasSelect);
  ui.actionDelete->setEnabled(bHasSelect);

  ui.actionUndo->setEnabled(sourceCode->document()->isUndoAvailable());
  ui.actionRedo->setEnabled(sourceCode->document()->isRedoAvailable());
}

//-------------------------------------------------------------------------
void MainWindow::openSetupDialog()
{
  SetupDialog setupDialog(this);
  setupDialog.exec();
}

//-------------------------------------------------------------------------
void MainWindow::openSetupExtProcessorDialog()
{
//  SetupDialog setupDialog(this);
//  setupDialog.exec();

  SetupExtProcessorDialog setupExtProcessorDialog(this);
  setupExtProcessorDialog.exec();
}

//-------------------------------------------------------------------------
void MainWindow::openAboutDialog()
{
  QDialog aboutDialog(this, Qt::Tool);
  Ui::AboutDialog ui;
  ui.setupUi(&aboutDialog);
  std::string scoutVersion("<html>");
  scoutVersion += APP::Application::getScoutFullVersionInfo();
  scoutVersion += "\nQT version " QT_VERSION_STR "\n\n<a href=\"http://scout.zih.tu-dresden.de\">http://scout.zih.tu-dresden.de</a></html>";
  boost::algorithm::replace_all(scoutVersion, "\n", "<br>");
  ui.VersionText->setText(scoutVersion.c_str());
  aboutDialog.exec();
}

//-------------------------------------------------------------------------
QPlainTextEdit* MainWindow::getCurrentSourceCode()
{
  return qobject_cast<QPlainTextEdit*>(ui.tabWidget->currentWidget());
}

//-------------------------------------------------------------------------
QPlainTextEdit* MainWindow::getCurrentText()
{
  return focusWidget() == ui.transformedCode ?
         ui.transformedCode : getCurrentSourceCode();
}

//-------------------------------------------------------------------------
void MainWindow::openFindDialog()
{
  m_FindDialog->showCursorText();
  m_FindDialog->show();
  m_FindDialog->activateWindow();
  ui.actionFind_again->setEnabled(true);
}

//-------------------------------------------------------------------------
void MainWindow::findAgain()
{
  m_FindDialog->findAgain();
}

//-------------------------------------------------------------------------
void MainWindow::openGoToLine()
{
  m_GoToLine->show();
  m_GoToLine->activateWindow();
}

//-------------------------------------------------------------------------
void MainWindow::undo()
{
  getCurrentText()->undo();
}

//-------------------------------------------------------------------------
void MainWindow::redo()
{
  getCurrentText()->redo();
}

//-------------------------------------------------------------------------
void MainWindow::selectAll()
{
  getCurrentText()->selectAll();
}

//-------------------------------------------------------------------------
void MainWindow::copy()
{
  getCurrentText()->copy();
}

//-------------------------------------------------------------------------
void MainWindow::cut()
{
  getCurrentText()->cut();
}

//-------------------------------------------------------------------------
void MainWindow::paste()
{
  getCurrentText()->paste();
}

//-------------------------------------------------------------------------
void MainWindow::deleteSourceCode()
{
  getCurrentText()->textCursor().removeSelectedText();
}

//-------------------------------------------------------------------------
void MainWindow::displayError(const char* errorText, const char* detailedText)
{
  QMessageBox errorBox(this);
  errorBox.setText(errorText);
  if (detailedText != 0)
  {
    errorBox.setDetailedText(detailedText);
  }
  errorBox.setIcon(QMessageBox::Warning);
  errorBox.setSizeGripEnabled(true);
  errorBox.exec();
}

//-------------------------------------------------------------------------
void MainWindow::openConfigurationDialog()
{
  QString fileName = QFileDialog::getOpenFileName(this, "Configuration file",
                                                  APP::Application::getConfigurationPath().c_str(), "C++ Source (*.cc *.cpp)");
  if (!fileName.isEmpty())
  {
    std::stringstream diagnostics;
    std::string configFile(fileName.toUtf8().constData());
    if (!APP::Application::loadConfiguration(configFile, diagnostics))
    {
      displayError("Error loading configuration", diagnostics.str().c_str());
    }
    APP::Application::getPreferences().setConfiguration(configFile);
    clearLog();
    log(diagnostics.str());
  }
}

//-------------------------------------------------------------------------
void MainWindow::saveSourceFile()
{
  QString fileName = getCurrentSourceFile();
  QFile file(fileName);

  if(fileName.isEmpty() || !file.open(QIODevice::WriteOnly|QIODevice::Truncate))
  {
    return;
  }

  file.write(getCurrentSourceCode()->toPlainText().toUtf8());

}

//-------------------------------------------------------------------------
void MainWindow::saveSourceFileUnder()
{
  QString fileName = QFileDialog::getSaveFileName(this, "Source file", getCurrentSourceFile(), "C-Source (*.c *.cc *.cpp);;C-Header (*.h *.hpp);;All Files (*.*)");
  QFile file(fileName);

  if(fileName.isEmpty() || !file.open(QIODevice::WriteOnly|QIODevice::Truncate))
  {
    return;
  }

  file.write(getCurrentSourceCode()->toPlainText().toUtf8());
  renameCurrentTab(fileName);
}

//-------------------------------------------------------------------------
void MainWindow::saveTargetFileUnder()
{
  QString fileName = getCurrentSourceFile();

  if(!m_TargetFilePath.isEmpty())
  {
    fileName = m_TargetFilePath.append(fileName);
  }

  fileName = QFileDialog::getSaveFileName(this, "Target file", fileName, "Target file (*.c, *.cc, *.cpp)");
  QFile file(fileName);

  if(fileName.isEmpty() || !file.open(QIODevice::WriteOnly|QIODevice::Truncate))
  {
    return;
  }

  QFileInfo fileInfo(fileName);
  file.write(ui.transformedCode->toPlainText().toUtf8());
  m_TargetFilePath = fileInfo.canonicalPath();;
}

//-------------------------------------------------------------------------
void MainWindow::openProject()
{
  QString projectName = QFileDialog::getOpenFileName(this, "Project",
                                                     QString::fromUtf8(APP::Application::getCurrentProjectFile().c_str()),
                                                     "Scout project (*.spr)");
  if (!projectName.isEmpty())
  {
    std::stringstream diagnostics;
    if (!APP::Application::loadProject(projectName.toUtf8().constData(), diagnostics))
    {
      displayError("Error loading project", diagnostics.str().c_str());
    }
    else
    {
      clearLog();
      log(diagnostics.str());
    }
  }
}

//-------------------------------------------------------------------------
void MainWindow::saveProject()
{
  QString projectName = APP::Application::getCurrentProjectFile().c_str();

  if(!projectName.isEmpty())
  {
    APP::Application::saveProject(projectName.toUtf8().constData());
  }
}

//-------------------------------------------------------------------------
void MainWindow::saveProjectUnder()
{
  QString projectName = QFileDialog::getSaveFileName(this, "Project",
                                                     QString::fromUtf8(APP::Application::getCurrentProjectFile().c_str()),
                                                     "Scout project (*.spr)");
  if (!projectName.isEmpty())
  {
    APP::Application::saveProject(projectName.toUtf8().constData());
  }
}

//-------------------------------------------------------------------------
void MainWindow::saveProjectWithCurrentFiles()
{
  QString currentSourceFile = getCurrentSourceFile();

  if(!currentSourceFile.isEmpty())
  {
    APP::Application::getCurrentProject().setLastLoadedFile(currentSourceFile.toUtf8().constData());
  }

  QString projectName = APP::Application::getCurrentProjectFile().c_str();

  if (!projectName.isEmpty())
  {
    APP::Application::saveProject(projectName.toUtf8().constData());
  }
}

//-------------------------------------------------------------------------
void MainWindow::saveProjectWithCurrentFilesAs()
{
  QString currentSourceFile = getCurrentSourceFile();

  if(!currentSourceFile.isEmpty())
  {
    APP::Application::getCurrentProject().setLastLoadedFile(currentSourceFile.toUtf8().constData());
  }

  QString projectName = QFileDialog::getSaveFileName(this, "Project",
                                                     QString::fromUtf8(APP::Application::getCurrentProjectFile().c_str()),
                                                     "Scout project (*.spr)");
  if (!projectName.isEmpty())
  {
    APP::Application::saveProject(projectName.toUtf8().constData());
  }
}

//-------------------------------------------------------------------------
void MainWindow::markForVectorize()
{
  insertTextAtLine(-1, "#pragma scout loop vectorize\n");
}

//-------------------------------------------------------------------------
void MainWindow::markConditionInvariant()
{
  insertTextAtLine(-1, "#pragma scout condition invariant\n");
}

//-------------------------------------------------------------------------
void MainWindow::markFunctionExpand()
{
  insertTextAtLine(-1, "#pragma scout function expand\n");
}


//-------------------------------------------------------------------------
void MainWindow::newProject()
{
  APP::Application::newProject();
}

//-------------------------------------------------------------------------
void MainWindow::processContent()
{
  QString fileName = getCurrentSourceFile();
  if(fileName.isEmpty())
  {
    return;
  }

  QPlainTextEdit *currentSourceCode = getCurrentSourceCode();

  std::vector<const char*> Args;
  std::list<std::string> commandLine;
  QByteArray sourceCode(currentSourceCode->toPlainText().toUtf8());
  APP::Application::addCmdArgsFromProject(Args, commandLine);

  std::string processedFile;
  std::stringstream diagnostics;
  APP::Application::processFile(sourceCode.constData(),
                                sourceCode.constData() + sourceCode.size(),
                                QFileInfo(fileName).canonicalFilePath().toUtf8().constData(),
                                Args, processedFile, diagnostics);
  clearLog();
//diagnostics<<processedFile;
  log(diagnostics.str());
  setTransformedSource(processedFile.c_str());

  ui.tabWidget_2->setTabText(0, QFileInfo(fileName).fileName());
}

//-------------------------------------------------------------------------
void MainWindow::processContentExt()
{
    const APP::Preferences& preferences = APP::Application::getPreferences();

    QString fileName = getCurrentSourceFile();
    QString logStr;
    if(fileName.isEmpty())
    {
      return;
    }

//Todo move to new class

    //Prepare external program to run
    QString extProg = preferences.getExtProcessorFile().c_str();
    QStringList extProgArguments;
    if (preferences.getExtProcessorOption().size() > 0)
        extProgArguments << preferences.getExtProcessorOption().c_str();

    extProgArguments.replaceInStrings("%f", QString::fromStdString(fileName.toStdString()));

    QProcess extProcess;
    extProcess.start(extProg, extProgArguments);

    if (!extProcess.waitForStarted())
        logStr.append("!!! Error starting external process !!!");

    //We are not writing data to the external process so we can close this channel.
    //This resolves some trouble with processes waiting for input.
    extProcess.closeWriteChannel();

    if (!extProcess.waitForFinished())
        logStr.append("!!! Error running external process !!!");

    logStr.append( extProcess.readAllStandardError());

//Todo move to new class (end)

    setTransformedSource( QString(extProcess.readAllStandardOutput()).toLatin1() );

    clearLog();
    log(logStr.toStdString());

}

//-------------------------------------------------------------------------
void MainWindow::log(const std::string& msg)
{
  ui.output->appendPlainText(QString::fromUtf8(msg.c_str()));
  ui.output->appendPlainText("\n");
}

//-------------------------------------------------------------------------
void MainWindow::clearLog()
{
  ui.output->setPlainText(QString());
}

//-------------------------------------------------------------------------
void MainWindow::setTransformedSource(const char* pMsg)
{
  ui.transformedCode->setPlainText(QString::fromUtf8(pMsg));
}

//-------------------------------------------------------------------------
void MainWindow::insertTextAtLine(int lineNr, const char* pText)
{
  QPlainTextEdit *sourceCode = getCurrentSourceCode();
  if (lineNr >= 0)
  {
    QTextCursor sourceCursor = sourceCode->textCursor();
    sourceCursor.setPosition(sourceCode->document()->findBlockByLineNumber(lineNr).position());
    sourceCode->setTextCursor(sourceCursor);
  }
  else
  {
    sourceCode->moveCursor(QTextCursor::StartOfLine);
  }
  sourceCode->insertPlainText(QString::fromUtf8(pText));
}

//-------------------------------------------------------------------------
void MainWindow::showLineRange(QPlainTextEdit* pTextEdit, int startLineNr, int endLineNr)
{
  QList<QTextEdit::ExtraSelection> extraSelections;
  QTextEdit::ExtraSelection selection;
  selection.format.setBackground(QColor(Qt::yellow).lighter(160));
  selection.format.setProperty(QTextFormat::FullWidthSelection, true);
  selection.cursor = pTextEdit->textCursor();
  int startLinePos = pTextEdit->document()->findBlockByLineNumber(startLineNr-1).position();
  selection.cursor.setPosition(startLinePos);
  selection.cursor.setPosition(pTextEdit->document()->findBlockByLineNumber(endLineNr-1).position(), QTextCursor::KeepAnchor);
  extraSelections.append(selection);
  pTextEdit->setExtraSelections(extraSelections);
  pTextEdit->setTextCursor(selection.cursor);
  pTextEdit->ensureCursorVisible();

  // deselcet the range and move to the start of the block:
  selection.cursor.setPosition(startLinePos);
  pTextEdit->setTextCursor(selection.cursor);
  //pTextEdit->ensureCursorVisible();
}

//-------------------------------------------------------------------------
void MainWindow::selectLogLine()
{
  QTextCursor selectedText(ui.output->textCursor());
  selectedText.select(QTextCursor::LineUnderCursor);
  if (selectedText.anchor() < selectedText.position())
  {
    int iBeginOfLine = selectedText.anchor();
    selectedText.setPosition(selectedText.position());  // move the anchor
    selectedText.setPosition(iBeginOfLine, QTextCursor::KeepAnchor);
  }
  ui.output->setTextCursor(selectedText);
}

//-------------------------------------------------------------------------
void MainWindow::addTabFromLog()
{
  QString lineInfo(ui.output->textCursor().selectedText());
  QRegExp expression(":[0-9]+:[0-9]+:");
  int index = expression.indexIn(lineInfo);
  if (index >= 0)
  {
    displaySourceFile(lineInfo.left(index), false);
    QString lineNr = lineInfo.mid(index + 1, lineInfo.indexOf(':', index + 1) - index - 1);
    int iLineNr = lineNr.toInt();
    showLineRange(getCurrentSourceCode(), iLineNr, iLineNr);
  }

  QRegExp targetLineRange("\\{tgt:[0-9]+:[0-9]+\\}");
  index = targetLineRange.indexIn(lineInfo);
  if (index >= 0)
  {
    int afterFirstColon = index + 5;
    int secondColon = lineInfo.indexOf(':', afterFirstColon);
    QString startLineNr = lineInfo.mid(afterFirstColon, secondColon - afterFirstColon);
    QString endLineNr = lineInfo.mid(secondColon + 1, lineInfo.indexOf('}', secondColon) - secondColon - 1);
    showLineRange(ui.transformedCode, startLineNr.toInt(), endLineNr.toInt());
  }
}

//-------------------------------------------------------------------------
  } // namespace GUI
} // namespace HICFD
