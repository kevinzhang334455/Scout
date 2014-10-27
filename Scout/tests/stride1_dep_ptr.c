typedef struct { float a, b } A;


void foo(float* sink)
{
  int i;
  A x[100];
  float* ptr;

#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    ptr = &x[i].a;
    *ptr += x[i].b;
  }
}
