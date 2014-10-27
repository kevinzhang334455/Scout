/* the intermediate result in a[i] should not be stored but hold
   in a temporary */

void g(float* a, float* c, float* d) 
{
  int i;
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    a[i] += c[i];
    a[i] += d[i];
  }
}
