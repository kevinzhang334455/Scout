
namespace scout {

template<class T, unsigned size> struct config;

// this configuration is a helper to prevent unrolling of doubles
// hence it has no store functions. Use config<int, 16> for computations!
// TODO: The absence of store functions is detected by Scout and the configuration 
// is choosen appropriately.
// WORKAROUD: Use xeon_phi_double_only.cpp

template<>
struct config<int, 8>
{
  typedef int base_type;
  typedef base_type __m512i __attribute__ ((__vector_size__ (64)));

  typedef __m512i type;
  enum { align = 64 };

  // gather/scatter support:
  // gs_index_type is a reserved type name for the index type expected by g/s
  // get_uniform_gs_index is a reserved function getting called for uniform g/s distances
  // if get_uniform_gs_index does not exist, then the distance is directly issued to g/s 
  // (in that case gs_index_type must be a typedef to int)
  typedef __m512i gs_index_type;

  static gs_index_type get_uniform_gs_index(int)
  {
    "_mm512_set_epi32(0, 0, 0, 0, 0, 0, 0, 0, %1%*7, %1%*6, %1%*5, %1%*4, %1%*3, %1%*2, %1%, 0)";
  }

  static type gather(base_type*, gs_index_type)
  {
    "_mm512_mask_i32gather_epi32(_mm512_undefined_epi32(), 0xff, %2%, %1%, _MM_SCALE_1)";
  }
  // end of gather/scatter support

  // special function names defined by Scout:
  static type load_aligned(base_type*)
  {
    "_mm512_mask_load_epi32(_mm512_undefined_epi32(), 0xff, %1%)";
  }

  static type load_unaligned(base_type*)
  {
    "scout_mm512_loadu_8epi32(%1%)"
  }

  static type splat(base_type)
  {
    "_mm512_set1_epi32(%1%)";
  }

  static type set(base_type, base_type, base_type, base_type, 
                  base_type, base_type, base_type, base_type)
  {
    "_mm512_set_epi32(0,0,0,0,0,0,0,0, %7%, %6%, %5%, %4%, %3%, %2%, %1%)";
  }

  static base_type extract(type, unsigned int)
  {
    "*(((int*)(&%1%))+(%2%))";
    //"%1%.__m512i_i32[%2%]";
  }

  static void insert(type, base_type, unsigned int)
  {
    "*(((int*)(&%1%))+(%3%)) = %2%";
    //"%1%.__m512i_i32[%3%] = %2%";
  }


  // expression mapping:
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "_mm512_add_epi32(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "_mm512_sub_epi32(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "_mm512_fmadd_epi32(%1%, %2%, _mm512_setzero_epi32())";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "_mm512_sub_epi32(_mm512_setzero_epi32(), %1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "_mm512_min_epi32(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "_mm512_max_epi32(%1%, %2%)";
  }

  static base_type expression_condition_lt(base_type a, base_type b, base_type c, base_type d)
  {
    a < b ? c : d;
    "_mm512_mask_mov_epi32(%4%, _mm512_cmp_epi32_mask(%1%, %2%, _MM_CMPINT_LT), %3%)";
  }

  static base_type expression_condition_le(base_type a, base_type b, base_type c, base_type d)
  {
    a <= b ? c : d;
    "_mm512_mask_mov_epi32(%4%, _mm512_cmp_epi32_mask(%1%, %2%, _MM_CMPINT_LE), %3%)";
  }

  static base_type expression_condition_gt(base_type a, base_type b, base_type c, base_type d)
  {
    a > b ? c : d;
    "_mm512_mask_mov_epi32(%4%, _mm512_cmp_epi32_mask(%2%, %1%, _MM_CMPINT_LT), %3%)";
  }

  static base_type expression_condition_ge(base_type a, base_type b, base_type c, base_type d)
  {
    a >= b ? c : d;
    "_mm512_mask_mov_epi32(%4%, _mm512_cmp_epi32_mask(%2%, %1%, _MM_CMPINT_LE), %3%)";
  }

  static type expression_fmadd(type a, type b, type c)
  {
    a * b + c;
    c + a * b;
    "_mm512_fmadd_epi32(%1%, %2%, %3%)";
  }
};

}  // namespace scout