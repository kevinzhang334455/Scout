
void h(float* a, float* b, float c, float* d) 
{
  int i, j, k;
  float x;
#pragma scout loop vectorize size(8)
  for (i = 0; i < 100; ++i)
  {
    x = .0;
    for (j = 0; j < k; ++j)
    {
      x += b[j];
    }
    d[i] += x;  // check that d[i+4] is loaded before adding
  }
}
