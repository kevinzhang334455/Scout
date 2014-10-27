
void f(float a[5])
{
  int i;
  for (i = 0; i < 5; ++i)  
  {
    a[i] += 1;
  }
}

void h() 
{
  int i;
  float a[100][5];
#pragma scout loop vectorize scalar
  for (i = 0; i < 100; ++i)
  {
    f(a[i]);
  }
}
