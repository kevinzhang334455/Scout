
void f(float* a, float* b, float c)
{
  *a = *b + c;
}


void h(float* a, float* b, float c) 
{
  int i;
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    f(a+i, b+i, c);
  }
}
