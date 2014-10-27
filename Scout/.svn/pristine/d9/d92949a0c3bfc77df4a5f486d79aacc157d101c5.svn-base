/*
Tests whether b is initialized before b_vectorized is initialized.
that is whether VectorizeInfo::m_ScalarLoopInvariantExprs is handled properly.
*/

void g(float* a, float b) 
{
  int i;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    b = 2.0;
    a[i] = b;
  }
}
