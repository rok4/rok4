/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file Utils.h
 ** \~french
 * \brief Définition de fonctions de conversion et calculs sur des tableaux. Chaque fonctions est définie avec et sans instructions SSE2.
 * \details
 * \li Conversions disponibles
 * \image html conversions.png
 */

#ifndef UTILS_H
#define UTILS_H

#include <cstring>
#include <iostream>
#include <stdint.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif


/**
 * \brief Conversion qui n'est qu'une copie.
 * @param to     Tableau destination
 * @param from   Tableau source
 * @param length Nombre d'éléments à convertir
 */
template<typename T>
inline void convert ( T* to, const T* from, size_t length ) {
    memcpy ( to, from, length*sizeof ( T ) );
}


/**
 * \brief Conversion uint8 -> float
 * \warning Maximum value : 254
 * @param to Tableau de flottants de destination
 * @param from Tableau d'entiers 8 bits source
 * @param length Nombre d'éléments à convertir
 */
#ifdef __SSE2__
inline void convert ( float* to, const uint8_t* from, int length ) {
    while ( ( intptr_t ) to & 0x0f && length ) {
        --length;
        *to++ = ( float ) *from++;
    }
    while ( length & 0x0f ) {
        --length;    // On s'arrange pour avoir un multiple de 16 d'éléments à traiter.
        to[length] = ( float ) from[length];
    }
    length /= 16;

    // On traite les éléments 16 par 16 en utlisant les fonctions intrinsics SSE
    __m128i z = _mm_setzero_si128();
    __m128i* F = ( __m128i* ) from;

    if ( ( intptr_t ) from & 0x0f )
        for ( int i = 0; i < length; ++i ) { // cas from non aligné
            __m128i m = _mm_loadu_si128 ( F + i );

            __m128i L = _mm_unpacklo_epi8 ( m, z );
            __m128i H = _mm_unpackhi_epi8 ( m, z );

            __m128i L0 = _mm_unpacklo_epi8 ( L, z );
            __m128i L1 = _mm_unpackhi_epi8 ( L, z );
            __m128i H0 = _mm_unpacklo_epi8 ( H, z );
            __m128i H1 = _mm_unpackhi_epi8 ( H, z );

            _mm_store_ps ( to + 16*i,      _mm_cvtepi32_ps ( L0 ) );
            _mm_store_ps ( to + 16*i + 4,  _mm_cvtepi32_ps ( L1 ) );
            _mm_store_ps ( to + 16*i + 8,  _mm_cvtepi32_ps ( H0 ) );
            _mm_store_ps ( to + 16*i + 12, _mm_cvtepi32_ps ( H1 ) );
        }
    else for ( int i = 0; i < length; ++i ) { // cas from aligné
            __m128i m = _mm_load_si128 ( F + i );

            __m128i L = _mm_unpacklo_epi8 ( m, z );
            __m128i H = _mm_unpackhi_epi8 ( m, z );

            __m128i L0 = _mm_unpacklo_epi8 ( L, z );
            __m128i L1 = _mm_unpackhi_epi8 ( L, z );
            __m128i H0 = _mm_unpacklo_epi8 ( H, z );
            __m128i H1 = _mm_unpackhi_epi8 ( H, z );

            _mm_store_ps ( to + 16*i,      _mm_cvtepi32_ps ( L0 ) );
            _mm_store_ps ( to + 16*i + 4,  _mm_cvtepi32_ps ( L1 ) );
            _mm_store_ps ( to + 16*i + 8,  _mm_cvtepi32_ps ( H0 ) );
            _mm_store_ps ( to + 16*i + 12, _mm_cvtepi32_ps ( H1 ) );
        }

}

#else // Version non SSE 

inline void convert ( float* to, const uint8_t* from, int length ) {
    for ( int i = 0; i < length; ++i ) to[i] = ( float ) from[i];
}
#endif

/**
 * \brief Conversion uint16 -> float
 * @param to Tableau de flottants de destination
 * @param from Tableau d'entiers 16 bits source
 * @param length Nombre d'éléments à convertir
 */
