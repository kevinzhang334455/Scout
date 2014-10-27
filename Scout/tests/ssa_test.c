/*
Should scout vectorize this? 
It is not possible with our current approach but are we forced to detect this? 
*/ 

void g(float* a, float b, float* c, float* d) 
{
  int i;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    a[i] = b;
    b = c[i];
    d[i] = b * b;
  }
}
