

#ifndef SCOUT_SIMD_H
#define SCOUT_SIMD_H

/* 
  replace with the appropriate sse header, if
  <ia32intrin.h> does not exist on your platform:
*/
#include	<ia32intrin.h>


/* 
  insertion is the same for SSE and AVX, 
  but if there are available intrinsics, use them
*/

#ifndef _mm_extract_ps
#define _mm_extract_ps(R, I) (*((float*)(&R)+I))
#endif

#ifndef _mm_extract_pd
#define _mm_extract_pd(R, I) (*((double*)(&R)+I))
#endif

#ifndef _mm256_extract_ps
#define _mm256_extract_ps(R, I) (*((float*)(&R)+I))
#endif

#ifndef _mm256_extract_pd
#define _mm256_extract_pd(R, I) (*((double*)(&R)+I))
#endif

/* insertion is the same for SSE and AVX, no intrinsics avail */
#define scout_insert_ps(R, F, I) (*((float*)(&R)+I)) = F
#define scout_insert_pd(R, F, I) (*((double*)(&R)+I)) = F

#endif

