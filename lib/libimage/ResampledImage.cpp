/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file ResampledImage.cpp
 ** \~french
 * \brief Implémentation de la classe ResampledImage, permettant le réechantillonnage d'image
 ** \~english
 * \brief Implement classe ResampledImage, allowing image resampling
 */

#include "ResampledImage.h"
#include "Logger.h"
#include "Utils.h"
#include <tiff.h>
#include <cmath>
#include <cstring>

ResampledImage::ResampledImage ( Image* image, int width, int height,
                                 double resx, double resy, BoundingBox< double > bbox,
                                 Interpolation::KernelType KT, bool bMask ) :

    Image ( width, height, image->getChannels(), resx, resy, bbox ), sourceImage ( image ), K ( Kernel::getInstance ( KT ) ), useMask ( bMask ) {

    double resX_src = image->getResX();
    double resY_src = image->getResY();

    ratioX = resx / resX_src;
    ratioY = resy / resY_src;
    
    setCRS(image->getCRS());

    // Pour considérer les valeurs comme celles aux centres des pixels, on ramène les coordonnées au centre
    left = ( ( bbox.xmin + 0.5*resx ) - ( image->getBbox().xmin + 0.5*resX_src ) ) / resX_src;
    top = ( ( image->getBbox().ymax - 0.5*resY_src ) - ( bbox.ymax - 0.5*resy ) ) / resY_src;

    // On calcule le nombre de pixels sources à considérer dans l'interpolation, dans le sens des x et des y
    Kx = ceil ( 2 * K.size ( ratioX )-1E-7 );
    Ky = ceil ( 2 * K.size ( ratioY )-1E-7 );

    if ( ! sourceImage->getMask() ) useMask = false;

    /* On veut mémoriser un certain nombre de lignes pour ne pas refaire un travail déjà fait.
     * On va travailler les lignes 4 par 4 (pour l'utilisation des instructions SSE). On va donc mémoriser
     * un multiple de 4 lignes.
     * Une ligne va intervenir au maximum dans l'interpolation de Ky lignes (diamètre du noyau d'interpolation)
     * Conclusion : on mémorise "Ky arrondi au multiple de 4 supérieur" lignes
     */
    memorizedLines = 4* ( ( Ky+3 ) /4 );

    /* -------------------- PLACE MEMOIRE ------------------- */

    // nombre d'éléments d'une ligne de l'image source arrondie au multiple de 4 supérieur.
    int srcImgSize = 4* ( ( sourceImage->getWidth() *channels + 3 ) /4 );
    int srcMskSize = 4* ( ( sourceImage->getWidth() + 3 ) /4 );

    // nombre d'éléments d'une ligne de l'image calculée arrondie au multiple de 4 supérieur.
    int outImgSize = 4* ( ( width*channels + 3 ) /4 );
    int outMskSize = 4* ( ( width + 3 ) /4 );

    // nombre de poids dans Wx
    int xWeightSize = 4*width*Kx;
    int xMinSize = 4* ( ( width+3 ) /4 );

    int sz = 8 * srcImgSize * sizeof ( float ) // src_image_buffer + mux_src_image_buffer;
             // resampled_line ("memorize_line" lignes) + mux_resampled_line + dst_image_buffer
             + outImgSize * ( memorizedLines + 4 + 1 ) * sizeof ( float )
             + xWeightSize * sizeof ( float )              // place pour Wx
             + xMinSize * sizeof ( int );               // place pour le tableau xmin

    if ( useMask ) {
        sz += 8 * srcMskSize * sizeof ( float )     // src_mask_buffer + mux_src_mask_buffer;
              // resampled_mask ("memorize_line" lignes) + mux_resampled_mask + weight_buffer
              + outMskSize * ( memorizedLines + 4 + 1 ) * sizeof ( float );
    }


    /* Allocation de la mémoire pour tous les buffers en une seule fois :
     *  - gain de temps (l'allocation est une action qui prend du temps)
     *  - tous les buffers sont côtes à côtes dans la mémoire, gain de temps lors des lectures/écritures
     */
    __buffer = ( float* ) _mm_malloc ( sz, 16 ); // Allocation allignée sur 16 octets pour SSE
    memset ( __buffer, 0, sz );

    float* B = ( float* ) __buffer;

    /* -------------------- PARTIE IMAGE -------------------- */

    // Lignes source d'image
    for ( int i = 0; i < 4; i++ ) {
        src_image_buffer[i] = B;
        B += srcImgSize;
    }
    mux_src_image_buffer = B;
    B += 4*srcImgSize;

    // Ligne d'image rééchantillonnée
    resampled_image = new float*[memorizedLines];
    resampled_line_index = new int[memorizedLines];

    mux_resampled_image = B;
    B += 4*outImgSize;
    for ( int i = 0; i < memorizedLines; i++ ) {
        resampled_image[i] = B;
        B += outImgSize;
        resampled_line_index[i] = -1;
    }
    dst_image_buffer = B;
    B += outImgSize;

    /* -------------------- PARTIE MASQUE ------------------- */

    if ( useMask ) {
        // Lignes source de masque
        for ( int i = 0; i < 4; i++ ) {
            src_mask_buffer[i] = B;
            B += srcMskSize;
        }
        mux_src_mask_buffer = B;
        B += 4*srcMskSize;

        weight_buffer = B;
        B += outMskSize;

        // Ligne de masque rééchantillonnée
        resampled_mask = new float*[memorizedLines];
        mux_resampled_mask = B;
        B += 4*outMskSize;

        for ( int i = 0; i < memorizedLines; i++ ) {
            resampled_mask[i] = B;
            B += outMskSize;
        }
    }

    /* -------------------- PARTIE POIDS -------------------- */

    Wx = B;
    B += xWeightSize;
    xMin = ( int* ) B;
    B += xMinSize;

    memset ( Wx, 0, xWeightSize * sizeof ( float ) );
    float* W = Wx;
    for ( int x = 0; x < width; x++ ) {
        int lg = Kx;
        xMin[x] = K.weight ( W, lg, left + x * ratioX, sourceImage->getWidth() );
        // On copie chaque poids en 4 exemplaires.
        for ( int i = lg-1; i >= 0; i-- ) for ( int j = 0; j < 4; j++ ) W[4*i + j] = W[i];
        W += 4*Kx;
    }
}

