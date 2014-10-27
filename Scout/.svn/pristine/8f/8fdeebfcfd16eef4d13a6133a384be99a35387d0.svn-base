
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<float, 4>
{
  typedef float base_type;
  typedef base_type __m128 __attribute__ ((__vector_size__ (16)));

  typedef __m128 type;
  enum { align = 16 };

  // special function names defined by Scout:

  static type splat(base_type)
  {
    "_mm_set1_ps(%1%)";
  }

  static type set(base_type, base_type, base_type, base_type)
  {
    "_mm_set_ps(%4%, %3%, %2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "_mm_extract_ps(%1%, %2%)";       //msvc: %1%.m128_f32[%2%]
  }

  static void insert(type, base_type, unsigned int)
  {
    "scout_insert_ps(%1%, %2%, %3%)"; //"(*((float*)(&%1%)+%3%)) = %2%"
  }

  static void store_aligned(base_type*, type)
  {
    "_mm_store_ps(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "_mm_load_ps(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "_mm_storeu_ps(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "_mm_loadu_ps(%1%)";
  }

#ifdef SCOUT_CONFIG_WITH_SSE4
  static void store_nt_scalar(base_type*, type)
  {
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(-1,0,0,0), (char*)%1%)";
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(0,-1,0,0), (char*)(%1%)-4)";
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(0,0,-1,0), (char*)(%1%)-8)";
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(0,0,0,-1), (char*)(%1%)-12)";
  }

  static void store_nt_packed_unaligned(base_type*, type)
  {
    "_mm_maskmoveu_si128((__m128i)%2%, _mm_set_epi32(-1,-1,-1,-1), (char*)%1%)";
  }

  static void store_nt_packed_aligned(base_type*, type)
  {
    "_mm_stream_ps(%1%, %2%)";
  }
#endif

  // expression mapping (function name starts with expression_):
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm_add_ps(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm_sub_ps(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm_mul_ps(%1%, %2%)";
  }

  static base_type expression_div(base_type a, base_type b)
  {
    a / b;
    "_mm_div_ps(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm_sub_ps(_mm_setzero_ps(), %1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm_min_ps(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm_max_ps(%1%, %2%)";
  }

#ifdef SCOUT_CONFIG_WITH_SSE4
  static base_type expression_condition(base_type a, base_type b, base_type c, base_type d)
  {
    a < b ? c : d;
    "_mm_blendv_ps(%4%, %3%, _mm_cmplt_ps(%1%, %2%))";
  }

  static base_type expression_condition_le(base_type a, base_type b, base_type c, base_type d)
  {
    a <= b ? c : d;
    "_mm_blendv_ps(%4%, %3%, _mm_cmple_ps(%1%, %2%))";
  }

  static base_type expression_condition_gt(base_type a, base_type b, base_type c, base_type d)
  {
    a > b ? c : d;
    "_mm_blendv_ps(%4%, %3%, _mm_cmpgt_ps(%1%, %2%))";
  }

  static base_type expression_condition_ge(base_type a, base_type b, base_type c, base_type d)
  {
    a >= b ? c : d;
    "_mm_blendv_ps(%4%, %3%, _mm_cmpge_ps(%1%, %2%))";
  }
#endif

  // common global function name mapping 
  // (applicated, if the function name is not a special one):

  static base_type sqrt(base_type)
  {
    base_type sqrt(base_type);      // source function signature list
    base_type sqrtf(base_type);    
    double sqrt(double);    
    "_mm_sqrt_ps(%1%)";
  }

  static base_type exp(base_type)
  {
    base_type expf(base_type);    
    double exp(double);    
    "_mm_exp_ps(%1%)";
  }

  static base_type fabs(base_type)
  {
    base_type fabsf(base_type);    
    double fabs(double);      
    long double fabs(long double);      
    "_mm_andnot_ps(_mm_set1_ps(-0.0f), %1%)";
  }
};

}  // namespace scout
