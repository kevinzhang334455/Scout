
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<float, 16>
{
  typedef float base_type;
  typedef base_type __m512 __attribute__ ((__vector_size__ (64)));

  typedef __m512 type;
  enum { align = 64 };

  // gather/scatter support:
  // gs_index_type is a reserved type name for the index type expected by g/s
  // get_uniform_gs_index is a reserved function getting called for uniform g/s distances
  // if get_uniform_gs_index does not exist, then the distance is directly issued to g/s 
  // (in that case gs_index_type must be a typedef to int)
  
  typedef int __m512i __attribute__ ((__vector_size__ (64)));
  typedef __m512i gs_index_type;

  static gs_index_type get_uniform_gs_index(int)
  {
    "_mm512_set_epi32(%1%*15, %1%*14, %1%*13, %1%*12, %1%*11, %1%*10, %1%*9, %1%*8, %1%*7, %1%*6, %1%*5, %1%*4, %1%*3, %1%*2, %1%, 0)";
  }

  static type gather(base_type*, gs_index_type)
  {
    "_mm512_i32gather_ps(%2%, %1%, _MM_SCALE_1)";
  }

  static void scatter(base_type*, gs_index_type, type)
  {
    "_mm512_i32scatter_ps(%1%, %2%, %3%, _MM_SCALE_1)";
  }
  // end of gather/scatter support

  // special function names defined by Scout:
  static void store_aligned(base_type*, type)
  {
    "_mm512_store_ps(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "_mm512_load_ps(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "scout_mm512_storeu_ps(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "scout_mm512_loadu_ps(%1%)";
  }

  static type splat(base_type)
  {
    "_mm512_set1_ps(%1%)";
  }

  /*static type broadcast(base_type*)
  {
    "_mm512_set1_ps(*%1%)";
  }*/

  static type set(base_type, base_type, base_type, base_type, 
                  base_type, base_type, base_type, base_type, 
                  base_type, base_type, base_type, base_type, 
                  base_type, base_type, base_type, base_type)
  {
    "_mm512_set_ps(%16%, %15%, %14%, %13%, %12%, %11%, %10%, %9%, %8%, %7%, %6%, %5%, %4%, %3%, %2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "*(((float*)(&%1%))+(%2%))";
  }

  static void insert(type, base_type, unsigned int)
  {
    "*(((float*)(&%1%))+(%3%)) = %2%";
  }


  // expression mapping:
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm512_add_ps(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm512_sub_ps(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm512_mul_ps(%1%, %2%)";
  }

  static base_type expression_div(base_type a, base_type b)
  {
    a / b;
    "_mm512_div_ps(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm512_sub_ps(_mm512_setzero_ps(), %1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm512_min_ps(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm512_max_ps(%1%, %2%)";
  }

  static base_type expression_condition_lt(base_type a, base_type b, base_type c, base_type d)
  {
    a < b ? c : d;
    "_mm512_mask_mov_ps(%4%, _mm512_cmp_ps_mask(%1%, %2%, _MM_CMPINT_LT), %3%)";
  }

  static base_type expression_condition_le(base_type a, base_type b, base_type c, base_type d)
  {
    a <= b ? c : d;
    "_mm512_mask_mov_ps(%4%, _mm512_cmp_ps_mask(%1%, %2%, _MM_CMPINT_LE), %3%)";
  }

  static base_type expression_condition_gt(base_type a, base_type b, base_type c, base_type d)
  {
    a > b ? c : d;
    "_mm512_mask_mov_ps(%4%, _mm512_cmp_ps_mask(%2%, %1%, _MM_CMPINT_LT), %3%)";
  }

  static base_type expression_condition_ge(base_type a, base_type b, base_type c, base_type d)
  {
    a >= b ? c : d;
    "_mm512_mask_mov_ps(%4%, _mm512_cmp_ps_mask(%2%, %1%, _MM_CMPINT_LE), %3%)";
  }

/*static bool expression_ge(type a, type b)
  {
    a >= b;
    "(_mm512_movemask_pd(_mm512_cmpge_pd(%1%, %2%)) == 3)";
  }*/


/*
  static type expression_fmadd(type a, type b, type c)
  {
    a * b + c;
    c + a * b;
    "_mm512_fmadd_ps(%1%, %2%, %3%)";
  }

  static type expression_fmsub(type a, type b, type c)
  {
    a * b - c;
    "_mm512_fmsub_ps(%1%, %2%, %3%)";
  }
*/

  // common global function name mapping 
  // (applicated, if the function name is not a special one):

  static base_type sqrt(base_type)
  {
    double sqrt(double);    
    "_mm512_sqrt_ps(%1%)";
  }

  static base_type fabs(base_type)
  {
    double fabs(double);      
    long double fabs(long double);  
    "_mm512_gmaxabs_ps(%1%, %1%)";   
  }

  static base_type exp(base_type)
  {
    double exp(double);      
    "_mm512_exp_ps(%1%)";   
  }
};

}  // namespace scout
