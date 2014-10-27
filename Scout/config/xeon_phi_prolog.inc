
/* prolog text added by scout */
#include <mmintrin.h>
#include <immintrin.h>


#define scout_mm512_loadu_ps( p ) _mm512_loadunpackhi_ps(_mm512_loadunpacklo_ps(_mm512_undefined_ps(), p), p+16)

void scout_mm512_storeu_ps(float* p, __m512 v)
{
  _mm512_packstorelo_ps(p, v);
  _mm512_packstorehi_ps(p+16, v);
}


#define scout_mm512_loadu_pd( p ) _mm512_loadunpackhi_pd(_mm512_loadunpacklo_pd(_mm512_undefined_pd(), p), p+8)

void scout_mm512_storeu_pd(double* p, __m512d v)
{
  _mm512_packstorelo_pd(p, v);
  _mm512_packstorehi_pd(p+8, v);
}


#define scout_mm512_loadu_epi32( p ) _mm512_loadunpackhi_epi32(_mm512_loadunpacklo_epi32(_mm512_undefined_epi32(), p), p+16)

void scout_mm512_storeu_epi32(int* p, __m512i v)
{
  _mm512_packstorelo_epi32(p, v);
  _mm512_packstorehi_epi32(p+16, v);
}


#define scout_mm512_loadu_8epi32( p ) _mm512_mask_loadunpackhi_epi32(_mm512_mask_loadunpacklo_epi32(_mm512_undefined_epi32(), 0xff, p), 0xff, p+16)


/* end of prolog text */
