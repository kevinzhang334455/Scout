
typedef struct { float a, b; } A;

void f(float* a, int b) 
{
#pragma scout loop vectorize scalar
  for (int i = 0; i < b; ++i)
  {
    A x = { };
    A y = { .b = .4 };
    x.a = a[i];
    x.b = y.b;
  }
}
