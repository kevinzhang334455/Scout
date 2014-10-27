
void h(float* a, float* d) 
{
  int i;
  float x, y = 1.0, z;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    z = d[i] + a[i];
    x = a[i] < 0 ? z + y : z - y;
    a[i] = x;
  }
}
