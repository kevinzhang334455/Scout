
void h(float* a, float* b, float c, float* d) 
{
  int i, j, k;
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
#pragma scout loop vectorize 
    for (j = 0; j < k; ++j)
    {
      d[i * k + j] = b[i] + a[j];
    }
  }
}
