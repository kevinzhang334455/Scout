
void h1(double* a, double* b, double* d) 
{
  int i, cond;
  double y, x, z;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    y = a[i];
    cond = 1;    // must be replaced by the unrolled variable used in the if-leaf
    x = z;       // assignment must remain
    if (y < 0) 
    {
      cond = 0;
      y = b[i];
      x = d[i];
    }

    if (cond == 0)
    {
      y *= y;
    }
    d[i] = y * x;
  }
}

