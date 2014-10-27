
void g(float* a, float* b, int *c) 
{
  int i, j, k;
  float x;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    j = c[i];
    k = j * 100;
    x = a[k];
  }
}
