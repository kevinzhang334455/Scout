
double f(double);

void g(double* b, double* c) 
{
  int i;
  double x;
  double* ptr;

#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    ptr = b;
    x = ptr[i] + b[i];   // shall alias

    ptr = c;
    c[i] = 2.0 * f(ptr[i]);
  }
}
