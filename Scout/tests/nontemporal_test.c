
#ifndef DATATYPE
#define DATATYPE float
#endif

void call_test(DATATYPE* source1, DATATYPE* , DATATYPE* sink1, DATATYPE* sink2)
{
  int i, index;
  int modulo = 1;

#pragma scout loop vectorize nontemporal(sink, x) aligned(x)
  for (i = 0; i < 100; ++i)
  {
    index = i * modulo;
    sink1[index] = source[i];
    sink2[i] = source[i]; 
  }
}
