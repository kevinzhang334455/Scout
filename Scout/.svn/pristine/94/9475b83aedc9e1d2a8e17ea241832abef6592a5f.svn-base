
void f(float* a, float b, int* c) 
{
  int i;
  float d[100];
  float* p;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    if (c[i])
    { 
      p = a+i;
      d[i] = *p + b;
    }
  }
}
