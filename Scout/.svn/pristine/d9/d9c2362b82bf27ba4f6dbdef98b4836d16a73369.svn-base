
typedef struct { float a, b; } A;

void f(float* a, int b) 
{
#pragma scout loop vectorize
  for (int i = 0; i < b; ++i)
  {
    A x = {};
    x.a = a[i];
  }
}
