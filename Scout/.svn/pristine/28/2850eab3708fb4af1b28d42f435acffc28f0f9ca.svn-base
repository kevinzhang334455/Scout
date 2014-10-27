
float sqr(float x)
{
  return x * x;
}

void f(float* a, int b) 
{
  int j;
#pragma openmp argument(1 + symbol)
  for (j = 0; j < b; ++j)
  {
#pragma scout loop vectorize
    for (int i = 0; i < b; ++i)
    {
      a[i] = sqr(a[i]); 
    }
  }
}