#ifdef __SSE2__
inline void convert ( float* to, const uint16_t* from, int length ) {
    for ( int i = 0; i < length; ++i ) to[i] = ( float ) from[i];
}
#else // Version non SSE 
inline void convert ( float* to, const uint16_t* from, int length ) {
    for ( int i = 0; i < length; ++i ) to[i] = ( float ) from[i];
}
#endif

/**
 * \brief Conversion uint8 -> uint16
 * @param to Tableau d'entiers 16 bits destination
 * @param from   Tableau d'entiers 8 bits source
 * @param length Nombre d'éléments à convertir
 */
#ifdef __SSE2__
inline void convert ( uint16_t* to, const uint8_t* from, int length ) {
    for ( int i = 0; i < length; ++i ) to[i] = ( uint16_t ) from[i];
}
#else // Version non SSE 
inline void convert ( uint16_t* to, const uint8_t* from, int length ) {
    for ( int i = 0; i < length; ++i ) to[i] = ( uint16_t ) from[i];
}
#endif


/**
 * \brief Conversion float -> uint8
 * \details Les valeurs sont arrondies au plus proche avec saturation
 *
 * @param to Tableau d'entiers 8 bits destination
 * @param from Tableau de flottants de source
 * @param length Nombre d'éléments à convertir
 * 
 * \warning Tant que la version SSE2 ne sature pas lors de l'arrondi (5.999 donne 5 et pas 6), on l'inhibe (version non SSE2 dans tous les cas)
 */

#ifdef __SSE2__

inline void convert ( uint8_t* to, const float* from, int length ) {
    for ( int i = 0; i < length; i++ ) {
        int t = ( int ) ( from[i] + 0.5 );
        if ( t < 0 ) to[i] = 0;
        else if ( t > 255 ) to[i] = 255;
        else to[i] = t;
    }
}
/*
inline void convert ( uint8_t* to, const float* from, int length ) {
    // _MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
    //
    // On avance sur les premiers éléments jusqu'à alignement de to sur 128bits
    while ( ( intptr_t ) from & 0x0f && length ) {
        if ( *from > 255 ) *to = 255;
        else if ( *from < 0 ) *to = 0;
        else *to = ( uint8_t ) *from;
        ++from;
        ++to;
        --length;
    }
    while ( length & 0x0f )  { // On s'arrange pour avoir un multiple de 16 d'éléments à traiter.
        --length;
        if ( from[length] > 255 ) to[length] = 255;
        else if ( from[length] < 0 ) to[length] = 0;
        else to[length] = ( uint8_t ) from[length];
    }

    // On traite les éléments 16 par 16 en utlisant les fonctions intrinsics SSE
    __m128i* T = ( __m128i* ) to;
    length /= 16;

    if ( ( intptr_t ) to & 0x0f )
        for ( int i = 0; i < length; i++ ) { // Cas to non aligné
            __m128i m1 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i ) );
            __m128i m2 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i+4 ) );
            __m128i m3 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i+8 ) );
            __m128i m4 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i+12 ) );
            m1 = _mm_packs_epi32 ( m1, m2 );
            m3 = _mm_packs_epi32 ( m3, m4 );
            m1 = _mm_packus_epi16 ( m1, m3 );
            _mm_storeu_si128 ( T + i, m1 );
        }
    else
        for ( int i = 0; i < length; i++ ) { // cas to aligné
            __m128i m1 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i ) );
            __m128i m2 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i+4 ) );
            __m128i m3 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i+8 ) );
            __m128i m4 = _mm_cvtps_epi32 ( _mm_load_ps ( from + 16*i+12 ) );
            m1 = _mm_packs_epi32 ( m1, m2 );
            m3 = _mm_packs_epi32 ( m3, m4 );
            m1 = _mm_packus_epi16 ( m1, m3 );
            _mm_store_si128 ( T + i, m1 );
        }
}*/
#else // Version non SSE

inline void convert ( uint8_t* to, const float* from, int length ) {
    for ( int i = 0; i < length; i++ ) {
        int t = ( int ) ( from[i] + 0.5 );
        if ( t < 0 ) to[i] = 0;
        else if ( t > 255 ) to[i] = 255;
        else to[i] = t;
    }
}
#endif


