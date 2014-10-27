
void g(float y) 
{
  float x;
  x = 0.1 < (y) ? 0.1 : 0.2;
}

void f(float* a)
{
  int var, varK;
  float z;
#pragma scout loop vectorize 
  for (int i = 0; i < 100; ++i)
  {
    z = a[i];
    g(z);

/*#pragma scout condition invariant*/
    if (var == varK) {

    }
  }
}

