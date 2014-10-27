#ifndef MAINWINDOW_GUI_H
#define MAINWINDOW_GUI_H

//-------------------------------------------------------------------------

#include <QtGui/QMainWindow>
#include "ui_MainWindow.h"
#include <memory>

//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {

//-------------------------------------------------------------------------
class CPPHighlighter;
class FindDialog;
class GoToLine;

//-------------------------------------------------------------------------

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = 0, Qt::WFlags flags = 0);
  ~MainWindow();

  void log(const std::string& msg);
  void displayError(const char* errorText, const char* detailedText);

  void clearLog();
  bool displaySourceFile(const QString& fileName, bool alwaysLoad);
  QPlainTextEdit* getCurrentText();
  void removeAllTabs();

public Q_SLOTS:
  void openSetupDialog();
  void openSetupExtProcessorDialog();
  void openConfigurationDialog();
  void openAboutDialog();
  void openFile();
  void saveSourceFile();
  void saveSourceFileUnder();
  void saveTargetFileUnder();
  void openFindDialog();
  void findAgain();
  void openGoToLine();
  void copy();
  void cut();
  void paste();
  void deleteSourceCode();
  void selectAll();
  void undo();
  void redo();
  void selectLogLine();
  void addTabFromLog();
  void processContent();
  void processContentExt();
  void openProject();
  void saveProject();
  void saveProjectUnder();
  void saveProjectWithCurrentFiles();
  void saveProjectWithCurrentFilesAs();
  void newProject();
  void markForVectorize();
  void markFunctionExpand();
  void markConditionInvariant();
  void checkSelection();
  void closeTab(int tabIndex);
  void checkFileMenu();

private:
  static void showLineRange(QPlainTextEdit* pTextEdit, int startLineNr, int endLineNr);
  QPlainTextEdit* getCurrentSourceCode();
  QString getCurrentSourceFile();
  QString getCurrentTabLabel();
  void setTransformedSource(const char* pMsg);
  void insertTextAtLine(int lineNr, const char* pText);
  void initTextTab();
  bool loadSourceFile(const QString& fileSource, QPlainTextEdit *sourceCode);
  QPlainTextEdit* addNewTab(const QString& fileName);
  QPlainTextEdit* getTab(const QString& fileName);
  QPlainTextEdit* findTab(const QString& fileName);
  QPlainTextEdit* createSourceTextEdit();
  void renameCurrentTab(const QString& fileName);

  FindDialog *m_FindDialog;
  GoToLine *m_GoToLine;

  std::auto_ptr<CPPHighlighter>  m_SourceHighlighter, m_TargetHighlighter;
  QString m_TargetFilePath;

  Ui::MainWindow ui;

};

//-------------------------------------------------------------------------

  } // namespace GUI
} // namespace HICFD

//-------------------------------------------------------------------------

#endif // MAINWINDOW_GUI_H
