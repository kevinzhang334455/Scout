typedef struct { float a, b } A;


void foo(float* sink)
{
  int i;
  A x[100];
  float* ptr;

#pragma scout loop vectorize force align
  for (i = 0; i < 100; ++i)
  {
    ptr = sink + i;
    *ptr += x[i].b;
  }
}
