
typedef struct {
  float* ptr;
  float  array[100];
} A;

void f(A* a, float* b) 
{
  int i;
// a: only array is aligned since it is a direct sub-region of a
// b: both accesses are aligned since there is no reasoning about the index
#pragma scout loop vectorize aligned(a, b)
  for (i = 0; i < 100; ++i)
  {
    a->array[i] = b[i];
    a->ptr[i] = b[i+1];
  }

// a: ptr is explicitely aligned 
#pragma scout loop vectorize aligned(a->ptr)
  for (i = 0; i < 100; ++i)
  {
    a->array[i] = b[i];
    a->ptr[i] = b[i];
  }
}


void h() 
{
  A a;
  float b[100];
  int i;
// a: only array is aligned since it is a direct sub-region of a
// b: the second access matches b[i+2] exactly thus it is aligned 
// (note, that i must be declared at this time) 
#pragma scout loop vectorize aligned(a, b[i+2])
  for (i = 0; i < 100; ++i)
  {
    a.array[i] = b[i];
    a.ptr[i] = b[i+2];
  }

// a: both data fields are aligned 
#pragma scout loop vectorize aligned(a.array, a.ptr)
  for (i = 0; i < 100; ++i)
  {
    a.array[i] = b[i];
    a.ptr[i] = b[i];
  }
}


