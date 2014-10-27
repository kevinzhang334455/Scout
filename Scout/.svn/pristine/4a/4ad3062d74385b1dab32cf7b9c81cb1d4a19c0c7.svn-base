
void f(float* a, float b, int* c) 
{
  int i, mode;
  float d[100];
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    mode = 0;
    if (mode == 0)
    {
      d[i] = a[i] + b;
    }
    else
    {
      d[i] = a[i] - b;
    }
  }
}
