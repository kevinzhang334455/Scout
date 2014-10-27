#ifndef CPPHIGHLIGHTER_GUI_H
#define CPPHIGHLIGHTER_GUI_H

//------------------------------------------------------------------------- 

#include <QSyntaxHighlighter>
#include <QHash>
#include <QTextCharFormat>

//------------------------------------------------------------------------- 
class QTextDocument;

//------------------------------------------------------------------------- 

namespace HICFD {

  namespace GUI {

//------------------------------------------------------------------------- 

class CPPHighlighter : public QSyntaxHighlighter
{
  Q_OBJECT

public:
  CPPHighlighter(QTextDocument *parent = 0);

protected:
  void highlightBlock(const QString &text);

private:
  struct HighlightingRule
  {
     QRegExp pattern;
     QTextCharFormat format;
  };
  QVector<HighlightingRule> highlightingRules;

  QRegExp commentStartExpression;
  QRegExp commentEndExpression;

  QTextCharFormat keywordFormat;
  QTextCharFormat commentFormat;
  QTextCharFormat quotationFormat;
};

//------------------------------------------------------------------------- 

  } // namespace GUI
} // namespace HICFD
 
//------------------------------------------------------------------------- 

#endif // CPPHIGHLIGHTER_GUI_H
