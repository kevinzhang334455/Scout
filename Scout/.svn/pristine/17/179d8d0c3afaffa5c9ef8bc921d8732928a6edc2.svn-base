
typedef struct 
{
  float x;
} A;

A g_A[100];

void f(float a[][10]) 
{
  int i;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    a[i][1] = g_A[i].x;
  }
}
