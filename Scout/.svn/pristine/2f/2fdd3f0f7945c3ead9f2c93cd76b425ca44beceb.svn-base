
void g(double* a, double* b, int *c) 
{
  int i, j;
  double x;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    j = c[i];
    x = a[j] + b[j+1];
    a[c[i]] = x;
    a[c[i]+1] = x;
    a[c[i+1]] = x;
  }
}
