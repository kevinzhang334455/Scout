#include "FindDialog_GUI.h"
#include "MainWindow_GUI.h"

//-------------------------------------------------------------------------
namespace HICFD {
  namespace GUI {


//-------------------------------------------------------------------------
FindDialog::FindDialog(MainWindow *parent) :
    QDialog(parent, Qt::Tool),
    m_Parent(parent)
{
  ui.setupUi(this);
  ui.textComboBox->completer()->setCaseSensitivity(Qt::CaseSensitive);

  QObject::connect(ui.findButton, SIGNAL(clicked()), this, SLOT(findDialog()));
  QObject::connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(hide()));
}

//-------------------------------------------------------------------------
void FindDialog::findDialog()
{
  m_Text = ui.textComboBox->currentText();

  if (ui.textComboBox->findText(m_Text) == -1)
  {
    ui.textComboBox->addItem(m_Text);
  }

  m_Flag = QFlags<QTextDocument::FindFlag>();

  if(ui.caseSensitiveCheckBox->isChecked())
  {
    m_Flag |= QTextDocument::FindCaseSensitively;
  }

  if(ui.findWholeWordsCheckBox->isChecked())
  {
    m_Flag |= QTextDocument::FindWholeWords;
  }

  findAgain();
}

//-------------------------------------------------------------------------
void FindDialog::findAgain()
{
  QPlainTextEdit *currentText = m_Parent->getCurrentText();
  if(currentText->find(m_Text, m_Flag) == 0)
  {
    currentText->moveCursor(QTextCursor::Start);
  }
}

//-------------------------------------------------------------------------
void FindDialog::showCursorText()
{
  QPlainTextEdit *currentText = m_Parent->getCurrentText();
  QTextCursor cursor = currentText->textCursor();
  QString cursorText;

  cursor.select(QTextCursor::WordUnderCursor);
  cursorText = cursor.selectedText();
  ui.textComboBox->setEditText(cursorText);
}



//-------------------------------------------------------------------------

  } // namespace APP
} // namespace HICFD
