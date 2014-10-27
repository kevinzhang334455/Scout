
typedef struct { float x; } Comp;

/* test unrollComplexLHS */
void f(float* a, float b, Comp* c, float* d) 
{
  int i;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    c[i].x = a[i] + b;
  }
}

/* test unrollComplexRHS */
void g(float* a, float b, Comp* c, float* d) 
{
  int i;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    a[i] = c[i].x + b;
  }
}
