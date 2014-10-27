
void h(float* a, float* b, float c, float* d) 
{
  int i;
  int idx_1[100], idx_2[100];
#pragma scout loop vectorize 
  for (i = 0; i < 100; ++i)
  {
    float x = a[i] * c + b[i]; 
#pragma scout loop vectorize unroll    
    d[idx_1[i]] += x;
    d[idx_2[i]] -= x;
  }
}
