
void g(float* a, float* b, int *c) 
{
  int i, j;
  float x;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    j = c[i];
    x = a[j] + b[j+1];
    a[c[i]] = x;
    a[c[i]+1] = x;
    a[c[i+1]] = x;
  }
}
