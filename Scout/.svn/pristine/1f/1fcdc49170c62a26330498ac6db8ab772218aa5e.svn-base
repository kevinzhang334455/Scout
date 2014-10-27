//===--- RewriteObjC.cpp - Playground for the code rewriter ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Hacks and fun related to the code rewriter.
//
//===----------------------------------------------------------------------===//

#include "clang/Vectorizing/ExpressionMatch.h"    
#include "clang/ASTProcessing/StmtEditor.h"    
#include "clang/ASTProcessing/Toolbox.h"    
#include "clang/AST/DeclCXX.h"    
#include "clang/AST/StmtVisitor.h"    
#include "llvm/ADT/SmallVector.h"

//--------------------------------------------------------- 
namespace clang {

namespace ASTProcessing {

//--------------------------------------------------------- 
namespace {

//--------------------------------------------------------- 
struct ExpressionStringBuilder : StmtVisitor<ExpressionStringBuilder, bool>
{
  std::string m_TextExpr;
  CXXMethodDecl* m_MD;
  
  ExpressionStringBuilder(CXXMethodDecl* MD) : m_MD(MD) {}

  bool VisitBinaryOperator(BinaryOperator* Node)
  {
    m_TextExpr += '2';
    m_TextExpr += ' ' + (char)Node->getOpcode();
    return Visit(Node->getLHS()) && Visit(Node->getRHS());
  }

  bool VisitUnaryOperator(UnaryOperator* Node)
  {
    m_TextExpr += '1';
    m_TextExpr += ' ' + (char)Node->getOpcode();
    return Visit(Node->getSubExpr());
  }

  bool VisitConditionalOperator(ConditionalOperator* Node)
  {
    m_TextExpr += "3c";
    return Visit(Node->getCond()) &&
           Visit(Node->getTrueExpr()) &&
           Visit(Node->getFalseExpr());
  }

