#ifndef SSE_H
#define SSE_H

#ifdef __SSE2__ 

#include <xmmintrin.h>

/**
 * Multiplie le tableau de float from par w et sauvegarde dans to
 * @param nb Nombre d'éléments dans le tableau
 */

inline void mult(float* to, float* from, float w, int nb) {
  while( (intptr_t)from & 0x0f && nb--) *to++ = (float) w * *from++;
  __m128* SRC = (__m128*) from;
  __m128 *DST = (__m128*) to;
  __m128 W = _mm_set1_ps(w);
  nb = (nb+3)/4;
  for(int i = 0; i < nb; i++) DST[i] = W * SRC[i];
}

/**
 * Ajoute au tableau to le produit de from par w
 * @param nb Nombre d'éléments dans le tableau
 */

inline void add_mult(float* to, float* from, float w, int nb) {
  while( (intptr_t)from & 0x0f && nb--) *to++ += (float) w * *from++;
  __m128* SRC = (__m128*) from;
  __m128 *DST = (__m128*) to;
  __m128 W = _mm_set1_ps(w);
  nb = (nb+3)/4;
  for(int i = 0; i < nb; i++) DST[i] += W * SRC[i];
}

inline void multiplex(float* T, float* F1, float* F2, float* F3, float* F4, int nb) {
  __m128* DST = (__m128*) T;
  for(int i = 0; i < nb; i++) 
    DST[i] = _mm_setr_ps(F1[i], F2[i], F3[i], F4[i]);
}

inline void demultiplex(float* T1, float* T2, float* T3, float* T4, float* F, int nb) {
  nb = (nb+3)/4;
  __m128* _F = (__m128*) F;
  __m128* _T1 = (__m128*) T1;
  __m128* _T2 = (__m128*) T2;
  __m128* _T3 = (__m128*) T3;
  __m128* _T4 = (__m128*) T4;

  for(int i = 0; i < nb; i++) {
      __m128 L02 = _mm_unpacklo_ps(_F[0], _F[2]);
      __m128 L13 = _mm_unpacklo_ps(_F[1], _F[3]);
      __m128 H02 = _mm_unpackhi_ps(_F[0], _F[2]);
      __m128 H13 = _mm_unpackhi_ps(_F[1], _F[3]);
      _T1[i] = _mm_unpacklo_ps(L02, L13);
      _T2[i] = _mm_unpackhi_ps(L02, L13);
      _T3[i] = _mm_unpacklo_ps(H02, H13);
      _T4[i] = _mm_unpackhi_ps(H02, H13);
      _F += 4;
  }
}

#else // ifdef __SSE2__

inline void mult(float* to, float* from, float w, int nb) {for(int i = 0; i < nb; i++) to[i] = from[i] * w;}
inline void add_mult(float* to, float* from, float w, int nb) {for(int i = 0; i < nb; i++) to[i] += from[i] * w;}
inline void multiplex(float* T, float* F1, float* F2, float* F3, float* F4, int nb) { 
  for(int i = 0; i < nb; i++) {T[4*i] = F1[i]; T[4*i+1] = F2[i]; T[4*i+2] = F3[i]; T[4*i+3] = F4[i];}
}

inline void demultiplex(float* T1, float* T2, float* T3, float* T4, float* F, int nb) {
  for(int i = 0; i < nb; i++) {T1[i] = F[4*i]; T2[i] = F[4*i+1]; T3[i] = F[4*i+2]; T4[i] = F[4*i+3];}
}




#endif // ifdef __SSE2__
#endif // SSE_H
