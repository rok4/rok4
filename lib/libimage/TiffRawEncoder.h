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

#ifndef _TIFFRAWENCODER_
#define _TIFFRAWENCODER_

#include "Data.h"
#include "Image.h"
#include "TiffHeader.h"

#include <cstring>

template <typename T>
class TiffRawEncoder : public TiffEncoder {
protected:
    Image *image;
    int line;   // Ligne courante

public:
    TiffRawEncoder(Image *image) : image(image), line(-1) {}
    ~TiffRawEncoder() {
        delete image;
    }
    virtual size_t read(uint8_t *buffer, size_t size) {
        size_t offset = 0, header_size=TiffHeader::headerSize, linesize=image->width*image->channels;
        if (line == -1) { // écrire le header tiff
            if (image->channels==1)
                if ( sizeof(T) == sizeof(float)) {
                    memcpy(buffer, TiffHeader::TIFF_HEADER_RAW_FLOAT32_GRAY, header_size);
                } else {
                    memcpy(buffer, TiffHeader::TIFF_HEADER_RAW_INT8_GRAY, header_size);
                }
            else if (image->channels==3)
                memcpy(buffer, TiffHeader::TIFF_HEADER_RAW_INT8_RGB, header_size);
            else if (image->channels==4)
                memcpy(buffer, TiffHeader::TIFF_HEADER_RAW_INT8_RGBA, header_size);
            *((uint32_t*)(buffer+18))  = image->width;
            *((uint32_t*)(buffer+30))  = image->height;
            *((uint32_t*)(buffer+102)) = image->height;
            *((uint32_t*)(buffer+114)) = image->height*linesize * sizeof(T) ;
            offset = header_size;
            line = 0;
        }
        linesize *= sizeof(T);
        for (; line < image->height && offset + linesize <= size; line++) {
            image->getline((T*)(buffer + offset), line);
            offset += linesize;
        }

        return offset;
    }
    virtual bool eof() {
        return (line>=image->height);
    }
};

#endif


