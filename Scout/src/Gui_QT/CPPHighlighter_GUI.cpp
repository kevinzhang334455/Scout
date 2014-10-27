#include "CPPHighlighter_GUI.h"

//------------------------------------------------------------------------- 

namespace HICFD {
  namespace GUI {

//------------------------------------------------------------------------- 
CPPHighlighter::CPPHighlighter(QTextDocument *parent) : 
  QSyntaxHighlighter(parent)
{
  static const char* keyWords[] = {
    "auto", "const", "double", "float", "int", "short", "struct", "unsigned",
    "break", "continue", "else", "for", "long", "signed", "switch", "void",
    "case", "default", "enum", "goto", "register", "sizeof", "typedef", 
    "volatile", "char", "do", "extern", "if", "return", "static", "union",
    "while", "asm", "dynamic_cast", "namespace", "reinterpret_cast", "try", 
    "bool", "explicit", "new", "static_cast", "typeid", "catch", "false", 
    "operator", "template", "typename", "class", "friend", "private", "this", 
    "using", "const_cast", "inline", "public", "throw", "virtual", "delete",
    "mutable", "protected", "true", "wchar_t", "and", "bitand", "compl", 
    "not_eq", "or_eq", "xor_eq", "and_eq", "bitor", "not", "or", "xor", 
    0
  };


  HighlightingRule rule;
  keywordFormat.setForeground(Qt::blue);
  for (const char** pKeyword = keyWords; *pKeyword != 0; ++pKeyword)
  {
    rule.pattern = QRegExp(QString::fromAscii("\\b%1\\b").arg(QString::fromAscii(*pKeyword)));
    rule.format = keywordFormat;
    highlightingRules.append(rule);
  }

  rule.pattern = QRegExp("\\s*#[^\\n]*");
  rule.format = keywordFormat;
  highlightingRules.append(rule);

  commentFormat.setForeground(Qt::darkGreen);
  rule.pattern = QRegExp("//[^\n]*");
  rule.format = commentFormat;
  highlightingRules.append(rule);

  quotationFormat.setForeground(Qt::red);
  rule.pattern = QRegExp("\".*\"");
  rule.format = quotationFormat;
  highlightingRules.append(rule);

  commentStartExpression = QRegExp("/\\*");
  commentEndExpression = QRegExp("\\*/");
}

//------------------------------------------------------------------------- 
void CPPHighlighter::highlightBlock(const QString &text)
{
  foreach (const HighlightingRule &rule, highlightingRules) 
  {
    QRegExp expression(rule.pattern);
    int index = expression.indexIn(text);
    while (index >= 0) 
    {
      int length = expression.matchedLength();
      setFormat(index, length, rule.format);
      index = expression.indexIn(text, index + length);
    }
  }
  setCurrentBlockState(0);

  int startIndex = 0;
  if (previousBlockState() != 1)
  {
    startIndex = commentStartExpression.indexIn(text);
  }

  while (startIndex >= 0) 
  {
    int endIndex = commentEndExpression.indexIn(text, startIndex);
    int commentLength;
    if (endIndex == -1) 
    {
      setCurrentBlockState(1);
      commentLength = text.length() - startIndex;
    } 
    else 
    {
      commentLength = endIndex - startIndex
                       + commentEndExpression.matchedLength();
    }
    setFormat(startIndex, commentLength, commentFormat);
    startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
  }
}

//------------------------------------------------------------------------- 
  } // namespace GUI
} // namespace HICFD
