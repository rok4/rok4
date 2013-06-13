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
 * \file ReprojectedImage.cpp
 ** \~french
 * \brief Implémentation de la classe ReprojectedImage, permettant la reprojection d'image
 ** \~english
 * \brief Implement classe ReprojectedImage, allowing image reprojecting
 */

#include "ReprojectedImage.h"

#include <string>
#include "Image.h"
#include "Grid.h"
#include "Logger.h"
#include "Kernel.h"

#include "Utils.h"
#include <cmath>

void ReprojectedImage::initialize () {

    ratioX = ( grid->bbox.xmax - grid->bbox.xmin ) / double ( width );
    ratioY = ( grid->bbox.ymax - grid->bbox.ymin ) / double ( height );

    // On calcule le nombre de pixels sources à considérer dans l'interpolation, dans le sens des x et des y
    Kx = ceil ( 2 * K.size ( ratioX ) );
    Ky = ceil ( 2 * K.size ( ratioY ) );

    if ( ! sourceImage->getMask() ) {
        useMask = false;
    }

    memorizedLines = 2*Ky + ceil ( grid->getDeltaY() );

    /* -------------------- PLACE MEMOIRE ------------------- */

    // nombre d'éléments d'une ligne de l'image source, arrondi au multiple de 4 supérieur.
    int srcImgSize = 4* ( ( sourceImage->getWidth() *channels + 3 ) /4 );
    int srcMskSize = 4* ( ( sourceImage->getWidth() + 3 ) /4 );

    // nombre d'éléments d'une ligne de l'image calculée, arrondi au multiple de 4 supérieur.
    int outImgSize = 4* ( ( width*channels + 3 ) /4 );
    int outMskSize = 4* ( ( width + 3 ) /4 );

    // nombre d'éléments d'une ligne de la grille, arrondi au multiple de 4 supérieur.
    int gridSize = 4* ( ( width*channels + 3 ) /4 );

    // nombre d'élément dans les tableaux de poids, arrondi au multiple de 4 supérieur.
    int kxSize = 4* ( ( Kx+3 ) /4 );
    int kySize = 4* ( ( Ky+3 ) /4 );

    int globalSize = srcImgSize * memorizedLines * sizeof ( float ) // place pour "memorizedLines" lignes d'image source
                     + outImgSize * 8 * sizeof ( float ) // 4 lignes reprojetées, en multiplexées et en séparées => 8
                     + gridSize * 8 * sizeof ( float ) // 4 lignes de la grille, X et Y => 8

                     /*   1024 possibilités de poids
                      * + poids pour 4 lignes, multiplexés
                      * + extrait des 4 lignes sources, sur lesquelles appliqué les poids
                      */
                     + kxSize * ( 1028 + 4*channels ) * sizeof ( float )
                     + kySize * ( 1028 + 4*channels ) * sizeof ( float );

    if ( useMask ) {
        globalSize += srcMskSize * memorizedLines * sizeof ( float ) // place pour charger "memorizedLines" lignes du masque source
                      + outMskSize * 8 * sizeof ( float ) // 4 lignes reprojetées, en multiplexées et en séparées => 8
                      + kxSize * 4 * sizeof ( float )
                      + kySize * 4 * sizeof ( float );
    }

    /* Allocation de la mémoire pour tous les buffers en une seule fois :
     *  - gain de temps (l'allocation est une action qui prend du temps)
     *  - tous les buffers sont côtes à côtes dans la mémoire, gain de temps lors des lectures/écritures
     */
    __buffer = ( float* ) _mm_malloc ( globalSize, 16 ); // Allocation allignée sur 16 octets pour SSE
    memset ( __buffer, 0, globalSize );

    float* B = __buffer;

    /* -------------------- PARTIE IMAGE -------------------- */

    src_image_buffer = new float*[memorizedLines];
    src_line_index = new int[memorizedLines];

    for ( int i = 0; i < memorizedLines; i++ ) {
        src_image_buffer[i] = B;
        src_line_index[i] = -1;
        B += srcImgSize;
    }

    for ( int i = 0; i < 4; i++ ) {
        dst_image_buffer[i] = B;
        B += outImgSize;
    }

    dst_line_index = -1;

    mux_dst_image_buffer = B;
    B += 4*outImgSize;

    tmp1Img = B;
    B += 4*channels*kxSize;
    tmp2Img = B;
    B += 4*channels*kySize;

    /* -------------------- PARTIE MASQUE ------------------- */

    if ( useMask ) {
        src_mask_buffer = new float*[memorizedLines];
        for ( int i = 0; i < memorizedLines; i++ ) {
            src_mask_buffer[i] = B;
            B += srcMskSize;
        }

        for ( int i = 0; i < 4; i++ ) {
            dst_mask_buffer[i] = B;
            B += outMskSize;
        }

        mux_dst_mask_buffer = B;
        B += 4*outMskSize;

        tmp1Msk = B;
        B += 4*kxSize;
        tmp2Msk = B;
        B += 4*kySize;
    }

    /* -------------------- PARTIE POIDS -------------------- */

    for ( int i = 0; i < 4; i++ ) {
        X[i] = B;
        B += gridSize;
        Y[i] = B;
        B += gridSize;
    }

    for ( int i = 0; i < 1024; i++ ) {
        Wx[i] = B;
        B += kxSize;
        Wy[i] = B;
        B += kySize;
    }
    WWx = B;
    B += 4*kxSize;
    WWy = B;
    B += 4*kySize;

    for ( int i = 0; i < 1024; i++ ) {
        int lgX = Kx;
        int lgY = Ky;
        xmin[i] = K.weight ( Wx[i], lgX, double ( i ) /1024. + Kx, sourceImage->getWidth() ) - Kx;
        ymin[i] = K.weight ( Wy[i], lgY, double ( i ) /1024. + Ky, sourceImage->getHeight() ) - Ky;
    }
}

