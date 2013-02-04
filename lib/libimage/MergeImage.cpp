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

#include "MergeImage.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>

static inline float Multiply(float Sca, float Sa, float Dca, float Da) {
    return(Sca*Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
}

static inline void composeMultiply( uint8_t * dest,int outchannels, Pixel back, Pixel front) {
    //BUG
    //return(Sca*Dca+Sca*(1.0-Da)+Dca*(1.0-Sa));
    float gamma = back.Sa + front.Sa - back.Sa * front.Sa;
    //gamma = back.a * front.a;
    //gamma = gamma;
    gamma = ( gamma < 0.0 ? 0.0 : gamma );
    gamma = ( gamma > 1.0 ? 1.0 : gamma );
    switch ( outchannels ) {
        case 4:
//             *(dest+3) = (uint8_t) (255 * (1.0-gamma));
            *(dest+3) = (uint8_t) (255 * (1.0-back.Sa * front.Sa));
        case 3:
//             *(dest+2) = (uint8_t) ( 255 *255* gamma * Multiply(back.Sba/255 , back.Sa, front.Sba/255, front.Sa));
//             *(dest+1) = (uint8_t) ( 255 *255* gamma * Multiply(back.Sga/255 , back.Sa, front.Sga/255, front.Sa));
            *(dest+2) = (uint8_t) ( back.Sba * front.Sba/255 );
            *(dest+1) = (uint8_t) ( back.Sga * front.Sga/255 );
        case 1: 
//             *dest = (uint8_t) ( 255 *255* gamma * Multiply(back.Sra/255 , back.Sa, front.Sra/255, front.Sa));
            *dest     = (uint8_t) ( back.Sra * front.Sra/255 );
    }
}

static inline void composeNormal( uint8_t * dest,int outchannels, Pixel back, Pixel front) {
    //float gamma = back.a * front.a;
    //gamma = back.a * front.a;
    //gamma = gamma;
    //gamma = ( gamma < 0.0 ? 0.0 : gamma );
    //gamma = ( gamma > 1.0 ? 1.0 : gamma );
    switch ( outchannels ) {
        case 4:
            *(dest+3) = (uint8_t) ( 255 * (1.0 - ( 1.0 - front.Sa ) * ( 1.0 - back.Sa ) ) );
            
        case 3:
            *(dest+2) = (uint8_t) ( ( (1.0-front.Sa) * back.Sba +  front.Sba ) );
            *(dest+1) = (uint8_t) ( ( (1.0-front.Sa) * back.Sga + front.Sga ) );
        case 1: 
            
            *dest = (uint8_t) ( ( (1.0-front.Sa) * back.Sra + front.Sra ) );
    }
}


void MergeImage::mergeline ( uint8_t* buffer, uint8_t* back, uint8_t* front ) {
    size_t column = 0;
    Pixel *backPix, *frontPix;
    
    while ( column < width ) {
            switch (backImage->channels){
                case 1:
                    backPix = new Pixel(*(back+column));
                    break;
                case 3:
                    backPix = new Pixel(*(back+column*3),*(back+1+column*3),*(back+2+column*3));
                    break;
                case 4:
                    backPix = new Pixel(*(back+column*4),*(back+1+column*4),*(back+2+column*4),*(back+3+column*4));
                    break;
            }
            switch (frontImage->channels){
                case 1:
                    frontPix = new Pixel(*(front+column));
                    break;
                case 3:
                    frontPix = new Pixel(*(front+column*3),*(front+1+column*3),*(front+2+column*3));
                    break;
                case 4:
                    frontPix = new Pixel(*(front+column*4),*(front+1+column*4),*(front+2+column*4),*(front+3+column*4));
                    break;
        }
        switch ( composition ) {

        case NORMAL:
            composeNormal ( ( buffer + column * channels ), channels, *backPix, *frontPix );
            break;
        case MULTIPLY:
            composeMultiply ( ( buffer + column * channels ), channels, *backPix, *frontPix );
            break;
        case LIGHTEN:
        case DARKEN:
        default:
            composeNormal ( ( buffer + column * channels ), channels, *backPix, *frontPix );
            break;
        }

        delete frontPix;
        delete backPix;
        column++;
            
        }
}
    
//    case MULTIPLYOLD:
//         if ( backImage->channels == frontImage->channels ) {
//             while ( column < width * channels ) {
//                 * ( buffer+column ) = * ( back + column ) * * ( front + column ) * factor / 255;
//                 column++;
//             }
//             break;
//         }
//         switch ( backImage->channels ) {
//         case 1: {
//             switch ( frontImage->channels ) {
//             case 3:
//                 while ( column < width * channels ) {
//                     * ( buffer+column ) = * ( back + ( column - ( column % frontImage->channels ) ) /frontImage->channels ) * * ( front + column ) * factor / 255;
//                     column++;
//                 }
//                 break;
//             case 4:
//                 while ( column < width * channels ) {
//                     if ( column%4 == 3 ) {
//                         * ( buffer+column ) = 0;
//                     } else {
//                         * ( buffer+column ) = * ( back + ( column - ( column % frontImage->channels ) ) /frontImage->channels ) * * ( front + column ) * factor / 255;
//                     }
//                     column++;
//                 }
//                 break;
//             }
//         }
//         break;
//         case 3: {
//             switch ( frontImage->channels ) {
//             case 1:
//                 while ( column < width * channels ) {
//                     * ( buffer+column ) =( * ( back + column ) * * ( front + ( column - ( column % backImage->channels ) ) /backImage->channels ) * factor )/ 255;
//                     column++;
//                 }
//                 break;
//             case 4:
//                 while ( column < width * channels ) {
//                     if ( column%4 == 3 ) {
//                         * ( buffer+column ) = 0;
//                     } else {
//                         * ( buffer+column ) = * ( back + ( column - ( column % frontImage->channels ) ) /frontImage->channels ) * * ( front + column ) * factor / 255;
//                     }
//                     column++;
//                 }
//                 break;
//             }
//         }
//         break;
//         case 4: {
//             switch ( frontImage->channels ) {
//             case 1:
//                 while ( column < width * channels ) {
//                     if ( column%4 == 3 ) {
//                         * ( buffer+column ) = ( * ( back + column ) ==255?* ( back + column ) :0 );
//                     } else {
//                         * ( buffer+column ) = * ( back + column ) * (* ( front + ( column - ( column % backImage->channels ) ) /backImage->channels ) * factor + 255*factor) / 255;
//                     }
//                     column++;
//                 }
//                 break;
//             case 3:
//                 while ( column < width * channels ) {
//                     if ( column%4 == 3 ) {
//                         * ( buffer+column ) = ( * ( back + column ) ==255?* ( back + column ) :0 );
//                     } else {
//                         * ( buffer+column ) = * ( back + column ) * * ( front + ( column - ( column % backImage->channels ) ) /backImage->channels ) * factor / 255;
//                     }
//                     column++;
//                 }
//                 break;
//             }
//         }


int MergeImage::getline ( float* buffer, int line ) {
    uint8_t* backBuffer = new uint8_t[backImage->width* backImage->channels];
    uint8_t* frontBuffer = new uint8_t[frontImage->width* frontImage->channels];
    uint8_t* intBuffer = new uint8_t[width * channels];
    frontImage->getline ( frontBuffer, line );
    backImage->getline ( backBuffer, line );

    mergeline ( intBuffer,backBuffer,frontBuffer );
    convert ( buffer, intBuffer, width*channels );
    delete[] backBuffer;
    delete[] frontBuffer;
    delete[] intBuffer;
    return width*channels;
}

int MergeImage::getline ( uint8_t* buffer, int line ) {
    uint8_t* backBuffer = new uint8_t[backImage->width * backImage->channels];
    uint8_t* frontBuffer = new uint8_t[frontImage->width * frontImage->channels];

    frontImage->getline ( frontBuffer, line );
    backImage->getline ( backBuffer, line );

    mergeline ( buffer,backBuffer,frontBuffer );
    delete[] backBuffer;
    delete[] frontBuffer;
    return width*channels;
}

MergeImage::MergeImage ( Image* backImage, Image* frontImage, MergeImage::MergeType composition, float factor ) :
    Image ( backImage->width, backImage->height, ( backImage->channels>frontImage->channels?backImage->channels:frontImage->channels ), backImage->getbbox() ),
    backImage ( backImage ), frontImage ( frontImage ), composition ( composition ), factor ( factor ) {
    
    LOGGER_DEBUG("Out   channels = "  << channels );
    LOGGER_DEBUG("Back  channels = "  << backImage->channels );
    LOGGER_DEBUG("Front channels = "  << frontImage->channels );
}

MergeImage::~MergeImage() {
    delete backImage;
    delete frontImage;
}

