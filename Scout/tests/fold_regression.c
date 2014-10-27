
// regression: crash due to folding
// fix: compute parent before folding, since folding can replace 
// the node altogether with a literal
void f(float* a, int b) 
{
#pragma scout loop vectorize
  for (int i = 0; i < b; ++i)
  {
    a[i*2] = a[i]; 
  }
}
