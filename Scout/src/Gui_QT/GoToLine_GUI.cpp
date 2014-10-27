#include "GoToLine_GUI.h"
#include "MainWindow_GUI.h"
#include <QtGui>


//-------------------------------------------------------------------------

namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------
GoToLine::GoToLine(MainWindow *parent)
  : QDialog(parent, Qt::Tool),
  m_Parent(parent)
{
  ui.setupUi(this);

  QObject::connect(ui.showLineButton, SIGNAL(clicked()), this, SLOT(goToLine()));
}

//-------------------------------------------------------------------------
void GoToLine::goToLine()
{
  QPlainTextEdit *currentText = m_Parent->getCurrentText();
  int iLineNr = ui.lineNumberEdit->text().toInt();

  QTextCursor cursor = currentText->textCursor();
  cursor.setPosition(0);
  cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, iLineNr-1);
  currentText->setTextCursor(cursor);

  hide();
}

//-------------------------------------------------------------------------

  } // namespace APP
} // namespace HICFD

