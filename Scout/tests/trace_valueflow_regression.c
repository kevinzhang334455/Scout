/*  trace:float leads to mixing data types in matInv3D called by ilu_decomp */

typedef struct
{ 
  double c[4];
} tMatrix;

void f(tMatrix* a) 
{
  int i;
  double t[4], r[100];
  double *p0, *p1, *p2, *p3;
  double pivot;
  tMatrix* p;
#pragma scout loop vectorize
  for (i = 0; i < 100; ++i)
  {
    p = &a[i];
    t[0] = p->c[0];
    //t[1] = p->c[1];
    //t[2] = p->c[2];
    //t[3] = p->c[3];

    p0 = &(t[0]);

    // remove the +0 to get it to work properly
    p1 = p0+0;

    r[i] = 1.000000e+00 / (*p1 + 1.000000e-12);
/*
    pivot = 1.000000e+00 / (*p1 + 1.000000e-12);
    p1 = p0;
    *p1 = *p1 * pivot;

    p1 = p0+0;
    p2 = p0+1;
    p3 = p0+2;
    *p1 = *p1 + *p2 * *p3;
    p->c[0] = t[0];
    p->c[1] = t[1];
    p->c[2] = t[2];
    p->c[3] = t[3];
*/
  }
}