int ReprojectedImage::getSourceLineIndex ( int line ) {

    if ( src_line_index[line % memorizedLines] == line ) {
        // On a déjà la ligne source en mémoire, on renvoie donc son index (place dans src_image_buffer)
        return ( line % memorizedLines );
    }

    // Récupération de la ligne voulue
    sourceImage->getline ( src_image_buffer[line % memorizedLines], line );

    if ( useMask ) {
        sourceImage->getMask()->getline ( src_mask_buffer[line % memorizedLines], line );
    }

    // Mis à jour de l'index
    src_line_index[line % memorizedLines] = line;

    return line % memorizedLines;
}

float* ReprojectedImage::computeDestLine ( int line ) {

    if ( line/4 == dst_line_index ) {
        return dst_image_buffer[line%4];
    }
    dst_line_index = line/4;

    for ( int i = 0; i < 4; i++ ) {
        if ( 4*dst_line_index+i < height ) {
            grid->getline ( 4*dst_line_index+i, X[i], Y[i] );
        } else {
            memcpy ( X[i], X[0], width*sizeof ( float ) );
            memcpy ( Y[i], Y[0], width*sizeof ( float ) );
        }
    }

    int Ix[4], Iy[4];

    for ( int x = 0; x < width; x++ ) {

        for ( int i = 0; i < 4; i++ ) {
            Ix[i] = ( X[i][x] - floor ( X[i][x] ) ) * 1024;
            Iy[i] = ( Y[i][x] - floor ( Y[i][x] ) ) * 1024;
        }

        multiplex ( WWx, Wx[Ix[0]], Wx[Ix[1]], Wx[Ix[2]], Wx[Ix[3]], Kx );
        multiplex ( WWy, Wy[Iy[0]], Wy[Iy[1]], Wy[Iy[2]], Wy[Iy[3]], Ky );

        int y0 = ( int ) ( Y[0][x] ) + ymin[Iy[0]];
        int y1 = ( int ) ( Y[1][x] ) + ymin[Iy[1]];
        int y2 = ( int ) ( Y[2][x] ) + ymin[Iy[2]];
        int y3 = ( int ) ( Y[3][x] ) + ymin[Iy[3]];
        int dx0 = ( ( int ) ( X[0][x] ) + xmin[Ix[0]] );
        int dx1 = ( ( int ) ( X[1][x] ) + xmin[Ix[1]] );
        int dx2 = ( ( int ) ( X[2][x] ) + xmin[Ix[2]] );
        int dx3 = ( ( int ) ( X[3][x] ) + xmin[Ix[3]] );

        for ( int j = 0; j < Ky; j++ ) {

            multiplex_unaligned ( tmp1Img,
                                  src_image_buffer[getSourceLineIndex ( y0 + j )] + dx0*channels,
                                  src_image_buffer[getSourceLineIndex ( y1 + j )] + dx1*channels,
                                  src_image_buffer[getSourceLineIndex ( y2 + j )] + dx2*channels,
                                  src_image_buffer[getSourceLineIndex ( y3 + j )] + dx3*channels,
                                  Kx * channels );

            if ( useMask ) {
                multiplex_unaligned ( tmp1Msk,
                                      src_mask_buffer[getSourceLineIndex ( y0 + j )] + dx0,
                                      src_mask_buffer[getSourceLineIndex ( y1 + j )] + dx1,
                                      src_mask_buffer[getSourceLineIndex ( y2 + j )] + dx2,
                                      src_mask_buffer[getSourceLineIndex ( y3 + j )] + dx3,
                                      Kx );

                dot_prod ( channels, Kx,
                           tmp2Img + 4*j*channels,
                           tmp2Msk + 4*j,
                           tmp1Img,
                           tmp1Msk,
                           WWx );
            } else {
                dot_prod ( channels, Kx,
                           tmp2Img + 4*j*channels,
                           tmp1Img,
                           WWx );
            }
        }

        if ( useMask ) {
            dot_prod ( channels, Ky,
                       mux_dst_image_buffer + 4*x*channels,
                       mux_dst_mask_buffer + 4*x,
                       tmp2Img,
                       tmp2Msk,
                       WWy );
        } else {
            dot_prod ( channels, Ky,
                       mux_dst_image_buffer + 4*x*channels,
                       tmp2Img,
                       WWy );
        }
    }

    demultiplex ( dst_image_buffer[0], dst_image_buffer[1], dst_image_buffer[2], dst_image_buffer[3],
                  mux_dst_image_buffer, width*channels );

    if ( useMask ) {
        demultiplex ( dst_mask_buffer[0], dst_mask_buffer[1], dst_mask_buffer[2], dst_mask_buffer[3],
                      mux_dst_mask_buffer, width );
    }

    return dst_image_buffer[line%4];
}

int ReprojectedImage::getline ( uint8_t* buffer, int line ) {
    const float* dst_line = computeDestLine ( line );
    convert ( buffer, dst_line, width*channels );
    return width*channels;
}

int ReprojectedImage::getline ( float* buffer, int line ) {
    const float* dst_line = computeDestLine ( line );
    convert ( buffer, dst_line, width*channels );
    return width*channels;
}




