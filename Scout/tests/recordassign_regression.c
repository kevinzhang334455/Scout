
typedef struct { float a, b; } Vec;

Vec foo(float a) 
{
  Vec x;
  x.a = a*a;
  x.b = a*a;
  return x;
}


void h(float* a, float* d) 
{
  int i;
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    Vec x = foo(a[i]);
    d[i] = x.a + x.b;
  }
}

