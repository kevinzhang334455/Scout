
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<double, 2>
{
  typedef double base_type;
  typedef base_type __m128d __attribute__ ((__vector_size__ (16)));

  typedef __m128d type;
  enum { align = 16 };

  // special function names defined by Scout:

  static type splat(base_type)
  {
    "_mm_set1_pd(%1%)";
  }

  static type set(base_type, base_type)
  {
    "_mm_set_pd(%2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "_mm_extract_pd(%1%, %2%)";       //msvc: %1%.m128_f32[%2%]
  }

  static void insert(type, base_type, unsigned int)
  {
    "scout_insert_pd(%1%, %2%, %3%)"; //"(*((float*)(&%1%)+%3%)) = %2%"
  }

  static void store_aligned(base_type*, type)
  {
    "_mm_store_pd(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "_mm_load_pd(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "_mm_storeu_pd(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "_mm_loadu_pd(%1%)";
  }

#ifdef SCOUT_CONFIG_WITH_SSE4
  static void store_nt_scalar(base_type*, type)
  {
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(-1,-1,0,0), (char*)%1%)";
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(0,0,-1,-1), (char*)(%1%)-8)";
  }

  static void store_nt_packed_unaligned(base_type*, type)
  {
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(-1,-1,-1,-1), (char*)%1%)";
  }

  static void store_nt_packed_aligned(base_type*, type)
  {
    "_mm_stream_pd(%1%, %2%)";
  }
#endif

  // expression mapping (function name starts with expression_):
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm_add_pd(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm_sub_pd(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm_mul_pd(%1%, %2%)";
  }

  static base_type expression_div(base_type a, base_type b)
  {
    a / b;
    "_mm_div_pd(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm_sub_pd(_mm_setzero_pd(), %1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm_min_pd(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm_max_pd(%1%, %2%)";
  }

#ifdef SCOUT_CONFIG_WITH_SSE4
  static base_type expression_condition_lt(base_type a, base_type b, base_type c, base_type d)
  {
    a < b ? c : d;
    "_mm_blendv_pd(%4%, %3%, _mm_cmplt_pd(%1%, %2%))";
  }

  static base_type expression_condition_le(base_type a, base_type b, base_type c, base_type d)
  {
    a <= b ? c : d;
    "_mm_blendv_pd(%4%, %3%, _mm_cmple_pd(%1%, %2%))";
  }

  static base_type expression_condition_gt(base_type a, base_type b, base_type c, base_type d)
  {
    a > b ? c : d;
    "_mm_blendv_pd(%4%, %3%, _mm_cmpgt_pd(%1%, %2%))";
  }

  static base_type expression_condition_ge(base_type a, base_type b, base_type c, base_type d)
  {
    a >= b ? c : d;
    "_mm_blendv_pd(%4%, %3%, _mm_cmpge_pd(%1%, %2%))";
  }
#endif

/*
  typedef int condition_type;

  static condition_type expression_ge(type a, type b)
  {
    a >= b;
    "_mm_movemask_pd(_mm_cmpge_pd(%1%, %2%))";
  }

  static bool all_true(condition_type)
  {
    "%1% == 3"
  }

  static bool all_false(condition_type)
  {
    "%1% == 0"
  }

  static bool test_condition(condition_type, int)
  {
    // index from 0 to vs - 1
    "%1% & (0x01 << %2%)"
  }
*/

  // common global function name mapping 
  // (applicated, if the function name is not a special one):


  static base_type sqrt(base_type)
  {
    double sqrt(double);    
    "_mm_sqrt_pd(%1%)";
  }

  static base_type invsqrt(base_type a, base_type b)
  {
    double sqrt(double);    
    a / sqrt(b);    
    "_mm_invsqrt_pd(%1%, %2%)";
  }

  static base_type fabs(base_type)
  {
    double fabs(double);      
    long double fabs(long double);      
    "_mm_andnot_pd(_mm_set1_pd(-0.0f), %1%)";
  }
};

}  // namespace scout
