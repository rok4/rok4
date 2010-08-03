#ifndef CONVERT_H
#define CONVERT_H

#include <xmmintrin.h>
#include <stdint.h>
#include <cstring> // pour memcpy
#include <cmath>

/**
 * Converti et copie count éléments du tableau from
 * vers le tableau to.
 */
/*
template<typename T, typename F>
void convert(T* to, const F* from, size_t count)
{
  for(size_t i = 0; i < count; i++) to[i] = (T) from[i];
}
*/


/**
 * Conversion qui n'est qu'une copie.
 */
inline void convert(uint8_t* to, const uint8_t* from, size_t length) {
  memcpy(to, from, length);
}



/**
 * Convertir des int8 en float
 */
inline void convert(float* to, const uint8_t* from, size_t count) {
/*
  // On avance sur les premiers éléments jusqu'à alignement de to sur 128bits
  while( (intptr_t)to & 0x0f && count--) *to++ = (float) *from++;

  // On traite les éléments 4 par 4 en utlisant les fonctions intrinsics SSE
  __m128* T = (__m128*) to;
  int* F = (int*) from;
  for(int i = count/4; i--;) *T++ = _mm_cvtpu8_ps(_m_from_int(*F++));

  // On a utilisé des instructions MMX il faut réinitialiser les registres pour éviter des plantages sur les prochaines instructions FPU
  _mm_empty();

  // On traite les derniers éléments si count n'est pas un multuple de 4.
  while(count-- & 0x03) to[count] = (float) from[count];
*/
/*
 * Si rien ne va plus, on peut toujours faire simplement ceci:*/
  for (size_t i=0; i<count; ++i){
	  to[i]=from[i];
  }

}




#define _mm_cvtps_pu8(a) _mm_packs_pu16(_mm_cvtps_pi16(a), _mm_setzero_si64())

/**
 * Convertir de float à int8 en seuillant les valeur <0 et >255
 */
inline void convert(uint8_t* to, const float* from, size_t count) {

  //_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);

  // On avance sur les premiers éléments jusqu'à alignement de to sur 128bits
  while( (intptr_t)from & 0x0f && count--)
    *to++ = (uint8_t) _m_to_int(_mm_cvtps_pu8(_mm_load_ss(from++)));

  // On traite les éléments 4 par 4 en utlisant les fonctions intrinsics SSE
  int* T = (int*) to;
  __m128* F = (__m128*) from;
  for(int i = count/4; i--;) *T++ = _m_to_int(_mm_cvtps_pu8(*F++));

  // On traite les derniers éléments si count n'est pas un multuple de 4.
  while(count-- & 0x03) to[count] = (uint8_t) _m_to_int(_mm_cvtps_pu8(_mm_load_ss(from + count)));

  // On a utilisé des instructions MMX il faut réinitialiser les registres pour éviter des plantages sur les prochaines instructions FPU
  _mm_empty();
}









#endif
