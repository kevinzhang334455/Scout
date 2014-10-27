
typedef struct A { float x; } A;

void f(float* a, int b) 
{
  A x;
#pragma scout loop vectorize
  for (int i = 0; i < b; ++i)
  {
    x.x += a[i];
  }
}
