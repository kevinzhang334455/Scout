
//-----------------------------------------------
// z is used after the condition -> vectorize it
//-----------------------------------------------
void h1(double* a, double* b, double* d) 
{
  int i;
  double x, y, z;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    y = a[i];
    if (y < 0) 
    {
      z = b[i-1];
      x = d[i] + y * z; 
    }
    else 
    {
      z = b[i+1];
      x = d[i] - y * z;
    }
    a[i] = x * d[i]+z;
  }
}

//-----------------------------------------------
// z is not used after the condition -> leave it
//-----------------------------------------------
void h2(double* a, double* b, double* d) 
{
  int i;
  double x, y, z;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    y = a[i];
    if (y < 0) 
    {
      z = b[i-1];
      x = d[i] + y * z; 
    }
    else 
    {
      z = b[i+1];
      x = d[i] - y * z;
    }
    a[i] = x * d[i];
  }
}