/**
 * \brief Conversion float -> uint16
 * \details Les valeurs sont arrondies au plus proche avec saturation
 *
 * @param to   Tableau de flottants de source
 * @param from Tableau d'entiers 16 bits destination
 * @param length Nombre d'éléments à convertir
 */

#ifdef __SSE2__

inline void convert ( uint16_t* to, const float* from, int length ) {
    for ( int i = 0; i < length; i++ ) {
        int t = ( int ) ( from[i] + 0.5 );
        if ( t < 0 ) to[i] = 0;
        else if ( t > 65535 ) to[i] = 65535;
        else to[i] = t;
    }
}
#else // Version non SSE

inline void convert ( uint16_t* to, const float* from, int length ) {
    for ( int i = 0; i < length; i++ ) {
        int t = ( int ) ( from[i] + 0.5 );
        if ( t < 0 ) to[i] = 0;
        else if ( t > 65535 ) to[i] = 65535;
        else to[i] = t;
    }
}
#endif




/**
 * \brief Multiplie un tableau de float from par w et sauvegarde dans to
 * @param to   Tableau de flottants de source
 * @param from Tableau de flottants de destination
 * @param length Nombre d'éléments dans le tableau
 */

#ifdef __SSE2__

// Sans masque
inline void mult ( float* to, const float* from, const float w, int length ) {
    while ( ( intptr_t ) to & 0x0f && length ) {
        --length;    // On aligne to sur 128bits
        *to++ = w * *from++;
    }
    while ( length & 0x03 ) {
        --length;    // On s'arrange pour avoir un multiple de 4 d'éléments à traiter.
        to[length] = w * from[length];
    }

    const __m128 W = _mm_set1_ps ( w );
    length /= 4;

    if ( ( intptr_t ) from & 0x0f ) // cas from non aligné
        for ( int i = 0; i < length; ++i ) _mm_store_ps ( to + 4*i, _mm_mul_ps ( W, _mm_loadu_ps ( from + 4*i ) ) );
    else // cas from aligné
        for ( int i = 0; i < length; ++i ) _mm_store_ps ( to + 4*i, _mm_mul_ps ( W, _mm_load_ps ( from + 4*i ) ) );
}

// Avec masque
inline void mult ( float* outImg,float* weightSum, const float* inImg, const float* inMask,
                   const float weight, int width, int channels ) {
    for ( int w = 0; w < width; w++ ) {
        if ( inMask[w] ) {
            weightSum[w] = weight;
            for ( int c = 0; c < channels; c++ ) {
                outImg[w*channels + c] = inImg[w*channels + c] * weight;
            }
        } else {
            weightSum[w] = 0.;
            memset ( outImg + w*channels,0,channels*sizeof ( float ) );
        }
    }
}

#else // Version non SSE

// Sans masque
inline void mult ( float* to, const float* from, const float w, int length ) {
    for ( int i = 0; i < length; i++ ) to[i] = from[i] * w;
}

// Avec masque
inline void mult ( float* outImg,float* weightSum, const float* inImg, const float* inMask,
                   const float weight, int width, int channels ) {
    for ( int w = 0; w < width; w++ ) {
        if ( inMask[w] ) {
            weightSum[w] = weight;
            for ( int c = 0; c < channels; c++ ) {
                outImg[w*channels + c] = inImg[w*channels + c] * weight;
            }
        } else {
            weightSum[w] = 0.;
            memset ( outImg + w*channels,0,channels*sizeof ( float ) );
        }
    }
}

#endif


/**
 * \brief Ajoute au tableau to le produit des élements de from par w
 * @param to   Tableau de flottants de source
 * @param from Tableau de flottants de destination
 * @param length Nombre d'éléments dans le tableau
 */

#ifdef __SSE2__