  bool VisitParenExpr(ParenExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  bool VisitImplicitCastExpr(ImplicitCastExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  bool VisitDeclRefExpr(DeclRefExpr* Node)
  {
    FunctionDecl::param_iterator i = 
      std::find(m_MD->param_begin(), m_MD->param_end(), Node->getDecl());
    if (i == m_MD->param_end())
    {
      return false;
    }
    // terminal symbol, ensures that more complex expressions are lower:
    m_TextExpr += 'h' + char(i - m_MD->param_begin());  
    return true;
  }

  bool VisitStmt(Stmt*)
  {
    return false;
  }
};

//--------------------------------------------------------- 
struct ExpressionStringSearcherBase : ConstStmtVisitor<ExpressionStringSearcherBase, bool>
{
  StmtEditor&            m_Editor;
  const std::string&     m_TextExpr;
  std::string::size_type m_CurrentIndex;


  ExpressionStringSearcherBase(const std::string& textExpr, StmtEditor& editor) :
    m_Editor(editor),
    m_TextExpr(textExpr),
    m_CurrentIndex(0)
  {}

  bool isOpcode(char number, char opCode)
  {
    return m_TextExpr.size() > m_CurrentIndex + 2 && 
           m_TextExpr[m_CurrentIndex] == number && 
           m_TextExpr[m_CurrentIndex+1] == (opCode + ' ');
  }

  bool isOpcode(const BinaryOperator* Node)
  {
    return isOpcode('2', (char)Node->getOpcode());
  }

  bool isOpcode(const UnaryOperator* Node)
  {
    return isOpcode('1', (char)Node->getOpcode());
  }

  bool isOpcode(const ConditionalOperator* Node)
  {
    return isOpcode('3', 'c' - ' ');
  }

  template<class T>
  bool check_operator(const T* Node)
  {
    if (isOpcode(Node))
    {
      m_CurrentIndex += 2;
      return check_subnodes(Node);
    }
    return VisitExpr(Node);
  }

  virtual Expr* getSubExpression(unsigned argumentPos) = 0;
  virtual void insertSubExpr(unsigned argumentPos, const Expr* Node) = 0;

  bool check_terminal(const Expr* Node)
  {
    if (m_TextExpr[m_CurrentIndex] < 'h')
    {
      return false;
    }

    unsigned argumentPos = m_TextExpr[m_CurrentIndex] - 'h';
    Expr* subExpr = getSubExpression(argumentPos);
    if (subExpr != 0)
    {
      std::string expr1, expr2;
      getAstAsString(m_Editor, subExpr, expr1);
      getAstAsString(m_Editor, Node, expr2);
      if (expr1 != expr2)
      {
        return false;
      }
    }
    insertSubExpr(argumentPos, Node);
    ++m_CurrentIndex;
    return true;
  }

  bool check_subnodes(const Expr* Node)
  {
    for (Stmt::const_child_iterator i = Node->child_begin(), e = Node->child_end();
         i != e; ++i)
    {
      if (!Visit(cast<Expr>(*i)))
      {
        return false;
      }
    }
    return true;
  }


  bool VisitBinaryOperator(const BinaryOperator* Node)
  {
    return check_operator(Node);
  }

  bool VisitUnaryOperator(const UnaryOperator* Node)
  {
    return check_operator(Node);
  }

  bool VisitConditionalOperator(const ConditionalOperator* Node)
  {
    return check_operator(Node);
  }

  bool VisitParenExpr(const ParenExpr* Node)
  {
    return Visit(Node->getSubExpr());
  }

  bool VisitExpr(const Expr* Node)
  {
    return check_terminal(Node);
  }
};

//--------------------------------------------------------- 
struct ExpressionStringSearcher : ExpressionStringSearcherBase
{
  llvm::SmallVector<Expr*, 4>& m_SubExprs;

  ExpressionStringSearcher(const std::string& textExpr, StmtEditor& editor,
                           llvm::SmallVector<Expr*, 4>& subExprs) :
    ExpressionStringSearcherBase(textExpr, editor),
    m_SubExprs(subExprs)
  {}

  virtual Expr* getSubExpression(unsigned argumentPos)
  {
    return argumentPos < m_SubExprs.size() ? m_SubExprs[argumentPos] : 0;
  }    

  virtual void insertSubExpr(unsigned argumentPos, const Expr* Node)
  {
    if (argumentPos >= m_SubExprs.size())
    {
      m_SubExprs.resize(argumentPos+1, 0);
    }
    m_SubExprs[argumentPos] = const_cast<Expr*>(Node);
  }    
};

//--------------------------------------------------------- 
struct AllSubExpressionStringSearcher : ExpressionStringSearcherBase
{
  tAllSubExpressions& m_SubExprs;

  AllSubExpressionStringSearcher(const std::string& textExpr, StmtEditor& editor,
                                 tAllSubExpressions& subExprs) :
    ExpressionStringSearcherBase(textExpr, editor),
    m_SubExprs(subExprs)
  {}

  virtual Expr* getSubExpression(unsigned argumentPos)
  {
    return argumentPos < m_SubExprs.size() && (!m_SubExprs[argumentPos].empty()) ? m_SubExprs[argumentPos].front() : 0;
  }    

  virtual void insertSubExpr(unsigned argumentPos, const Expr* Node)
  {
    if (argumentPos >= m_SubExprs.size())
    {
      m_SubExprs.resize(argumentPos+1);
    }
    m_SubExprs[argumentPos].push_back(const_cast<Expr*>(Node));
  }    
};

//--------------------------------------------------------- 
std::string buildBinExpr(const std::string& lhs, const std::string& rhs, 
                         BinaryOperatorKind opCode)
{
  if (rhs.empty())
  {
    assert(0);
    return rhs;
  }
  std::string result;
  result += '2';
  result += ' ' + (char)opCode;
  result += lhs;
  result += rhs;
  return result;
}

//--------------------------------------------------------- 
char getTerminal(const char*& i)
{
  if (*i >= 'a' && *i <= 'z')
  {
    return 'h' - 'a' + *i++;
  }
  return 0;
}

//--------------------------------------------------------- 
std::string getMultiplicativeExpr(const char*& i)
{
  char lhs = getTerminal(i);
  if (lhs == 0 || (*i != '*' && *i != '/'))
  {
    return lhs == 0 ? std::string() : std::string(1u, lhs);
  }
  BinaryOperatorKind opCode = *i++ == '*' ? BO_Mul : BO_Div;
  char rhs = getTerminal(i);
  return rhs == 0 ? std::string() : buildBinExpr(std::string(1u, lhs), 
                                                 std::string(1u, rhs), opCode);
}

//--------------------------------------------------------- 
std::string getAdditiveExpr(const char*& i)
{
  std::string lhs = getMultiplicativeExpr(i);
  if (lhs.empty() || (*i != '+' && *i != '-'))
  {
    return lhs;
  }
  BinaryOperatorKind opCode = *i++ == '+' ? BO_Add : BO_Sub;
  return buildBinExpr(lhs, getMultiplicativeExpr(i), opCode);
}

//--------------------------------------------------------- 
std::string getRelationalExpr(const char*& i)
{
  std::string lhs = getAdditiveExpr(i);
  if (lhs.empty() || (*i != '>' && *i != '<'))
  {
    return lhs;
  }
  bool isLT = *i++ == '<';
  BinaryOperatorKind opCode;
  if (*i == '=')
  {
    ++i;
    opCode = isLT ? BO_LE : BO_GE;
  }
  else
  {
    opCode = isLT ? BO_LT : BO_GT;
  }
  return buildBinExpr(lhs, getAdditiveExpr(i), opCode);
}

//--------------------------------------------------------- 
std::string getConditionalExpr(const char*& i)
{
  std::string cond = getRelationalExpr(i);
  if (cond.empty() || *i != '?')
  {
    return cond;
  }

  std::string true_expr = getRelationalExpr(++i);
  if (true_expr.empty() || *i != ':')
  {
    return std::string();
  }

  std::string false_expr = getRelationalExpr(++i);
  if (false_expr.empty())
  {
    return std::string();
  }

  std::string result = "3c";
  result += cond;
  result += true_expr;
  result += false_expr;
  return result;
}

} // anon namespace

//--------------------------------------------------------- 
std::string buildExpressionString(CXXMethodDecl* MD, Expr* E)
{
  ExpressionStringBuilder builder(MD);
  return builder.Visit(E) ? builder.m_TextExpr : std::string();
}

//--------------------------------------------------------- 
bool matchExpressionString(const std::string& exprString, const Expr* E,
  llvm::SmallVector<Expr*, 4>& subExprs, StmtEditor& editor)
{
  ExpressionStringSearcher builder(exprString, editor, subExprs);
  return builder.Visit(E) && builder.m_CurrentIndex == exprString.size();
}

//--------------------------------------------------------- 
bool matchExpressionString(const std::string& exprString, const Expr* E,
                           tAllSubExpressions& subExprs, StmtEditor& editor)
{
  AllSubExpressionStringSearcher builder(exprString, editor, subExprs);
  return builder.Visit(E) && builder.m_CurrentIndex == exprString.size();
}

//--------------------------------------------------------- 
std::string buildInternalExpressionString(const char* i)
{
  char lhs = getTerminal(i);
  if (lhs == 0 || *i++ != '=')
  {
    assert(0 && "internal string representation broken(1)");
    return std::string();
  }
  std::string rhs = getConditionalExpr(i);
  if (rhs.empty() || *i != 0)
  {
    assert(0 && "internal string representation broken(2)");
    return std::string();
  }
  return buildBinExpr(std::string(1u, lhs), rhs, BO_Assign);
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang
