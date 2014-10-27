
#include "preprocess_regression.h"
/* there is a pragma in the include before line 13! */

void f(float* a, float b, int* c) 
{
  int i, mode;
  float d[100];
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
/* the condition is NOT invariant! */
    if (c[i] == 0)
    {
      d[i] = a[i] + b;
    }
    else
    {
      d[i] = a[i] - b;
    }
    d[i] = a[i] - b;
  }
}

