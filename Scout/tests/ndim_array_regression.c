
typedef struct 
{
  double x;
} A;

A g_A;

void f(double a[][10]) 
{
  int i;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    a[i][1] = g_A.x;
  }
}
