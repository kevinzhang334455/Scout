
namespace scout {

template<class T, unsigned size> struct config;

template<>
struct config<float, 4>
{
  typedef float base_type;
  typedef base_type float32x4_t __attribute__ ((__vector_size__ (16)));

  typedef float32x4_t type;
  enum { align = 16 };

  // special function names defined by Scout:
  static void store_aligned(base_type*, type)
  {
    "vst1q_f32(%1%, %2%)";
  }

  static type load_aligned(base_type*)
  {
    "vld1q_f32(%1%)";
  }

  static void store_unaligned(base_type*, type)
  {
    "vst1q_f32(%1%, %2%)";
  }

  static type load_unaligned(base_type*)
  {
    "vld1q_f32(%1%)";
  }

  static type splat(base_type)
  {
    "vdupq_n_f32(%1%)";    
  }

  static type broadcast(base_type*)
  {
    "vdupq_n_f32(*%1%)";
  }

  /*TODO: 
  static type set(base_type, base_type, base_type, base_type, base_type, base_type, base_type, base_type)
  {
    "_mm256_set_ps(%8%, %7%, %6%, %5%, %4%, %3%, %2%, %1%)";
  }*/

  static base_type extract(type, unsigned int)
  {
    "vgetq_lane_f32(%1%, %2%)";       //msvc: %1%.m128_f32[%2%]
  }

  /*TODO: 
  static void insert(type, base_type, unsigned int)
  {
    "scout_insert_ps(%1%, %2%, %3%)"; //"(*((float*)(&%1%)+%3%)) = %2%"
  }*/

  // expression mapping:
  static base_type expression_add(base_type a, base_type b)
  {
    a + b;
    "vaddq_f32(%1%, %2%)";
  }

  static base_type expression_sub(base_type a, base_type b)
  {
    a - b;
    "vsubq_f32(%1%, %2%)";
  }

  static base_type expression_mul(base_type a, base_type b)
  {
    a * b;
    "vmulq_f32(%1%, %2%)";
  }

  static base_type expression_div(base_type a, base_type b)
  {
    a / b;
    "scout_vdivq_f32(%1%, %2%)";
  }

  static base_type expression_neg(base_type a)
  {
    -a;
    "vnegq_f32(%1%)";
  }

  static base_type expression_min(base_type a, base_type b)
  {
    a < b ? a : b;
    a <= b ? a : b;
    "vminq_f32(%1%, %2%)";
  }

  static base_type expression_max(base_type a, base_type b)
  {
    a > b ? a : b;
    a >= b ? a : b;
    "vmaxq_f32(%1%, %2%)";
  }

  // common global function name mapping 
  // (applicated, if the function name is not a special one):

  static base_type sqrt(base_type)
  {
    double sqrt(double);    
    "scout_vsqrtq_f32(%1%)";
  }

  // exp for NEON?

  static base_type fabs(base_type)
  {
    double fabs(double);      
    long double fabs(long double);      
    float fabsf(float);
    "vabsq_f32(%1%)";
  }
};

}  // namespace scout
