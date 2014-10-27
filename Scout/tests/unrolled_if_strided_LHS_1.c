
//-----------------------------------------------
// b[i] is read before and after the condition 
// -> still easy: the conditional write introduces a new ssa 
//                region, which becomes bound th a var later on
//-----------------------------------------------
void h1(double* a, double* b, double* d) 
{
  int i;
  double y;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    y = b[i];
    if (y < 0) 
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
// b[i] is read after the condition 
// -> easy case: nothing special to do, 
//               b is loaded afterwards implicitely
//-----------------------------------------------
void h2(double* a, double* b, double* d) 
{
  int i;
  double y;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    y = a[i];
    if (y < 0) 
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
// b[i] is not used after the condition 
// -> easiest case: nothing special to do
//-----------------------------------------------
void h3(double* a, double* b, double* d) 
{
  int i;
  double x, y;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    y = a[i];
    if (y < 0) 
    {
      b[i] = d[i] + y; 
    }
    else 
    {
      b[i] = d[i] - y;
    }
    a[i] = x * d[i];
  }
}
