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

#ifndef _TIFFPACKBITSENCODER_
#define _TIFFPACKBITSENCODER_

#include "Data.h"
#include "Image.h"
#include "TiffHeader.h"
#include "pkbEncoder.h"

#include <cstring>
#include <cstdlib>

template <typename T>
class TiffPackBitsEncoder : public TiffEncoder {

    enum compression_state { BASE,
                             LITERAL,
                             RUN/*,
                            LITERAL_RUN*/
                           };

protected:
    Image *image;
    int line;   // Ligne courante
    T* rawBuffer;
    size_t rawBufferSize;

    size_t pkbBufferSize;
    size_t pkbBufferPos;
    uint8_t* pkbBuffer;

public:
    TiffPackBitsEncoder ( Image *image ) : image ( image ), line ( -1 ),rawBufferSize ( 0 ) , pkbBufferPos ( 0 ), pkbBufferSize ( 0 ), pkbBuffer ( NULL ), rawBuffer ( NULL ) {

    }
    ~TiffPackBitsEncoder() {
        delete image;
        if ( rawBuffer )
            delete[] rawBuffer;
        if ( pkbBuffer )
            delete[] pkbBuffer;
    }

    virtual size_t read ( uint8_t *buffer, size_t size ) {
        size_t offset = 0, header_size=TiffHeader::headerSize ( image->channels ), linesize=image->getWidth()*image->channels, dataToCopy=0;

        if ( !pkbBuffer ) {
            pkbBuffer = new uint8_t[linesize* image->getHeight() * sizeof ( T ) *2];
            pkbBufferSize = 0;
            rawBuffer = new T[linesize];
            rawBufferSize = linesize * sizeof ( T );
            int lRead = 0;
            pkbEncoder encoder;
            uint8_t * pkbLine;
            for ( ; lRead < image->getHeight() ; lRead++ ) {
                image->getline ( rawBuffer, lRead );
                size_t pkbLineSize = 0;
                pkbLine =  encoder.encode ( ( uint8_t* ) rawBuffer,rawBufferSize, pkbLineSize );
                memcpy ( pkbBuffer+pkbBufferSize,pkbLine,pkbLineSize );
                pkbBufferSize += pkbLineSize;
                delete[] pkbLine;
            }
            delete[] rawBuffer;
            rawBuffer = NULL;
        }


        if ( line == -1 ) { // écrire le header tiff
            if ( image->channels==1 )
                if ( sizeof ( T ) == sizeof ( float ) ) {
                    memcpy ( buffer, TiffHeader::TIFF_HEADER_PKB_FLOAT32_GRAY, header_size );
                } else {
                    memcpy ( buffer, TiffHeader::TIFF_HEADER_PKB_INT8_GRAY, header_size );
                }
            else if ( image->channels==3 )
                memcpy ( buffer, TiffHeader::TIFF_HEADER_PKB_INT8_RGB, header_size );
            else if ( image->channels==4 )
                memcpy ( buffer, TiffHeader::TIFF_HEADER_PKB_INT8_RGBA, header_size );
            * ( ( uint32_t* ) ( buffer+18 ) )  = image->getWidth();
            * ( ( uint32_t* ) ( buffer+30 ) )  = image->getHeight();
            * ( ( uint32_t* ) ( buffer+102 ) ) = image->getHeight();
            * ( ( uint32_t* ) ( buffer+114 ) ) = pkbBufferSize ;
            offset = header_size;
            line = 0;
        }

        if ( size - offset > 0 ) { // il reste de la place
            if ( pkbBufferPos <= pkbBufferSize ) { // il reste de la donnée
                dataToCopy = std::min ( size-offset, pkbBufferSize - pkbBufferPos );
                memcpy ( buffer+offset, pkbBuffer+pkbBufferPos, dataToCopy );
                pkbBufferPos+=dataToCopy;
                offset+=dataToCopy;
            }
        }

        return offset;
    }
    virtual bool eof() {
        return ( pkbBufferPos>=pkbBufferSize );
    }



};

#endif



