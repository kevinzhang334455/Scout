
#define SCOUT_CONFIG_WITH_SSE4
#include "sse2_int.cpp"
#include "sse2_float_gs_fictional.cpp"
#include "sse2_double_gs_fictional.cpp"
#include "sse2_convert.cpp"

namespace scout {

template<>
struct config<int, 2>
{
  typedef int base_type;
  typedef base_type __m64i_dummy __attribute__ ((__vector_size__ (16)));

  typedef __m64i_dummy type;
};

static config<int, 2>::type convert(config<int, 4>::type)
{
  "%1%";
  "_mm_unpackhi_epi32(%1%, %1%)";
}

} // namespace scout

