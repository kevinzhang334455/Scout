#include "LogWidget_GUI.h"

//-------------------------------------------------------------------------
LogWidget::LogWidget(QWidget *parent, Qt::WFlags flags) :
    QPlainTextEdit(parent)
{
}

//-------------------------------------------------------------------------
void LogWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
  emit cursorOnDblClickChanged();
}

