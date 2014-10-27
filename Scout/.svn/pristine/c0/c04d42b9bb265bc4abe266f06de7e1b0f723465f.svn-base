#ifndef SETUPDIALOG_GUI_H
#define SETUPDIALOG_GUI_H

//------------------------------------------------------------------------- 

#include <QtGui/QDialog>
#include "ui_SetupDialog.h"

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace GUI {

//------------------------------------------------------------------------- 

class SetupDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog tBase;

public:
  SetupDialog(QWidget *parent);

public Q_SLOTS: 
  void locatePreloadFile();
  void locatePrologTextFile();
  void addIncludePath();
  void removeIncludePath();
  void addPreprocessedFile();
  void removePreprocessedFile();
  void apply();
  virtual void accept();

private:
  Ui::SetupDialog   ui;
};


//------------------------------------------------------------------------- 

  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // SETUPDIALOG_GUI_H
