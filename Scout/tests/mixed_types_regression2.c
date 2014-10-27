
void h(double* a[2], float* d) 
{
  int i;
  float x, y;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    x = a[i][1];
    y = d[i];
    x /= y;
    a[i][0] = x;
  }
}
