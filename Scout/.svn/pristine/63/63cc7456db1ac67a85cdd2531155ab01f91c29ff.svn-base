
typedef struct { float x[5], y; } A;

void f(A* a)
{
  float temp[5];
  float* pF = temp;
  float* pF1;
  int i;

  temp[0] = a->x[0];
  temp[1] = a->x[1];
  temp[2] = a->x[2];
  temp[3] = a->x[3];
  temp[4] = a->x[4];

  *temp += a->y;
  // *pF += a->y;
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