// Sans masque
inline void add_mult ( float* to, const float* from, const float w, int length ) {
    while ( ( intptr_t ) to & 0x0f && length ) {
        --length;    // On aligne to sur 128bits
        *to++ += w * *from++;
    }
    while ( length & 0x03 ) {
        --length;    // On s'arrange pour avoir un multiple de 4 d'éléments à traiter.
        to[length] += w * from[length];
    }

    const __m128 W = _mm_set1_ps ( w );
    length /= 4;

    if ( ( intptr_t ) from & 0x0f ) // cas from non aligné
        for ( int i = 0; i < length; ++i ) {
            _mm_store_ps ( to + 4*i, _mm_add_ps ( _mm_load_ps ( to + 4*i ), _mm_mul_ps ( W, _mm_loadu_ps ( from + 4*i ) ) ) );
        }
    else // cas from aligné
        for ( int i = 0; i < length; ++i ) _mm_store_ps ( to + 4*i, _mm_add_ps ( _mm_load_ps ( to + 4*i ), _mm_mul_ps ( W, _mm_load_ps ( from + 4*i ) ) ) );
}

// Avec masque
inline void add_mult ( float* to,float* weightSum, const float* from, const float* fromMask,
                       const float weight, int width, int channels ) {
    int length = width*channels;
    while ( ( intptr_t ) to & 0x0f && length-- ) *to++ += ( float ) weight * *from++;

    for ( int w = 0; w < width; w++ ) {
        if ( ! fromMask[w] ) continue;

        weightSum[w] += weight;
        for ( int c = 0; c < channels; c++ ) {
            to[w*channels + c] += from[w*channels + c] * weight;
        }
    }
}

#else // Version non SSE

// Sans masque
inline void add_mult ( float* to, const float* from, const float w, int length ) {
    while ( ( intptr_t ) to & 0x0f && length-- ) *to++ += ( float ) w * *from++;

    for ( int i = 0; i < length; i++ ) to[i] += from[i] * w;
}

// Avec masque
inline void add_mult ( float* outImg,float* weightSum, const float* inImg, const float* inMask,
                       const float weight, int width, int channels ) {
    int length = width*channels;
    while ( ( intptr_t ) outImg & 0x0f && length-- ) *outImg++ += ( float ) weight * *inImg++;

    for ( int w = 0; w < width; w++ ) {
        if ( ! inMask[w] ) continue;

        weightSum[w] += weight;
        for ( int c = 0; c < channels; c++ ) {
            outImg[w*channels + c] += inImg[w*channels + c] * weight;
        }
    }
}
#endif

// Version non SSE

inline void normalize ( float* toNormalize, const float* coefficients, const float width, int channels ) {
    for ( int w = 0; w < width; w++ ) {
        if ( coefficients[w] == 0. ) {
            memset ( toNormalize + w*channels, 0, channels*sizeof ( float ) );
        } else {
            for ( int c = 0; c < channels; c++ ) {
                toNormalize[w*channels + c] /= coefficients[w];
            }
        }
    }
}



/**
 * \brief Mutiplexe 4 tableaux d'entrée en 1 tableau
 *
 * @param F1 Tableau source : A1 A2 A3 ...
 * @param F2 Tableau source : B1 B2 B3 ...
 * @param F3 Tableau source : C1 C2 C3 ...
 * @param F4 Tableau source : D1 D2 D3 ...
 * @param T  Tableau de sortie : A1 B1 C1 D1 A2 B2 C2 D2 A3 B3 C3 D3 ...
 * @param length taille des tableau F1, F2, F3, F4.
 */


inline void multiplex_unaligned ( float* T, const float* F1, const float* F2, const float* F3, const float* F4, int length ) {
    for ( int i = 0; i < length; i++ ) {
        T[4*i] = F1[i];
        T[4*i+1] = F2[i];
        T[4*i+2] = F3[i];
        T[4*i+3] = F4[i];
    }
}

