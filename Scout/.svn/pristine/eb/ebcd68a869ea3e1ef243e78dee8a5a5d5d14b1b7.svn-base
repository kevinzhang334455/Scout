
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<int, 4>
{
  typedef int base_type;
  typedef base_type __m128i __attribute__ ((__vector_size__ (16)));

  typedef __m128i type;
  enum { align = 16 };

  // special function names defined by Scout:

  static type splat(base_type)
  {
    "_mm_set1_epi32(%1%)";
  }

  static type set(base_type, base_type, base_type, base_type)
  {
    "_mm_set_epi32(%4%, %3%, %2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "_mm_extract_epi32(%1%, %2%)";       //msvc: %1%.m128_f32[%2%]
  }

  static void insert(type, base_type, unsigned int)
  {
    "_mm_insert_epi32(%1%, %2%, %3%)"; //"(*((float*)(&%1%)+%3%)) = %2%"
  }

  static void store_aligned(base_type*, type)
  {
    "_mm_store_epi32(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "_mm_load_epi32(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "_mm_storeu_epi32(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "_mm_loadu_epi32(%1%)";
  }

  // expression mapping (function name starts with expression_):
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm_add_epi32(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm_sub_epi32(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm_mul_epi32(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm_sub_epi32(_mm_setzero_si128(), %1%)";
  }

#ifdef SCOUT_CONFIG_WITH_SSE4
  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm_min_epi32(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm_max_epi32(%1%, %2%)";
  }
#endif
};

}  // namespace scout
