
void f(float* a, float b, int* c) 
{
  int i;
  float d[100];
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    if (c[i])
    {
      d[i] = a[i] + b;
    }
    else
    {
      d[i] = a[i] - b;
    }
  }
}
