
float sqr(float x)
{
  return x * x;
}

// regression: inlining occurs in an uncompounded for-body
// fix: ensure compound-body, since inlining works only on compounds
void f(float* a, int b) 
{
#pragma scout loop vectorize
  for (int i = 0; i < b; ++i)
    a[i] = sqr(a[i]); 
}
