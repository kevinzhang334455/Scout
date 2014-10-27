
void riemann_zeta()
{
  int i, k, m, n;
  int start_index, end_index, unroll;
  float l[4], p_sum[4], d_pow[4];
#pragma scout loop vectorize noremainder
  for(i=0;i < 4;i++) 
  {
    p_sum[i] = 0.0;
    for(k = start_index; k > end_index; k-=unroll) 
    {
      d_pow[i] = l[i];
      for (m = n; m > 1; m--) 
      {
        d_pow[i] *= l[i];
      }
      p_sum[i] += 1.0/d_pow[i];
      l[i] -= 4.0;
    }
  }
}