#ifdef __SSE2__
inline void multiplex ( float* T, const float* F1, const float* F2, const float* F3, const float* F4, int length ) {
    while ( length & 0x03 ) { // On s'arrange pour avoir un multiple de 4 d'éléments à traiter.
        --length;
        T[4*length] = F1[length];
        T[4*length+1] = F2[length];
        T[4*length+2] = F3[length];
        T[4*length+3] = F4[length];
    }
    length /= 4;
    for ( int i = 0; i < length; i++ ) {
        __m128 f0 = _mm_load_ps ( F1 + 4*i );
        __m128 f1 = _mm_load_ps ( F2 + 4*i );
        __m128 f2 = _mm_load_ps ( F3 + 4*i );
        __m128 f3 = _mm_load_ps ( F4 + 4*i );

        __m128 L02 = _mm_unpacklo_ps ( f0, f2 );
        __m128 H02 = _mm_unpackhi_ps ( f0, f2 );
        __m128 L13 = _mm_unpacklo_ps ( f1, f3 );
        __m128 H13 = _mm_unpackhi_ps ( f1, f3 );

        _mm_store_ps ( T + 16*i,    _mm_unpacklo_ps ( L02, L13 ) );
        _mm_store_ps ( T + 16*i+4,  _mm_unpackhi_ps ( L02, L13 ) );
        _mm_store_ps ( T + 16*i+8,  _mm_unpacklo_ps ( H02, H13 ) );
        _mm_store_ps ( T + 16*i+12, _mm_unpackhi_ps ( H02, H13 ) );
    }
}

#else // Version non SSE 
inline void multiplex ( float* T, const float* F1, const float* F2, const float* F3, const float* F4, int length ) {
    return multiplex_unaligned ( T, F1, F2, F3, F4, length );
}
#endif


/**
 * \brief Démutiplexe 1 tableau en 4 tableaux de sortie
 *
 * @param F Tableau source : A1 B1 C1 D1 A2 B2 C2 D2 A3 B3 C3 D3 ...
 * @param T1 Tableau de sortie : A1 A2 A3 ...
 * @param T2 Tableau de sortie : B1 B2 B3 ...
 * @param T3 Tableau de sortie : C1 C2 C3 ...
 * @param T4 Tableau de sortie : D1 D2 D3 ...
 * @param length taille des tableau F1, F2, F3, F4.
 */

#ifdef __SSE2__
inline void demultiplex ( float* T1, float* T2, float* T3, float* T4, const float* F, int length ) {

    while ( length & 0x03 ) { // On s'arrange pour avoir un multiple de 4 d'éléments à traiter.
        --length;
        T1[length] = F[4*length];
        T2[length] = F[4*length+1];
        T3[length] = F[4*length+2];
        T4[length] = F[4*length+3];
    }

    length /= 4;
    for ( int i = 0; i < length; i++ ) {
        __m128 F0 = _mm_load_ps ( F + 16*i );
        __m128 F1 = _mm_load_ps ( F + 16*i+4 );
        __m128 F2 = _mm_load_ps ( F + 16*i+8 );
        __m128 F3 = _mm_load_ps ( F + 16*i+12 );

        __m128 L02 = _mm_unpacklo_ps ( F0, F2 );
        __m128 H02 = _mm_unpackhi_ps ( F0, F2 );
        __m128 L13 = _mm_unpacklo_ps ( F1, F3 );
        __m128 H13 = _mm_unpackhi_ps ( F1, F3 );

        _mm_store_ps ( T1 + 4*i, _mm_unpacklo_ps ( L02, L13 ) );
        _mm_store_ps ( T2 + 4*i, _mm_unpackhi_ps ( L02, L13 ) );
        _mm_store_ps ( T3 + 4*i, _mm_unpacklo_ps ( H02, H13 ) );
        _mm_store_ps ( T4 + 4*i, _mm_unpackhi_ps ( H02, H13 ) );
    }
}
#else // Version non SSE 
inline void demultiplex ( float* T1, float* T2, float* T3, float* T4, const float* F, int length ) {
    for ( int i = 0; i < length; i++ ) {
        T1[i] = F[4*i];
        T2[i] = F[4*i+1];
        T3[i] = F[4*i+2];
        T4[i] = F[4*i+3];
    }
}
#endif


#ifdef __SSE2__

// Sans masque
template<int C>
inline void dot_prod ( int K, float* to, const float* from, const float* W ) {
    __m128 w = _mm_load_ps ( W );
    __m128 T[C];
    for ( int c = 0; c < C; c++ ) T[c] = _mm_mul_ps ( w, _mm_load_ps ( from + 4*c ) );

    for ( int i = 1; i < K; i++ ) {
        w = _mm_load_ps ( W+4*i );
        for ( int c = 0; c < C; c++ ) T[c] = _mm_add_ps ( T[c], _mm_mul_ps ( w, _mm_load_ps ( from + 4*C*i + 4*c ) ) );
    }

    for ( int c = 0; c < C; c++ ) _mm_store_ps ( to + 4*c, T[c] );
}

