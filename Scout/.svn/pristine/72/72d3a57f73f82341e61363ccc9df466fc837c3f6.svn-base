typedef	float	Fcol[5];
typedef	struct
{
    Fcol	col0;
    Fcol	col1;
    Fcol	col2;
    Fcol	col3;
    Fcol	col4;

} Fmatrix;


void bar(Fmatrix *a, const Fmatrix* b)
{
  a->col0[0] = b->col0[0];
}

void foo(Fmatrix* a, float* b)
{
  Fmatrix mat;
#pragma scout loop vectorize force
  for (int i = 0; i < 100; ++i)
  {
    (&mat)->col0[0] = b[i];
    bar(&a[i], &mat);

    //(a[i]).col0[0] = (&mat)->col0[0];
  }
}
