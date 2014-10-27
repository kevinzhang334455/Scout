
#define MIN(a,b) (a) < (b) ? (a) : (b)

void h(float* a, float* d) 
{
  int i;
  float x = .9, y = 1.0, z;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    x += a[i];
    y = MIN(y, d[i] + a[i]);
  }
}
