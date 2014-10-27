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

#ifndef SCOUT_CLANGADDONS_ASTPROCESSING_STMTTRAVERSAL_H
#define SCOUT_CLANGADDONS_ASTPROCESSING_STMTTRAVERSAL_H

#include "clang/AST/StmtGraphTraits.h"
#include "clang/AST/StmtVisitor.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/DenseSet.h"
#include <algorithm>

//--------------------------------------------------------- 
namespace clang {

//--------------------------------------------------------- 
namespace ASTProcessing
{

//--------------------------------------------------------- 
namespace Private {

template<class T, class U>
struct visit_all_stmts_tagged;

//--------------------------------------------------------- 
template<class T>
struct visit_all_stmts_tagged<T, bool>
{
  static void traverse_df(Stmt* S, const StmtVisitor<T, bool>& visitor)
  {
    for (llvm::df_iterator<Stmt*> i = llvm::df_begin(S), e = llvm::df_end(S); i != e;)
    {
      const_cast<StmtVisitor<T, bool>&>(visitor).Visit(*i) ? i.skipChildren() : ++i;
    }
  }

  static void traverse_df(Stmt* S, StmtVisitor<T, bool>& visitor)
  {
    for (llvm::df_iterator<Stmt*> i = llvm::df_begin(S), e = llvm::df_end(S); i != e;)
    {
      visitor.Visit(*i) ? i.skipChildren() : ++i;
    }
  }
};

//--------------------------------------------------------- 
template<class T>
struct visit_all_stmts_tagged<T, void>
{
  static void traverse_df(Stmt* S, const StmtVisitor<T, void>& visitor)
  {
    for (llvm::df_iterator<Stmt*> i = llvm::df_begin(S), e = llvm::df_end(S); i != e; ++i)
    {
      const_cast<StmtVisitor<T, void>&>(visitor).Visit(*i);
    }
  }

  static void traverse_df(Stmt* S, StmtVisitor<T, void>& visitor)
  {
    for (llvm::df_iterator<Stmt*> i = llvm::df_begin(S), e = llvm::df_end(S); i != e; ++i)
    {
      visitor.Visit(*i);
    }
  }
};

} // namespace Private

//--------------------------------------------------------- 
template<class T>
inline void for_each_df(Stmt* S, T fn)
{
  std::for_each(llvm::df_begin(S), llvm::df_end(S), fn);
}

//--------------------------------------------------------- 
template<class T, typename RetTy>
inline void visit_df(Stmt* S, const StmtVisitor<T, RetTy>& visitor)
{
  Private::visit_all_stmts_tagged<T, RetTy>::traverse_df(S, visitor);
}

//--------------------------------------------------------- 
template<class T, typename RetTy>
inline void visit_df(Stmt* S, StmtVisitor<T, RetTy>& visitor)
{
  Private::visit_all_stmts_tagged<T, RetTy>::traverse_df(S, visitor);
}

//--------------------------------------------------------- 
template<class T>
inline void visit_po(Stmt* S, const T& visitor)
{
  for (llvm::po_iterator<Stmt*> i = llvm::po_begin(S), e = llvm::po_end(S); i != e; ++i)
  {
    const_cast<T&>(visitor).Visit(*i);
  }
}

//--------------------------------------------------------- 
template<class T>
inline void visit_po(Stmt* S, T& visitor)
{
  for (llvm::po_iterator<Stmt*> i = llvm::po_begin(S), e = llvm::po_end(S); i != e; ++i)
  {
    visitor.Visit(*i);
  }
}

//--------------------------------------------------------- 
// helper for simple overload resolving in conjunction with bind
template<class T>
inline void insertValue(llvm::DenseSet<T>* S, const T& value)
{
  S->insert(value);
}

//--------------------------------------------------------- 
// stmt_iterator_initializer
struct stmt_iterator_initializer
{
  Stmt* m_S;
  explicit stmt_iterator_initializer(Stmt* S) : m_S(S) {}
};


//--------------------------------------------------------- 
// stmt_iterator traverses over all Stmts of a given type in a tree
template< class StmtTy, class IteratorTy = llvm::df_iterator<Stmt*> >
class stmt_iterator
{
  IteratorTy  m_Iterator;

  void toNext()
  {
    while (m_Iterator != IteratorTy::end(0) && 
           !isa<StmtTy>(*m_Iterator))
    {
      ++m_Iterator;
    } 
  }

public:
  typedef StmtTy* pointer;
  typedef stmt_iterator<StmtTy, IteratorTy> _Self;

  explicit stmt_iterator(Stmt* start) : 
    m_Iterator(IteratorTy::begin(start)) {
    toNext();
  }

  stmt_iterator() : m_Iterator(IteratorTy::end(0)) {}

  inline bool operator==(const _Self& x) const {
    return m_Iterator == x.m_Iterator;
  }

  inline bool operator!=(const _Self& x) const { return !operator==(x); }

  inline pointer operator*() const {
    return cast<StmtTy>(*m_Iterator);
  }

  inline pointer operator->() const { return operator*(); }

  inline _Self& operator++() {   // Preincrement
    ++m_Iterator;
    toNext();
    return *this;
  }

  inline _Self& skipChildren() { 
    m_Iterator.skipChildren();
    toNext();
    return *this;
  }

  inline stmt_iterator(const stmt_iterator_initializer& i) :
    m_Iterator(i.m_S ? IteratorTy::begin(i.m_S) : IteratorTy::end(0))
  {
    if (i.m_S) toNext();
  }
};

//--------------------------------------------------------- 
inline stmt_iterator_initializer stmt_ibegin(Stmt* Node)
{
  return stmt_iterator_initializer(Node);
}

//--------------------------------------------------------- 
inline stmt_iterator_initializer stmt_iend(Stmt*)
{
  return stmt_iterator_initializer(0);
}

//--------------------------------------------------------- 
} // namespace ASTProcessing

} // namespace clang

#endif  //SCOUT_CLANGADDONS_ASTPROCESSING_STMTTRAVERSAL_H