// Avec masque
template<int C>
inline void dot_prod ( int K, float* outImg, float* outMask, const float* inImg, const float* inMask, const float* W ) {
    float T[4*C];
    float weightSum[4] = {0.,0.,0.,0.};
    memset ( T,0,4*C*sizeof ( float ) );

    for ( int i = 0; i < K; i++ ) {

        for ( int lig = 0; lig < 4; lig++ ) {
            if ( ! inMask[4*i + lig] ) continue;
            weightSum[lig] +=  W[4*i + lig];

            for ( int c = 0; c < C; c++ ) {
                T[lig + c*4] += W[4*i + lig] * inImg[4*C*i + lig + c*4];
            }
        }
    }

    for ( int lig = 0; lig < 4; lig++ ) {
        if ( weightSum[lig] == 0. ) {
            for ( int c = 0; c < C; c++ ) {
                outImg[lig + c*4] = 0.;
            }
            outMask[lig] = 0.;
        } else {
            for ( int c = 0; c < C; c++ ) {
                // Normalisation des valeurs
                outImg[lig + c*4] = T[lig + c*4]/weightSum[lig];
            }
            outMask[lig] = 255.;
        }
    }
}

#else // Version non SSE 

// Avec masque
template<int C>
inline void dot_prod ( int K, float* outImg, float* outMask, const float* inImg, const float* inMask, const float* W ) {

    float T[4*C];
    float weightSum[4] = {0.,0.,0.,0.};
    memset ( T,0,4*C*sizeof ( float ) );

    for ( int i = 0; i < K; i++ ) {

        for ( int lig = 0; lig < 4; lig++ ) {
            if ( ! inMask[4*i + lig] ) continue;
            weightSum[lig] +=  W[4*i + lig];

            for ( int c = 0; c < C; c++ ) {
                T[lig + c*4] += W[4*i + lig] * inImg[4*C*i + lig + c*4];
            }
        }
    }

    for ( int lig = 0; lig < 4; lig++ ) {
        if ( weightSum[lig] == 0. ) {
            for ( int c = 0; c < C; c++ ) {
                outImg[lig + c*4] = 0.;
            }
            outMask[lig] = 0.;
        } else {
            for ( int c = 0; c < C; c++ ) {
                // Normalisation des valeurs
                outImg[lig + c*4] = T[lig + c*4]/weightSum[lig];
            }
            outMask[lig] = 255.;
        }
    }
}

// Sans masque
template<int C>
inline void dot_prod ( int K, float* to, const float* from, const float* W ) {
    float T[4*C];

    for ( int c = 0; c < 4*C; c++ ) {
        T[c] = W[c%4] * from[c];
    }

    for ( int i = 1; i < K; i++ ) {
        for ( int c = 0; c < 4*C; c++ ) {
            T[c] += W[4*i+c%4] * from[4*C*i + c];
        }
    }
    for ( int c = 0; c < 4*C; c++ ) {
        to[c] = T[c];
    }
}
#endif

// Avec masque
inline void dot_prod ( int C, int K, float* to, float* toMask, const float* from, const float* mask, const float* W ) {
    switch ( C ) {
    case 1:
        dot_prod<1> ( K, to, toMask, from, mask, W );
        break;
    case 2:
        dot_prod<2> ( K, to, toMask, from, mask, W );
        break;
    case 3:
        dot_prod<3> ( K, to, toMask, from, mask, W );
        break;
    case 4:
        dot_prod<4> ( K, to, toMask, from, mask, W );
        break;
    }
}

// Sans masque
inline void dot_prod ( int C, int K, float* to, const float* from, const float* W ) {
    switch ( C ) {
    case 1:
        dot_prod<1> ( K, to, from, W );
        break;
    case 2:
        dot_prod<2> ( K, to, from, W );
        break;
    case 3:
        dot_prod<3> ( K, to, from, W );
        break;
    case 4:
        dot_prod<4> ( K, to, from, W );
        break;
    }
}

#endif


