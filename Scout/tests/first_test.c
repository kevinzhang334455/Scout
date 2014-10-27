
float sqr(float x)
{
  return x * x;
}

void f(float* a, int b) 
{
#pragma scout loop vectorize
  for (int i = 0; i < b; ++i)
  {
    a[i] = sqr(a[i]); 
  }
}
