
double a[100][3];

void f() 
{
  int i;
  double x;
#pragma scout loop vectorize size(16)
  for (i = 0; i < 100; ++i)
  {
    x = a[i][0];
  }
}
