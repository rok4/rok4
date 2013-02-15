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
 * \brief Implémentation des classes MergeImage, MergeImageFactory et MergeMask
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeImageFactory : usine de création d'objet MergeImage
 * \li MergeMask : masque fusionné, associé à une image fusionnée
 ** \~english
 * \brief Implement classes MergeImage, MergeImageFactory and MergeMask
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeImageFactory : factory to create MergeImage object
 * \li MergeMask : merged mask, associated with a merged image
 *
 * \todo Travailler sur un nombre de canaux variable (pour l'instant, systématiquement 4).
 */

#include "MergeImage.h"
#include "Pixel.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>

template<typename T>
static inline void composeTransparency( T* dest, int outChannels, Pixel<T>* back, Pixel<T>* front)
{
    front->alphaBlending(back);
    front->write(dest, outChannels);
}

template<typename T>
static inline void composeMask( T* dest, int outChannels, uint8_t* mask, Pixel<T>* back, Pixel<T>* front)
{
    if (*mask) {
        front->write(dest, outChannels);
    }
}

template<typename T>
static inline void composeMultiply( T* dest, int outChannels, Pixel<T>* back, Pixel<T>* front)
{
    front->multiply(back);
    front->write(dest, outChannels);
}

template<typename T>
static inline void composeNormal( T* dest, int outChannels, Pixel<T>* back, Pixel<T>* front)
{
/*
    switch ( outChannels ) {
        case 4:
            *(dest+3) = (uint8_t) ( 255 * (1.0 - ( 1.0 - front.Sa ) * ( 1.0 - back.Sa ) ) );
            
        case 3:
            *(dest+2) = (uint8_t) ( ( (1.0-front.Sa) * back.Sba +  front.Sba ) );
            *(dest+1) = (uint8_t) ( ( (1.0-front.Sa) * back.Sga + front.Sga ) );
        case 1: 
            
            *dest = (uint8_t) ( ( (1.0-front.Sa) * back.Sra + front.Sra ) );
    }*/
}

template <typename T>
int MergeImage::_getline(T* buffer, int line)
{
    T workLine[width*workSpp];

    T bg[channels];
    for (int i = 0; i < channels; i++) {
        bg[i] = (T) bgValue[i];
    }
    Pixel<T> *bgPix = new Pixel<T>(bg, channels);

    T transparent[4];
    for (int i = 0; i < 4; i++) {
        transparent[i] = (T) transparentValue[i];
    }
    Pixel<T> *transparentPix = new Pixel<T>(transparent, 4);

    for (int i = 0; i < width; i++) {
        // Format de travail sur 4 canaux tout le temps
        bgPix->write(workLine + i*workSpp,workSpp);
    }

    for (int i = images.size()-1; i >= 0; i--) {
        //LOGGER_INFO("Image source " << i);
        
        T* imageLine = new T[width*images[i]->channels];
        images[i]->getline(imageLine,line);

        uint8_t* maskLine;
        if (composition == Merge::MASK) {
            maskLine = new uint8_t[width];
            if (images[i]->getMask() == NULL) {
                memset(maskLine, 255, width);
            } else {
                images[i]->getMask()->getline(maskLine,line);
            }
        }

        Pixel<T> *backPix, *frontPix;
        int srcSpp = images[i]->channels;

        for (int p = 0; p < width; p++) {
            
            frontPix = new Pixel<T>(imageLine + p*srcSpp, srcSpp);
            backPix = new Pixel<T>(workLine + p*workSpp, workSpp);

            switch ( composition ) {
                case Merge::NORMAL:
                    composeNormal(workLine + p*workSpp, workSpp, backPix, frontPix );
                    break;
                case Merge::MASK:
                    composeMask(workLine + p*workSpp, workSpp, maskLine+p, backPix, frontPix );
                    break;
                case Merge::MULTIPLY:
                    composeMultiply(workLine + p*workSpp, workSpp, backPix, frontPix );
                    break;
                case Merge::TRANSPARENCY:
                    frontPix->isItTransparent(transparentPix);
                    composeTransparency(workLine + p*workSpp, workSpp, backPix, frontPix );
                    break;
                case Merge::LIGHTEN:
                case Merge::DARKEN:
                default:
                    composeNormal(workLine + p*workSpp, workSpp, backPix, frontPix );
                    break;
            }
        }

        delete [] imageLine;
        delete frontPix;
        delete backPix;
    }

    // On repasse la ligne sur le nombre de canaux voulu
    Pixel<T> *workPix;
    for (int p = 0; p < width; p++) {
        workPix = new Pixel<T>(workLine + p*workSpp, workSpp);
        workPix->write(buffer + p*channels, channels);
    }
    
    delete workPix;
    delete bgPix;
    delete transparentPix;
    
    return width*channels*sizeof(T);
}

/* Implementation de getline pour les uint8_t */
int MergeImage::getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

/* Implementation de getline pour les float */
int MergeImage::getline(float* buffer, int line)
{
    if (ST.getSampleFormat() == 1) { //SAMPLEFORMAT_UINT
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint8_t* buffer_t = new uint8_t[width*channels];
        getline(buffer_t,line);
        convert(buffer,buffer_t,width*channels);
        delete [] buffer_t;
        return width*channels;
    } else { //float
        return _getline(buffer, line);
    }
}

MergeImage* MergeImageFactory::createMergeImage( std::vector< Image* >& images, SampleType ST, int channels,
                                                 int* bgValue, int* transparentValue,
                                                 Merge::MergeType composition )
{
    if (images.size() == 0) {
        LOGGER_ERROR("No source images to defined merged image");
        return NULL;
    }

    int width = images.at(0)->width;
    int height = images.at(0)->height;

    // On travaille pour l'instant toujours sur 4 canaux RGBA
    int workSpp = 4;

    for (int i = 1; i < images.size(); i++) {
        if (images.at(i)->width != width || images.at(i)->height != height) {
            LOGGER_ERROR("All images must have same dimensions");
            images.at(0)->print();
            images.at(i)->print();
            return NULL;
        }
    }

    if (ST.isFloat() && ! (composition == Merge::MASK || composition == Merge::NORMAL)) {
        LOGGER_ERROR("Merge method is not consistent with the sample type. For float sample : MASK or NORMAL");
        return NULL;
    }

    return new MergeImage(images, channels, workSpp, ST, bgValue, transparentValue, composition);
}


int MergeMask::_getline(uint8_t* buffer, int line) {

    memset(buffer,0,width);

    for (uint i = 0; i < MI->getImages()->size(); i++) {

        if (MI->getMask(i) == NULL) {
            /* L'image n'a pas de masque, on la considère comme pleine. Ca ne sert à rien d'aller voir plus loin,
             * cette ligne du masque est déjà pleine */
            memset(buffer, 255, width);
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
