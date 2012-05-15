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

#ifndef _TIFFLZWENCODER_
#define _TIFFLZWENCODER_

#include "Data.h"
#include "Image.h"
#include "lzwEncoder.h"
#include "TiffHeader.h"

#include <iostream>
#include <string.h> // Pour memcpy
#include <algorithm>

template <typename T>
class TiffLZWEncoder : public TiffEncoder {
protected:
    Image *image;
    int line;   // Ligne courante

    size_t rawBufferSize;
    T* rawBuffer;
    size_t lzwBufferSize;
    size_t lzwBufferPos;
    uint8_t* lzwBuffer;

public:
    TiffLZWEncoder(Image *image): image(image), line(-1), rawBufferSize(0), lzwBufferSize(0),lzwBufferPos(0) , lzwBuffer(NULL), rawBuffer(NULL){}
    ~TiffLZWEncoder(){if (lzwBuffer) delete[] lzwBuffer; delete image;}
    size_t read(uint8_t *buffer, size_t size){
    size_t offset = 0, header_size=TiffHeader::headerSize, linesize=image->width*image->channels, dataToCopy=0;
    if (!lzwBuffer) {
        rawBuffer = new T[image->height*image->width*image->channels];
        int lRead = 0;
        for (; lRead < image->height ; lRead++) {
            image->getline(rawBuffer + rawBufferSize, lRead);
            rawBufferSize += linesize;
        }
        rawBufferSize *= sizeof(T);
        lzwEncoder encoder;
        lzwBuffer = encoder.encode((uint8_t*)rawBuffer,rawBufferSize, lzwBufferSize);
        delete[] rawBuffer;
        rawBuffer = NULL;
    }

    if (line == -1) { // écrire le header tiff
        // Si pas assez de place pour le header, ne rien écrire.
        if (size < header_size) return 0;

        // Ceci est du tiff avec une seule strip.
        if (image->channels==1)
            if ( sizeof(T) == sizeof(float)) {
                memcpy(buffer, TiffHeader::TIFF_HEADER_LZW_FLOAT32_GRAY, header_size);
            }else {
                memcpy(buffer, TiffHeader::TIFF_HEADER_LZW_INT8_GRAY, header_size);
            }
        else if (image->channels==3)
            memcpy(buffer, TiffHeader::TIFF_HEADER_LZW_INT8_RGB, header_size);
        else if (image->channels==4)
            memcpy(buffer, TiffHeader::TIFF_HEADER_LZW_INT8_RGBA, header_size);
        *((uint32_t*)(buffer+18))  = image->width;
        *((uint32_t*)(buffer+30))  = image->height;
        *((uint32_t*)(buffer+102)) = image->height;
        *((uint32_t*)(buffer+114)) = lzwBufferSize;
        offset = header_size;
        line = 0;
    }

    if (size - offset > 0 ) { // il reste de la place
        if (lzwBufferPos <= lzwBufferSize) { // il reste de la donnée
            dataToCopy = std::min(size-offset, lzwBufferSize -lzwBufferPos);
            memcpy(buffer+offset,lzwBuffer+lzwBufferPos,dataToCopy);
            lzwBufferPos+=dataToCopy;
            offset+=dataToCopy;
        }
    }

    return offset;
}
    bool eof(){return (lzwBufferPos>=lzwBufferSize);}
    };

#endif


