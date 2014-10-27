#ifndef FINDFILES_GUI_H
#define FINDFILES_GUI_H

//-------------------------------------------------------------------------
#include <QtGui>
#include <QDialog>
#include "ui_FindDialog.h"

//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------
class MainWindow;

//-------------------------------------------------------------------------
class FindDialog : public QDialog
{
  Q_OBJECT


public:
  FindDialog(MainWindow *parent);
  void showCursorText();
  void findAgain();

private slots:
  void findDialog();

private:
  MainWindow* m_Parent;
  QString m_Text;
  QFlags<QTextDocument::FindFlag> m_Flag;

  Ui::FindDialogForm ui;
};

//-------------------------------------------------------------------------

  } // namespace GUI
} // namespace HICFD

//-------------------------------------------------------------------------

#endif // FINDFILES_GUI_H
