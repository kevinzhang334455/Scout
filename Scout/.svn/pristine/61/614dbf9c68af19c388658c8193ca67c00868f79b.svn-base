
#define TEST 1
#define ARG_TEST(ARG) (ARG)

#define COND_TEST(b, x, y) ((b < 0.0) ? (x) : (y))

typedef struct { float* member; } A;

#define A_MEM (a.member)

void g(A a, float b, float* c, float* d) 
{
  int i, j, k;
#pragma scout loop vectorize 
  for (i = j; i < k; ++i)
  {
    A_MEM[i] = COND_TEST(b, c[i], d[ARG_TEST(i)]) + c[ARG_TEST(i+TEST)];
  }
}
