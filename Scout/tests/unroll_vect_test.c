
typedef struct { float x[5], y } A;


float half(float x)
{
  return x * 0.5;
}

void f(A* a)
{
  int i;
  for (i = 0; i < 5; ++i)
  {
    a->x[i] += half(a->y);
  }
}


void h(A* a) 
{
  int i;
#pragma scout loop vectorize force
  for (i = 0; i < 100; ++i)
  {
    f(a + i);
  }
}
