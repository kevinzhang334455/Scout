

#ifndef SCOUT_NEON_SIMD_H
#define SCOUT_NEON_SIMD_H

#include	<arm_neon.h>


inline float32x4_t scout_vdivq_f32(float32x4_t a, float32x4_t b)
{
  float32x4_t q_inv0 = vrecpeq_f32(b);
  float32x4_t q_step0 = vrecpsq_f32(q_inv0, b);
  float32x4_t q_inv1 = vmulq_f32(q_step0, q_inv0);
  // add more steps for more accuracy  

  return vmulq_f32(a, q_inv1);
}

inline float32x4_t scout_vsqrtq_f32(float32x4_t a)
{
  // very precise:
  float32x4_t q_step_0 = vrsqrteq_f32( a );
  // step
  float32x4_t q_step_parm0 = vmulq_f32( a, q_step_0 );
  float32x4_t q_step_result0 = vrsqrtsq_f32( q_step_parm0, q_step_0 );
  // step
  float32x4_t q_step_1 = vmulq_f32( q_step_0, q_step_result0 );
  float32x4_t q_step_parm1 = vmulq_f32( a, q_step_1 );
  float32x4_t q_step_result1 = vrsqrtsq_f32( q_step_parm1, q_step_1 );
  // take the res
  float32x4_t q_step_2 = vmulq_f32( q_step_1, q_step_result1 );
  // mul by x to get sqrt, not rsqrt
  return vmul_f32( a, d_step_rlen2 );
}

#endif

