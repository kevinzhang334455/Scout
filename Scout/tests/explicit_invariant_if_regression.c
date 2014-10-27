
/* 
note the wrong reference to a splitted variable,
if the condition is explicitely marked invariant 
and the variable is declared inside the loop body
*/

void f(float* a, int* c) 
{
  int mode;
  float d[100];
#pragma scout loop vectorize scalar
  for (int i = 0; i < 100; ++i)
  {
    float b = a[i];
#pragma scout condition invariant
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
