
/* there is NO pragma in the including source before line 13! */



void foo(float* a) 
{
  int i, mode = 0;
  float d[100];
  for (i = 0; i < 100; ++i)
  {
#pragma scout condition invariant
    if (mode == 0)
    {
      d[i] = a[i];
    }
  }
}
