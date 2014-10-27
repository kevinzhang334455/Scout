
void h(float* a, float* d) 
{
  int i;
  double x, y;
#pragma scout loop vectorize size(8)
  for (i = 0; i < 100; ++i)
  {
    x = a[i];
    y = d[i];
    x /= y;
    a[i] = x;
  }
}
