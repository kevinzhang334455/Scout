
//-----------------------------------------------
// b[i] is written before and in the condition 
// fixed: no vector store neccessary
//-----------------------------------------------
void h1(double* a, double* b, double* d) 
{
  int i;
  double y;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    b[i] = y * a[i];
    a[i] = b[i] * y;
    if (a[i] < 0) 
    {
      b[i] = d[i] + y; 
    }
    else 
    {
      b[i] = d[i] - y;
    }
    a[i] = b[i] * d[i];
  }
}

//-----------------------------------------------
// b[i] is heavily used 
// all ok, because the store comes late
//-----------------------------------------------
void h2(double* a, double* b, double* d) 
{
  int i;
  double y;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    b[i] = y * a[i];
    a[i] = b[i] * y;
    if (a[i] < 0) 
    {
      b[i] += d[i]; 
    }
    else 
    {
      b[i] = d[i] - y;
    }
    b[i] *= d[i];
  }
}
