
void h(float* a, float* d) 
{
  int i;
  double x, y;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    x = a[i];
    y = d[i];
    x /= y;
    a[i] = x;
  }
}
