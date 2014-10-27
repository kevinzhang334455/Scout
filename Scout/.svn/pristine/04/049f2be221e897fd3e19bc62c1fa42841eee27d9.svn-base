
/* this tests if c[i] is loaded only once */

void g(float* a, float b, float* c, float* d) 
{
  int i;
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    a[i] = c[i] + c[i] * d[i] + a[i] * c[i];
  }
}
