#include "SetupExtProcessorDialog_GUI.h"
#include <QFileDialog>
#include "clang/Interface/Application.h"
#include "clang/Interface/Project.h"

#include "App/GuiApplication_APP.h"
#include "App/Preferences_APP.h"

//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------
SetupExtProcessorDialog::SetupExtProcessorDialog(QWidget *parent) :
  QDialog (parent)
{
  ui.setupUi(this);

  QObject::connect(ui.open_ext_processor, SIGNAL(clicked()), this, SLOT(locateExtProcessorFile()));

  const APP::Preferences& preferences = APP::Application::getPreferences();
  ui.input_ext_processor->setText(preferences.getExtProcessorFile().c_str());
  ui.input_exp_program_options->setText(preferences.getExtProcessorOption().c_str());

}

//-------------------------------------------------------------------------
void SetupExtProcessorDialog::locateExtProcessorFile() // ->file to load
{
  QString fileSource = QFileDialog::getOpenFileName(this, "External Processor", ui.input_ext_processor->text(), "All Files (*.*)");
  if (!fileSource.isEmpty())
  {
    ui.input_ext_processor->setText(fileSource);
  }
}

//-------------------------------------------------------------------------
void SetupExtProcessorDialog::apply()
{
  std::string ext_processor( ui.input_ext_processor->text().toStdString() );
  APP::Application::getPreferences().setExtProcessorFile( ext_processor );

  std::string ext_program_options( ui.input_exp_program_options->text().toStdString() );
  APP::Application::getPreferences().setExtProcessorOption( ext_program_options );

}

//-------------------------------------------------------------------------
void SetupExtProcessorDialog::accept()
{
  apply();
  tBase::accept();
}

//-------------------------------------------------------------------------
  } // namespace GUI
} // namespace HICFD