int ResampledImage::resampleSourceLine ( int line ) {
    /* Vu que l'on calcule les lignes 4 par 4 et qu'on les mémorise, on a potentiellement déjà en mémoire la ligne
     * demandée. On vérifie dans le tableau des index si c'est le cas.
     */
    if ( resampled_line_index[line % memorizedLines] == line ) {
        return ( line % memorizedLines );
    }

    /* On va réechantillonner 4 lignes d'un coup. On commence par charger les 4 lignes de l'image source concernées
     * On vérifie bien que les 4 lignes existent bel et bien (qu'on dépasse pas la hauteur de l'image source)
     */
    for ( int i = 0; i < 4; i++ ) {
        if ( 4* ( line/4 ) + i < sourceImage->getHeight() ) {
            sourceImage->getline ( src_image_buffer[i], 4* ( line/4 ) + i );
            if ( useMask ) {
                sourceImage->getMask()->getline ( src_mask_buffer[i], 4* ( line/4 ) + i );
            }
        }
    }

    /* Afin d'utiliser au mieux les instructions SSE, on "multiplexe" les 4 lignes, c'est-à dire que :
     *          - La colonne des 4 pixels sont à la suite dans le tableau
     *          - Les différents canaux ne sont plus entrelacés
     *
     *    ligne 1      R1 G1 B1         R1 G1 B1        ...
     *    ligne 2      R2 G2 B2         R2 G2 B2        ...
     *    ligne 3      R3 G3 B3         R3 G3 B3        ...
     *    ligne 4      R4 G4 B4         R4 G4 B4        ...
     *                  | ---->
     *       on "lit" colonne par colonne
     *    Multiplexé = R1 R2 R3 R4 G1 G2 G3 G4 B1 B2 B3 B4 R1 R2 R3 R4 G1 G2 G3 G4 B1 B2 B3 B4 ...
     */
    multiplex ( mux_src_image_buffer,
                src_image_buffer[0], src_image_buffer[1], src_image_buffer[2], src_image_buffer[3],
                sourceImage->getWidth() *sourceImage->getChannels() );


    if ( useMask ) {
        multiplex ( mux_src_mask_buffer,
                    src_mask_buffer[0], src_mask_buffer[1], src_mask_buffer[2], src_mask_buffer[3],
                    sourceImage->getWidth() );
    }

    for ( int x = 0; x < width; x++ ) {
        if ( useMask ) {
            dot_prod ( channels, Kx,
                       mux_resampled_image + 4*x*channels,
                       mux_resampled_mask + 4*x,
                       mux_src_image_buffer + 4*xMin[x]*channels,
                       mux_src_mask_buffer + 4*xMin[x],
                       Wx + 4*Kx*x );
        } else {
            dot_prod ( channels, Kx,
                       mux_resampled_image + 4*x*channels,
                       mux_src_image_buffer + 4*xMin[x]*channels,
                       Wx + 4*Kx*x );
        }
    }

    demultiplex ( resampled_image[ ( 4* ( line/4 ) ) % memorizedLines],
                  resampled_image[ ( 4* ( line/4 ) +1 ) % memorizedLines],
                  resampled_image[ ( 4* ( line/4 ) +2 ) % memorizedLines],
                  resampled_image[ ( 4* ( line/4 ) +3 ) % memorizedLines],
                  mux_resampled_image, width*channels );


    if ( useMask ) {
        demultiplex ( resampled_mask[ ( 4* ( line/4 ) ) % memorizedLines],
                      resampled_mask[ ( 4* ( line/4 ) +1 ) % memorizedLines],
                      resampled_mask[ ( 4* ( line/4 ) +2 ) % memorizedLines],
                      resampled_mask[ ( 4* ( line/4 ) +3 ) % memorizedLines],
                      mux_resampled_mask, width );
    }

    // Mise à jour des index des lignes mémorisées
    for ( int i = 0; i < 4; i++ ) {
        resampled_line_index[ ( 4* ( line/4 ) +i ) % memorizedLines] = 4* ( line/4 ) +i;
    }

    return ( line % memorizedLines );
}

