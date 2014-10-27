#ifndef GOTOLINE_GUI_H
#define GOTOLINE_GUI_H

//-------------------------------------------------------------------------
#include <QtGui>
#include <QDialog>
#include "ui_GoToLine.h"
#include "MainWindow_GUI.h"

//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------

class GoToLine : public QDialog
{
  Q_OBJECT


public:
  GoToLine(MainWindow *parent=0);

private slots:
  void goToLine();

private:
  MainWindow* m_Parent;

  Ui::GoToLineForm ui;
};

//-------------------------------------------------------------------------

  } // namespace GUI
} // namespace HICFD

//-------------------------------------------------------------------------


#endif // GOTOLINE_GUI_H
