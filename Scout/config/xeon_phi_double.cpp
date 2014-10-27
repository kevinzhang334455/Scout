
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<double, 8>
{
  typedef double base_type;
  typedef base_type __m512d __attribute__ ((__vector_size__ (64)));

  typedef __m512d type;
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
    "_mm512_set_epi32(0, 0, 0, 0, 0, 0, 0, 0, %1%*7, %1%*6, %1%*5, %1%*4, %1%*3, %1%*2, %1%, 0)";
  }

  static type gather(base_type*, gs_index_type)
  {
    "_mm512_i32logather_pd(%2%, %1%, _MM_SCALE_1)";
  }

  static void scatter(base_type*, gs_index_type, type)
  {
    "_mm512_i32loscatter_pd(%1%, %2%, %3%, _MM_SCALE_1)";
  }
  // end of gather/scatter support

  // special function names defined by Scout:
  // special function names defined by Scout:
  static void store_aligned(base_type*, type)
  {
    "_mm512_store_pd(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "_mm512_load_pd(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "scout_mm512_storeu_pd(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "scout_mm512_loadu_pd(%1%)";
  }

  static type splat(base_type)
  {
    "_mm512_set1_pd(%1%)";
  }

  /*static type broadcast(base_type*)
  {
    "_mm512_set1_pd(*%1%)";
  }*/

  static type set(base_type, base_type, base_type, base_type, base_type, base_type, base_type, base_type)
  {
    "_mm512_set_pd(%8%, %7%, %6%, %5%, %4%, %3%, %2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "*(((double*)(&%1%))+(%2%))";
    //"%1%.__m512d_f64[%2%]";
  }

  static void insert(type, base_type, unsigned int)
  {
    "*(((double*)(&%1%))+(%3%)) = %2%";
    //"%1%.__m512d_f64[%3%] = %2%";
  }


  // expression mapping:
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm512_add_pd(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm512_sub_pd(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm512_mul_pd(%1%, %2%)";
  }

  static base_type expression_div(base_type a, base_type b)
  {
    a / b;
    "_mm512_div_pd(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm512_sub_pd(_mm512_setzero_pd(), %1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm512_min_pd(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm512_max_pd(%1%, %2%)";
  }

  static base_type expression_condition_lt(base_type a, base_type b, base_type c, base_type d)
  {
    a < b ? c : d;
    "_mm512_mask_mov_pd(%4%, _mm512_cmp_pd_mask(%1%, %2%, _MM_CMPINT_LT), %3%)";
  }

  static base_type expression_condition_le(base_type a, base_type b, base_type c, base_type d)
  {
    a <= b ? c : d;
    "_mm512_mask_mov_pd(%4%, _mm512_cmp_pd_mask(%1%, %2%, _MM_CMPINT_LE), %3%)";
  }

  static base_type expression_condition_gt(base_type a, base_type b, base_type c, base_type d)
  {
    a > b ? c : d;
    "_mm512_mask_mov_pd(%4%, _mm512_cmp_pd_mask(%2%, %1%, _MM_CMPINT_LT), %3%)";
  }

  static base_type expression_condition_ge(base_type a, base_type b, base_type c, base_type d)
  {
    a >= b ? c : d;
    "_mm512_mask_mov_pd(%4%, _mm512_cmp_pd_mask(%2%, %1%, _MM_CMPINT_LE), %3%)";
  }

/*static bool expression_ge(type a, type b)
  {
    a >= b;
    "(_mm512_movemask_pd(_mm512_cmpge_pd(%1%, %2%)) == 3)";
  }*/

  static type expression_fmadd(type a, type b, type c)
  {
    a * b + c;
    c + a * b;
    "_mm512_fmadd_pd(%1%, %2%, %3%)";
  }

  static type expression_fmsub(type a, type b, type c)
  {
    a * b - c;
    "_mm512_fmsub_pd(%1%, %2%, %3%)";
  }


  // common global function name mapping 
  // (applicated, if the function name is not a special one):

  static base_type sqrt(base_type)
  {
    double sqrt(double);    
    "_mm512_sqrt_pd(%1%)";
  }

  static base_type fabs(base_type)
  {
    double fabs(double);      
    long double fabs(long double);      
    "_mm512_castsi512_pd(_mm512_andnot_epi64(_mm512_castpd_si512(_mm512_set_1to8_pd(-0.0)), _mm512_castpd_si512(%1%)))";
  }

  static base_type exp(base_type)
  {
    double exp(double);      
    "_mm512_exp_pd(%1%)";   
  }
};

}  // namespace scout
