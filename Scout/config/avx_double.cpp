
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<double, 4>
{
  typedef double base_type;
  typedef base_type __m256 __attribute__ ((__vector_size__ (32)));

  typedef __m256 type;
  enum { align = 32 };

  // special function names defined by Scout:
  static void store_aligned(base_type*, type)
  {
    "_mm256_store_pd(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "_mm256_load_pd(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "_mm256_storeu_pd(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "_mm256_loadu_pd(%1%)";
  }

  static type splat(base_type)
  {
    "_mm256_set1_pd(%1%)";
  }

  static type broadcast(base_type*)
  {
    "_mm256_broadcast_sd(%1%)";
  }

  static type set(base_type, base_type, base_type, base_type)
  {
    "_mm256_set_pd(%4%, %3%, %2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "_mm256_extract_pd(%1%, %2%)";       //msvc: %1%.m128_f32[%2%]
  }

  static void insert(type, base_type, unsigned int)
  {
    "scout_insert_pd(%1%, %2%, %3%)"; //"(*((float*)(&%1%)+%3%)) = %2%"
  }

#ifdef SCOUT_CONFIG_WITH_AVX2
  // gather/scatter support:
  // gs_index_type is a reserved type name for the index type expected by g/s
  // get_uniform_gs_index is a reserved function getting called for uniform g/s distances
  // if get_uniform_gs_index does not exist, then the distance is directly issued to g/s 
  // (in that case gs_index_type must be a typedef to int)
  typedef int __m128i __attribute__ ((__vector_size__ (16)));
  typedef __m128i gs_index_type;

  static gs_index_type get_uniform_gs_index(int)
  {
    "_mm_set_pi(%1%*3, %1%*2, %1%, 0)";
  }

  static type gather(base_type*, gs_index_type)
  {
    "_mm256_i32gather_pd(%1%, %2%, 1)";
  }
#endif

  // expression mapping:
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm256_add_pd(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm256_sub_pd(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm256_mul_pd(%1%, %2%)";
  }

  static base_type expression_div(base_type a, base_type b)
  {
    a / b;
    "_mm256_div_pd(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm256_sub_pd(_mm256_setzero_pd(), %1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm256_min_pd(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm256_max_pd(%1%, %2%)";
  }

  static base_type expression_condition_lt(base_type a, base_type b, base_type c, base_type d)
  {
    a < b ? c : d;
    "_mm256_blendv_pd(%4%, %3%, _mm256_cmp_pd(%1%, %2%, _CMP_LT_OQ))";
  }

  static base_type expression_condition_le(base_type a, base_type b, base_type c, base_type d)
  {
    a <= b ? c : d;
    "_mm256_blendv_pd(%4%, %3%, _mm256_cmp_pd(%1%, %2%, _CMP_LE_OQ))";
  }

  static base_type expression_condition_gt(base_type a, base_type b, base_type c, base_type d)
  {
    a > b ? c : d;
    "_mm256_blendv_pd(%4%, %3%, _mm256_cmp_pd(%1%, %2%, _CMP_GT_OQ))";
  }

  static base_type expression_condition_ge(base_type a, base_type b, base_type c, base_type d)
  {
    a >= b ? c : d;
    "_mm256_blendv_pd(%4%, %3%, _mm256_cmp_pd(%1%, %2%, _CMP_GE_OQ))";
  }

#ifdef SCOUT_CONFIG_WITH_AVX2
  static type expression_fmadd(type a, type b, type c)
  {
    a * b + c;
    c + a * b;
    "_mm256_fmadd_pd(%1%, %2%, %3%)";
  }
#endif

  // common global function name mapping 
  // (applicated, if the function name is not a special one):

  static base_type sqrt(base_type)
  {
    double sqrt(double);    
    "_mm256_sqrt_pd(%1%)";
  }

  static base_type exp(base_type)
  {
    double exp(double);    
    "_mm256_exp_pd(%1%)";
  }

  static base_type fabs(base_type)
  {
    double fabs(double);      
    long double fabs(long double);      
    "_mm256_andnot_pd(_mm256_set1_pd(-0.0), %1%)";
  }
};

}  // namespace scout
