
void h(float* a, float* d) 
{
  int i;
  float x, y = 1.0;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    x = d[i];
    x += y;
    a[i] = x;
  }
}
