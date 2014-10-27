#ifndef LOGWIDGET_GUI_H
#define LOGWIDGET_GUI_H

#include <QPlainTextEdit>

class LogWidget : public QPlainTextEdit
{
  Q_OBJECT

public:
  LogWidget(QWidget *parent = 0, Qt::WFlags flags = 0);

signals:
  void cursorOnDblClickChanged();

protected:
  virtual void mouseDoubleClickEvent (QMouseEvent *event);
};

#endif // LOGWIDGET_GUI_H
