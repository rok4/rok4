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
 * \file TiffReader.cpp
 ** \~french
 * \brief Implémentation de la classe TiffReader
 * \details Image physique, attaché à un fichier. Utilise la librairie libtiff.
 ** \~english
 * \brief Implement class TiffReader
 * \details Physical image, linked to a file. Use libtiff library.
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "TiffReader.h"

TiffReader::TiffReader ( const char* filename ) {
    input = TIFFOpen ( filename, "r" );
    if ( !input ) {
        std::cerr << "Unable to open file" << std::endl;
        exit ( 2 );
    }

    TIFFGetField ( input, TIFFTAG_IMAGEWIDTH, &width );
    TIFFGetField ( input, TIFFTAG_IMAGELENGTH, &height );
    TIFFGetField ( input, TIFFTAG_PHOTOMETRIC, &photometric );

    TIFFGetField ( input, TIFFTAG_BITSPERSAMPLE, &bitspersample );
    TIFFGetFieldDefaulted ( input, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel );

    pixelSize = ( bitspersample * sampleperpixel ) / 8;

    if ( TIFFIsTiled ( input ) ) {
        TIFFGetField ( input, TIFFTAG_TILEWIDTH, &tileWidth );
        TIFFGetField ( input, TIFFTAG_TILELENGTH, &tileHeight );
        BufferSize = 2*height/tileHeight;
        LineBuffer = new uint8_t[pixelSize*height];
    } else {
        BufferSize = 512;
        tileWidth = tileHeight = 0;
        LineBuffer = 0;
    }

    _Buffer = new uint8_t*[BufferSize];
    memset ( _Buffer, 0, BufferSize*sizeof ( uint8_t* ) );
    Buffer = new uint8_t*[height];
    memset ( Buffer, 0, height*sizeof ( uint8_t* ) );
    BIndex = new int[BufferSize];
    memset ( BIndex, -1, BufferSize*sizeof ( int ) );
    Buffer_pos = 0;
}

uint8_t* TiffReader::getRawLine ( uint32_t line ) {
    if ( !Buffer[line] ) {
        if ( !_Buffer[Buffer_pos] ) _Buffer[Buffer_pos] = new uint8_t[width*pixelSize];
        if ( BIndex[Buffer_pos] != -1 ) Buffer[BIndex[Buffer_pos]] = 0;
        BIndex[Buffer_pos] = line;
        Buffer[line] = _Buffer[Buffer_pos];
        if ( TIFFReadScanline ( input, Buffer[line], line ) == -1 ) {
            std::cerr << "Unable to read tiff line" << std::endl;
            exit ( 2 );
        }
        Buffer_pos = ( Buffer_pos + 1 ) % BufferSize;
    }
    return Buffer[line];
}

uint8_t* TiffReader::getRawTile ( uint32_t tile ) {
    if ( !Buffer[tile] ) {
        uint32_t tileSize = tileWidth*tileHeight*pixelSize;
        if ( !_Buffer[Buffer_pos] ) _Buffer[Buffer_pos] = new uint8_t[tileSize];
        if ( BIndex[Buffer_pos] != -1 ) Buffer[BIndex[Buffer_pos]] = 0;
        BIndex[Buffer_pos] = tile;
        Buffer[tile] = _Buffer[Buffer_pos];
        if ( TIFFReadEncodedTile ( input, tile, Buffer[tile], tileSize ) == -1 ) {
            std::cerr << "Unable to read tiff tile" << std::endl;
            exit ( 2 );
        }
        Buffer_pos = ( Buffer_pos + 1 ) % BufferSize;
    }
    return Buffer[tile];
}

uint8_t* TiffReader::getEncodedTile ( uint32_t tile ) {
    if ( !Buffer[tile] ) {
        uint32_t tileSize = TIFFTileSize ( input );
        if ( !_Buffer[Buffer_pos] ) _Buffer[Buffer_pos] = new uint8_t[tileSize];
        if ( BIndex[Buffer_pos] != -1 ) Buffer[BIndex[Buffer_pos]] = 0;
        BIndex[Buffer_pos] = tile;
        Buffer[tile] = _Buffer[Buffer_pos];
        if ( TIFFReadRawTile ( input, tile, Buffer[tile], tileSize ) == -1 ) {
            std::cerr << "Unable to read tiff tile" << std::endl;
            exit ( 2 );
        }
        Buffer_pos = ( Buffer_pos + 1 ) % BufferSize;
    }
    return Buffer[tile];
}

uint8_t* TiffReader::getLine ( uint32_t line, uint32_t offset, uint32_t size ) {
    if ( offset > height ) offset = height;
    if ( size > height - offset ) size = height - offset;
    if ( tileWidth ) { // tiled tiff
        int xmin = offset / tileWidth;
        int xmax = ( offset + size - 1 ) / tileWidth;
        int n = ( line / tileHeight ) * ( ( width + tileWidth - 1 ) / tileWidth ) + xmin; // tile number.
        int tileLineSize = tileHeight*pixelSize;
        int tileOffset = ( line % tileHeight ) * tileLineSize;
        for ( int x = xmin; x <= xmax; x++ ) {
            uint8_t *tile = getRawTile ( n++ );
            memcpy ( LineBuffer + tileLineSize*x, tile + tileOffset, tileLineSize );
        }
        return LineBuffer + offset*pixelSize;
    } else { // scanline tiff
        return getRawLine ( line ) + offset*pixelSize;
    }
}

int TiffReader::getWindow ( int offsetx, int offsety, int width, int height, uint8_t* buffer ) {
    for ( int y = 0; y < height; y++ ) {
        uint8_t* data = getLine ( offsety + y, offsetx, width );
        if ( !data ) return -1;
        memcpy ( buffer + y*width*pixelSize, data, width*pixelSize );
    }
    return 1;
}

void TiffReader::close() {
    TIFFClose ( input );
    for ( int i = 0; i < BufferSize; i++ ) if ( _Buffer[i] ) delete[] _Buffer[i];
    if ( LineBuffer ) delete[] LineBuffer;
    delete[] _Buffer;
    delete[] Buffer;
    delete[] BIndex;
}

