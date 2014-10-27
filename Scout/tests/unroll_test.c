
int sqr(int x)
{
  return x * x;
}

#pragma scout function expand
void f(int* a, int b) 
{
  int i;
  *a = 0;

#pragma scout loop unroll
  for (i = 0; i < 5; ++i)
  {
    a[i] += sqr(i); 
  }

  // don't unroll:
  for (i = 0; i < 50; ++i)
  {
    *a += sqr(i); 
  }
}
