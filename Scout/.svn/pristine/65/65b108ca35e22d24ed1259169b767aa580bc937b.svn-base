

typedef double (* const t3dPtr)[4][3]; 
typedef double (* const t2dPtr)[3]; 

double a[100][4][3];

void f() 
{
  int i;
  double x, y, z, zz;
  const double* p, *r;

#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    x = a[i][1][1];

    p = a[i][0];
    y = p[0];

    t3dPtr q = a;
    r = q[i][3];
    z = r[2];

    t2dPtr f = a[i];
    zz = f[2][1];
  }
}
