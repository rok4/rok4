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
 * \file MergeImage.cpp
 ** \~french
 * \brief Implémentation des classes MergeImage, MergeImageFactory et Pixel
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeImageFactory : usine de création d'objet MergeImage
 * \li Pixel
 ** \~english
 * \brief Implement classes MergeImage, MergeImageFactory and ExtendedCompoundImageFactory
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeImageFactory : factory to create MergeImage object
 * \li Pixel
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

template <typename T>
int MergeImage::_getline(T* buffer, int line)
{/*
    for (int i = 0; i < width*channels; i++) {
        buffer[i]=(T)bgValue[i%channels];
    }

    for (int i = 0; i < images.size(); i++) {
        
        T* buffer_t = new T[images[i]->width*images[i]->channels];
        images[i]->getline(buffer_t,images[i]->y2l(y));

        if (getMask(i) == NULL) {
            memcpy(&buffer[c0*channels], &buffer_t[c2*channels], (c1-c0)*channels*sizeof(T));
        } else {

            uint8_t* buffer_m = new uint8_t[getMask(i)->width];
            getMask(i)->getline(buffer_m,getMask(i)->y2l(y));

            for (int j=0; j < c1-c0; j++) {
                if (buffer_m[c2+j]) {
                    if (c2+j >= images[i]->width) {
                        // On dépasse la largeur de l'image courante (arrondis). On passe.
                        // Une sortie pour vérifier si ce cas se représente malgré les corrections
                        LOGGER_ERROR("Dépassement : demande la colonne "<<c2+j+1<<" sur "<<images[i]->width);
                        continue;
                    }
                    memcpy(&buffer[(c0+j)*channels],&buffer_t[(c2+j)*channels],sizeof(T)*channels);
                }
            }

            delete [] buffer_m;
        }
        delete [] buffer_t;
    }*/
    return width*channels*sizeof(T);
}

void MergeImage::mergeline ( uint8_t* buffer, uint8_t* back, uint8_t* front ) {/*
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
            
        }*/
}
    
int MergeImage::getline ( float* buffer, int line ) {/*
    uint8_t* backBuffer = new uint8_t[backImage->width* backImage->channels];
    uint8_t* frontBuffer = new uint8_t[frontImage->width* frontImage->channels];
    uint8_t* intBuffer = new uint8_t[width * channels];
    frontImage->getline ( frontBuffer, line );
    backImage->getline ( backBuffer, line );

    mergeline ( intBuffer,backBuffer,frontBuffer );
    convert ( buffer, intBuffer, width*channels );
    delete[] backBuffer;
    delete[] frontBuffer;
    delete[] intBuffer;*/
    return width*channels;
}

int MergeImage::getline ( uint8_t* buffer, int line ) {/*
    uint8_t* backBuffer = new uint8_t[backImage->width * backImage->channels];
    uint8_t* frontBuffer = new uint8_t[frontImage->width * frontImage->channels];

    frontImage->getline ( frontBuffer, line );
    backImage->getline ( backBuffer, line );

    mergeline ( buffer,backBuffer,frontBuffer );
    delete[] backBuffer;
    delete[] frontBuffer;*/
    return width*channels;
}

MergeImage* MergeImageFactory::createMergeImage( std::vector< Image* >& images, SampleType ST,
                                                 int* bgValue, int* transparentValue,
                                                 Merge::MergeType composition )
{
    if (images.size() == 0) {
        LOGGER_ERROR("No source images to defined merged image");
        return NULL;
    }

    int width = images.at(0)->width;
    int height = images.at(0)->height;
    int channels = images.at(0)->channels;

    for (int i = 1; i < images.size(); i++) {
        if (images.at(i)->width != width || images.at(i)->height != height) {
            LOGGER_ERROR("All images must have same dimensions");
            images.at(0)->print();
            images.at(i)->print();
            return NULL;
        }
        if (images.at(i)->channels > channels) { channels = images.at(i)->channels; }
    }

    return new MergeImage(images, channels, ST, bgValue, transparentValue, composition);
}


int MergeMask::_getline(uint8_t* buffer, int line) {

    memset(buffer,0,width);

    for (uint i = 0; i < MI->getImages()->size(); i++) {

        if (MI->getMask(i) == NULL) {
            /* L'image n'a pas de masque, on la considère comme pleine. Ca ne sert à rien d'aller voir plus loin,
             * cette ligne du masque est déjà pleine */
            memset(&buffer, 255, width);
            return width;
        } else {
            // Récupération du masque de l'image courante de l'MI.
            uint8_t* buffer_m = new uint8_t[width];
            MI->getMask(i)->getline(buffer_m,line);
            // On ajoute au masque actuel (on écrase si la valeur est différente de 0)
            for (int j = 0; j < width; j++) {
                if (buffer_m[j]) {
                    memcpy(&buffer[j],&buffer_m[j],1);
                }
            }
            delete [] buffer_m;
        }
    }

    return width;
}

/* Implementation de getline pour les uint8_t */
int MergeMask::getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

/* Implementation de getline pour les float */
int MergeMask::getline(float* buffer, int line) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    getline(buffer_t,line);
    convert(buffer,buffer_t,width*channels);
    delete [] buffer_t;
    return width*channels;
}
