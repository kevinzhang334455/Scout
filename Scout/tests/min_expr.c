
#define MIN(a,b) (a) < (b) ? (a) : (b)

void h(float* a, float* d) 
{
  int i;
  float x, y = 1.0, z;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    z = MIN(d[i] + a[i], 1.0);
    x = a[i] < 0 ? z + y : z - y;
    a[i] = x;
  }
}
