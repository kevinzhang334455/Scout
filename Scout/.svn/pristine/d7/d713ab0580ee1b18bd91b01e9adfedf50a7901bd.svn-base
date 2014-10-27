
#include "xeon_phi_float.cpp"
#include "xeon_phi_double.cpp"
#include "xeon_phi_int.cpp"

namespace scout {

static void vectorized_loop_prolog()
{
  "#pragma noprefetch";
}

static void vectorized_loop_epilog()
{
  "#pragma noprefetch end";
}

}

namespace scout {

template<>
struct config<int, 8>
{
  typedef int base_type;
  typedef base_type __m256i_dummy __attribute__ ((__vector_size__ (32)));

  typedef __m256i_dummy type;
};

static config<int, 8>::type convert(config<int, 16>::type)
{
  "%1%";
  "_mm512_permute4f128_epi32(%1%, _MM_PERM_BADC)";
}

} // namespace scout

