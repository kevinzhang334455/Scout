
void foo(int* sink, int x, int y)
{
  int temp[2];
  int* ptr;

  ptr = temp;

  temp[0] = x;
  temp[1] = y;
  *sink = ptr[0];   // (1)
  *sink = ptr[1];   // (2)
}
