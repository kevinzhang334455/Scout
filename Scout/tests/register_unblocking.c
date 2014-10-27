
void h(float* a, float* b, float c, float* d) 
{
  int i, j, k;
  float x;
#pragma scout loop vectorize size(16)
  for (i = 0; i < 100; ++i)
  {
    x = d[i];
    for (j = 0; j < k; ++j)
    {
      x += b[j];
    }
    d[i] = x;
  }
}
