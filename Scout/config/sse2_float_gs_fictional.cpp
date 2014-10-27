
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<float, 4>
{
  typedef float base_type;
  typedef base_type __m128 __attribute__ ((__vector_size__ (16)));

  typedef __m128 type;
  enum { align = 16 };

  // gather/scatter support:
  // gs_index_type is a reserved type name for the index type expected by g/s
  // get_uniform_gs_index is a reserved function getting called for uniform g/s distances
  // if get_uniform_gs_index does not exist, then the distance is directly issued to g/s 
  // (in that case gs_index_type must be a typedef to int)
  typedef int __m128i __attribute__ ((__vector_size__ (16)));
  typedef __m128i gs_index_type;

  static gs_index_type get_uniform_gs_index(int)
  {
    "_mm_set_epi32(%1%*3, %1%*2, %1%, 0)";
  }

  static type gather(base_type*, gs_index_type)
  {
    "_mm_gather_ps(%2%, %1%)";
  }

  static void scatter(base_type*, gs_index_type, type)
  {
    "_mm_scatter_ps(%1%, %2%, %3%)";
  }
  // end of gather/scatter support


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

  static base_type fabs(base_type)
  {
    base_type fabs(base_type);    
    double fabs(double);      
    long double fabs(long double);      
    "_mm_andnot_ps(_mm_set1_ps(-0.0f), %1%)";
  }
};

}  // namespace scout
