#ifndef SETUPEXTPROCESSORDIALOG_GUI_H
#define SETUPEXTPROCESSORDIALOG_GUI_H

//------------------------------------------------------------------------- 

#include <QtGui/QDialog>
#include "ui_SetupExtProcessorDialog.h"

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace GUI {

//------------------------------------------------------------------------- 

class SetupExtProcessorDialog : public QDialog
{
  Q_OBJECT
  typedef QDialog tBase;

public:
  SetupExtProcessorDialog(QWidget *parent);

public Q_SLOTS: 
    void locateExtProcessorFile();
//  void locatePrologTextFile();
//  void addIncludePath();
//  void removeIncludePath();
//  void addPreprocessedFile();
//  void removePreprocessedFile();
  void apply();
  virtual void accept();

private:
  Ui::SetupExtProcessorDialog ui;
};


//------------------------------------------------------------------------- 

  } // namespace APP
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // SETUPEXTPROCESSORDIALOG_GUI_H