int ResampledImage::getline ( float* buffer, int line ) {

    float weights[Ky];

    // On calcule les coefficient d'interpolation
    int ymin = K.weight ( weights, Ky, top + line * ratioY, sourceImage->getHeight() );

    int index = resampleSourceLine ( ymin );
    if ( useMask ) {
        mult ( buffer, weight_buffer, resampled_image[index], resampled_mask[index], weights[0], width, channels );
    } else {
        mult ( buffer, resampled_image[index], weights[0], width*channels );
    }

    for ( int y = 1; y < Ky; y++ ) {
        index = resampleSourceLine ( ymin+y );
        if ( useMask ) {
            add_mult ( buffer, weight_buffer, resampled_image[index], resampled_mask[index], weights[y], width, channels );
        } else {
            add_mult ( buffer, resampled_image[index], weights[y], width*channels );
        }
    }

    if ( useMask ) {
        normalize ( buffer, weight_buffer, width, channels );
    }

    return width*channels;
}

int ResampledImage::getline ( uint8_t* buffer, int line ) {
    int nb = getline ( dst_image_buffer, line );
    convert ( buffer, dst_image_buffer, nb );
    return nb;
}

int ResampledImage::getline ( uint16_t* buffer, int line ) {
    int nb = getline ( dst_image_buffer, line );
    convert ( buffer, dst_image_buffer, nb );
    return nb;
}
