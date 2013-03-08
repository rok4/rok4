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
 * \brief Implémentation des classes MergeImage, MergeImageFactory et MergeMask et du namespace Merge
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeImageFactory : usine de création d'objet MergeImage
 * \li MergeMask : masque fusionné, associé à une image fusionnée
 * \li Merge : énumère et manipule les différentes méthodes de fusion
 ** \~english
 * \brief Implement classes MergeImage, MergeImageFactory and MergeMask and the namespace Merge
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeImageFactory : factory to create MergeImage object
 * \li MergeMask : merged mask, associated with a merged image
 * \li Merge : enumerate and managed different merge methods
 *
 * \todo Travailler sur un nombre de canaux variable (pour l'instant, systématiquement 4).
 */

#include "MergeImage.h"
#include "Pixel.h"
#include "Line.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>

template <typename T>
int MergeImage::_getline(T* buffer, int line)
{
    Line<T> aboveLine(width);
    T* imageLine = new T[width*4];
    uint8_t* maskLine = new uint8_t[width];
    memset(maskLine, 0, width);
        
    T bg[channels*width];
    for (int i = 0; i < channels*width; i++) {
        bg[i] = (T) bgValue[i%channels];
    }
    Line<T> workLine(bg, maskLine, channels, width);

    T* transparent;
    if (transparentValue != NULL) {
        transparent = new T[3];
        for (int i = 0; i < 3; i++) {
            transparent[i] = (T) transparentValue[i];
        }
    }

    for (int i = images.size()-1; i >= 0; i--) {

        int srcSpp = images[i]->channels;
        images[i]->getline(imageLine,line);

        if (images[i]->getMask() == NULL) {
            memset(maskLine, 255, width);
        } else {
            images[i]->getMask()->getline(maskLine,line);
        }

        if (transparentValue == NULL) {
            aboveLine.store(imageLine, maskLine, srcSpp);
        } else {
            aboveLine.store(imageLine, maskLine, srcSpp, transparent);
        }

        switch ( composition ) {
            case Merge::NORMAL:
                workLine.alphaBlending(&aboveLine);
                break;
            case Merge::MASK:
                workLine.useMask(&aboveLine);
                break;
            case Merge::MULTIPLY:
                workLine.multiply(&aboveLine);
                break;
            case Merge::TRANSPARENCY:
                workLine.alphaBlending(&aboveLine);
                break;
            case Merge::LIGHTEN:
            case Merge::DARKEN:
            default:
                workLine.alphaBlending(&aboveLine);
                break;
        }

    }

    // On repasse la ligne sur le nombre de canaux voulu
    workLine.write(buffer, channels);

    if (transparentValue != NULL) { delete [] transparent; }
    delete [] imageLine;
    delete [] maskLine;

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

namespace Merge {

    const char *mergeType_name[] = {
        "UNKNOWN",
        "NORMAL",
        "LIGHTEN",
        "DARKEN",
        "MULTIPLY",
        "TRANSPARENCY",
        "MASK"
    };

    MergeType fromString ( std::string strMergeMethod ) {
        int i;
        for ( i = mergeType_size; i ; --i ) {
            if ( strMergeMethod.compare ( mergeType_name[i] ) == 0 )
                break;
        }
        return static_cast<MergeType> ( i );
    }

    std::string toString ( MergeType mergeMethod ) {
        return std::string ( mergeType_name[mergeMethod] );
    }
}
