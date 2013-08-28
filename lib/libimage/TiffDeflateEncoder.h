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

#ifndef _TIFFDEFLATEENCODER_
#define _TIFFDEFLATEENCODER_

#include "Data.h"
#include "Image.h"
#include "TiffHeader.h"
#include <zlib.h>
#include <iostream>
#include <string.h> // Pour memcpy
#include <algorithm>

template <typename T>
class TiffDeflateEncoder : public TiffEncoder {
protected:
    Image *image;
    int line;   // Ligne courante

    T* linebuffer;

    size_t deflateBufferSize;
    size_t deflateBufferPos;
    uint8_t* deflateBuffer;

    z_stream zstream;
    bool encode() {
        int rawLine = 0;
        int error = 0;
        zstream.zalloc = Z_NULL;
        zstream.zfree = Z_NULL;
        zstream.opaque = Z_NULL;
        zstream.data_type = Z_BINARY;
        deflateInit ( &zstream, 6 ); // taux de compression zlib
        zstream.avail_in = 0;
        zstream.next_out  = deflateBuffer;
        zstream.avail_out = deflateBufferSize;

        while ( rawLine >= 0 && rawLine < image->getHeight() && zstream.avail_out > 0 ) { // compresser les données dans des chunck idat
            if ( zstream.avail_in == 0 ) {                                    // si plus de donnée en entrée de la zlib, on lit une nouvelle ligne
                image->getline ( linebuffer, rawLine++ );
                zstream.next_in  = ( uint8_t* ) ( linebuffer );
                zstream.avail_in = image->getWidth() * image->channels * sizeof ( T );
            }
            error = deflate ( &zstream, Z_NO_FLUSH );
            switch (error){
                case Z_OK : 
                    break;
                case Z_MEM_ERROR :
                    LOGGER_DEBUG("MEM_ERROR");
                    deflateEnd ( &zstream );
                    return false;              // return 0 en cas d'erreur.
                case Z_STREAM_ERROR :
                    LOGGER_DEBUG("STREAM_ERROR");
                    deflateEnd ( &zstream );
                    return false;              // return 0 en cas d'erreur.
                case Z_VERSION_ERROR :
                    LOGGER_DEBUG("VERSION_ERROR");
                    deflateEnd ( &zstream );
                    return false;              // return 0 en cas d'erreur.
                default :
                    LOGGER_DEBUG("OTHER_ERROR");
                    deflateEnd ( &zstream );
                    return false;              // return 0 en cas d'erreur.
            }
//             if ( error != Z_OK ) {
//                 deflateReset ( &zstream );
//                 return false;              // return 0 en cas d'erreur.
//             }
        }

        if ( rawLine == image->getHeight() && zstream.avail_out > 6 ) { // plus d'entrée : il faut finaliser la compression
            int r = deflate ( &zstream, Z_FINISH );
            if ( r == Z_STREAM_END ) rawLine++;                   // on indique que l'on a compressé fini en passant rawLine ) height+1
            else if ( r != Z_OK ) {
                deflateEnd ( &zstream );
                return false;                      // une erreur
            }
        }

        if ( deflateEnd ( &zstream ) != Z_OK ) return false;

        uint32_t length = zstream.total_out;   // taille des données écritres
        deflateBufferSize = length;
        return true;
    }

public:
    TiffDeflateEncoder ( Image *image ) : image ( image ), line ( -1 ), deflateBufferSize ( 0 ),deflateBufferPos ( 0 ) , deflateBuffer ( NULL ) {
//         zstream.zalloc = Z_NULL;
//         zstream.zfree = Z_NULL;
//         zstream.opaque = Z_NULL;
//         zstream.data_type = Z_BINARY;
//         deflateInit ( &zstream, 6 ); // taux de compression zlib
//         zstream.avail_in = 0;
        linebuffer = new T[image->getWidth() * image->channels];
    }
    ~TiffDeflateEncoder() {
        if ( linebuffer ) delete[] linebuffer;
        if ( deflateBuffer ) delete[] deflateBuffer;
//         deflateEnd ( &zstream );

        delete image;
    }
    size_t read ( uint8_t *buffer, size_t size ) {
        size_t offset = 0, header_size=TiffHeader::headerSize ( image->channels ), linesize=image->getWidth()*image->channels, dataToCopy=0;
        if ( !deflateBuffer ) {
            deflateBufferSize = linesize * image->getHeight() * 2 ;
            deflateBuffer = new uint8_t[deflateBufferSize];
            while ( !encode() ) {
                deflateBufferSize *= 2;
                delete[] deflateBuffer;
                deflateBuffer = new uint8_t[deflateBufferSize];
            }

        }

        if ( line == -1 ) {
            // Si pas assez de place pour le header, ne rien écrire.
            if ( size < header_size ) return 0;

            // Ceci est du tiff avec une seule strip.
            if ( image->channels==1 )
                if ( sizeof ( T ) == sizeof ( float ) ) {
                    memcpy ( buffer, TiffHeader::TIFF_HEADER_ZIP_FLOAT32_GRAY, header_size );
                } else {
                    memcpy ( buffer, TiffHeader::TIFF_HEADER_ZIP_INT8_GRAY, header_size );
                }
            else if ( image->channels==3 )
                memcpy ( buffer, TiffHeader::TIFF_HEADER_ZIP_INT8_RGB, header_size );
            else if ( image->channels==4 )
                memcpy ( buffer, TiffHeader::TIFF_HEADER_ZIP_INT8_RGBA, header_size );
            * ( ( uint32_t* ) ( buffer+18 ) )  = image->getWidth();
            * ( ( uint32_t* ) ( buffer+30 ) )  = image->getHeight();
            * ( ( uint32_t* ) ( buffer+102 ) ) = image->getHeight();
            * ( ( uint32_t* ) ( buffer+114 ) ) = deflateBufferSize;
            offset = header_size;
            line = 0;
        }

        if ( size - offset > 0 ) { // il reste de la place
            if ( deflateBufferPos <= deflateBufferSize ) { // il reste de la donnée
                dataToCopy = std::min ( size-offset, deflateBufferSize -deflateBufferPos );
                memcpy ( buffer+offset,deflateBuffer+deflateBufferPos,dataToCopy );
                deflateBufferPos+=dataToCopy;
                offset+=dataToCopy;
            }
        }

        return offset;
    }

    bool eof() {
        return ( deflateBufferPos>=deflateBufferSize );
    }
};

#endif


