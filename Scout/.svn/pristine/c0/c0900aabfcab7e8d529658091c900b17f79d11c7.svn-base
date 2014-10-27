
void h(double* a) 
{
  int i, j[100], k[100];
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    int m = j[i], n = k[i];
    double f = ((int)a[m] == n) ? 1.0 : 0.0;
    a[m] = n;
  }
}
