
void h(float* a, float* b, float* d) 
{
  int i, j, k;
  float x, y;
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    x = a[i];  // needs a temporary
    x = 1.0;
    a[i] = b[i];
    d[i] = x;
  }
}
