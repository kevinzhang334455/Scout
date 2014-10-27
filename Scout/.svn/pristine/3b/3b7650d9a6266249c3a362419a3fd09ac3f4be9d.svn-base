
void foo(float* sink, int modulo)
{
  int i, index;
  float x[100], y[100];

#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    index = i * modulo;
    sink[index] = x[i] + y[index];
  }
}
