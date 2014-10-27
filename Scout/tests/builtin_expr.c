extern float sqrt(float);

void g(float* a, float b) 
{
  int i;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    a[i] = sqrt(b);
  }
}

void h(float* a, float b) 
{
  int i;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    a[i] = sqrt(a[i]);
  }
}
