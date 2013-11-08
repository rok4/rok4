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
 * \file TiledTiffReader.cpp
 ** \~french
 * \brief Implémentation de la classe TiledTiffReader
 * \details Image physique, attaché à un fichier. Utilise la librairie libtiff.
 ** \~english
 * \brief Implement class TiledTiffReader
 * \details Physical image, linked to a file. Use libtiff library.
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "TiledTiffReader.h"
#include "FileDataSource.h"
#include "Decoder.h"

static const uint8_t PNG_SIGNATURE[8] = {
    137, 80, 78, 71, 13, 10, 26, 10
}; 

TiledTiffReader::TiledTiffReader ( const char* name ) {

    filename = new char[512];
    strcpy ( filename,name );

    /* On commence par lire le fichier comme une image TIFF, pou récupérer les informations grâce aux tags */
    TIFF* tiffInput = TIFFOpen ( filename, "r" );
    if ( !tiffInput ) {
        LOGGER_ERROR ( "Unable to open TIFF image " << filename );
    }

    TIFFGetField ( tiffInput, TIFFTAG_IMAGEWIDTH, &width );
    TIFFGetField ( tiffInput, TIFFTAG_IMAGELENGTH, &height );
    TIFFGetField ( tiffInput, TIFFTAG_PHOTOMETRIC, &photometric );
    TIFFGetField ( tiffInput, TIFFTAG_COMPRESSION, &compression );

    TIFFGetField ( tiffInput, TIFFTAG_BITSPERSAMPLE, &bitspersample );
    TIFFGetFieldDefaulted ( tiffInput, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel );

    pixelSize = ( bitspersample * samplesperpixel ) / 8;

    if (! TIFFIsTiled ( tiffInput ) ) {
        LOGGER_ERROR ( "Handled only tiled TIFF images" );
    }
        
    TIFFGetField ( tiffInput, TIFFTAG_TILEWIDTH, &tileWidth );
    TIFFGetField ( tiffInput, TIFFTAG_TILELENGTH, &tileHeight );

    TIFFClose ( tiffInput );

    tileWidthwise = width/tileWidth;
    tileHeightwise = height/tileHeight;
    tilesNumber = tileWidthwise * tileHeightwise;

    /* Initialisation des différents buffers : on choisit de mémoriser autant de tuile qu'il y en a dans le sens de la largeur
     * (pour faciliter la construction d'un ligne à partir des tuiles */
    memorySize = tileWidthwise;

    memorizedTiles = new uint8_t*[memorySize];
    memset ( memorizedTiles, 0, memorySize*sizeof ( uint8_t* ) );
    
    memorizedIndex = new int[memorySize];
    memset ( memorizedIndex, -1, memorySize*sizeof ( int ) );
}

uint8_t* TiledTiffReader::memorizeRawTile ( size_t& size, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        size = 0;
        return NULL;
    }

    size = tileWidth * tileHeight * pixelSize;

    int index = tile%memorySize;

    if ( memorizedIndex[index] != tile ) {
        /* la tuile n'est pas mémorisée, on doit la récupérer et la stocker dans memorizedTiles */
        LOGGER_DEBUG ( "Not memorized tile (" << tile << "). We read, decompress, and memorize it");

        FileDataSource* encData = new FileDataSource(filename, ROK4_IMAGE_HEADER + tile*4, ROK4_IMAGE_HEADER + tilesNumber*4 + tile*4, "");

        DataSource* decData;

        if ( compression == COMPRESSION_NONE ) {
            decData = encData;
        }
        else if ( compression == COMPRESSION_JPEG ) {
            decData = new DataSourceDecoder<JpegDecoder> ( encData );
        }
        else if ( compression == COMPRESSION_LZW ) {
            decData = new DataSourceDecoder<LzwDecoder> ( encData );
        }
        else if ( compression == COMPRESSION_PACKBITS ) {
            decData = new DataSourceDecoder<PackBitsDecoder> ( encData );
        }
        else if ( compression == COMPRESSION_DEFLATE || compression == COMPRESSION_ADOBE_DEFLATE ) {
            /* Avec une telle compression dans l'en-tête TIFF, on peut avoir :
             *       - des tuiles compressée en deflate (format "officiel")
             *       - des tuiles en PNG, format propre à ROK4
             * Pour distinguer les deux cas (pas le même décodeur), on va tester la présence d'un en-tête PNG */
            size_t tmpSize;
            const uint8_t* header = encData->getData(tmpSize);
            if (memcmp(PNG_SIGNATURE, header, 8)) {
                decData = new DataSourceDecoder<DeflateDecoder> ( encData );
            } else {
                decData = new DataSourceDecoder<PngDecoder> ( encData );
            }
        }
        else {
            LOGGER_ERROR ( "Unhandled compression : " << compression );
            return 0;
        }

        const uint8_t* data = decData->getData(size);

        if (size == 0) {
            LOGGER_ERROR("Unable to decompress tile " << tile);
            return 0;
        } else if (size != tileWidth * tileHeight * pixelSize) {
            LOGGER_WARN("Raw tile size should have been " << tileWidth * tileHeight * pixelSize << ", and not " << size);
        }

        if ( ! memorizedTiles[index] ) memorizedTiles[index] = new uint8_t[tileWidth * tileHeight * pixelSize];
        memcpy(memorizedTiles[index], data, size );
        memorizedIndex[index] = tile;
        
        delete decData;
    }

    return memorizedTiles[index];
}

size_t TiledTiffReader::getRawTile ( uint8_t* buf, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        return 0;
    }

    size_t tileSize;

    uint8_t* memoryPlace = memorizeRawTile(tileSize, tile);

    buf = new uint8_t[tileSize];
    memcpy(buf, memoryPlace, tileSize );

    return tileSize;
}
    
size_t TiledTiffReader::getEncodedTile ( uint8_t* buf, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        return 0;
    }
    
    FileDataSource encData(filename, ROK4_IMAGE_HEADER + tile*4, ROK4_IMAGE_HEADER + tilesNumber*4 + tile*4, "");
    size_t tileSize = 0;

    const uint8_t* data = encData.getData(tileSize);

    if (tileSize == 0) {
        LOGGER_ERROR("Unable to read encoded tile " << tile);
        return 0;
    }

    buf = new uint8_t[tileSize];
    memcpy(buf, data, tileSize);
    
    encData.releaseData();
    
    return tileSize;
}

size_t TiledTiffReader::getLine ( uint8_t* buf, int line )
{    
    int tileRow = line / tileHeight;
    int tileLine = line % tileHeight;
    int tileLineSize = tileWidth * pixelSize;
    size_t tileSize;
    
    // le buffer est déjà alloue

    // On mémorise toutes les tuiles qui seront nécessaires pour constituer la ligne
    for ( int tileCol = 0; tileCol < tileWidthwise; tileCol++ ) {
        uint8_t* mem = memorizeRawTile ( tileSize, tileRow * tileWidthwise + tileCol);
        memcpy ( buf + tileCol * tileWidth * samplesperpixel, mem + tileLine * tileLineSize, tileLineSize );
    }
    
    return width * pixelSize;
}

size_t TiledTiffReader::getLine ( float* buf, int line )
{
    int tileRow = line / tileHeight;
    int tileLine = line % tileHeight;
    int tileLineSize = tileWidth * pixelSize;
    size_t tileSize;

    // le buffer est déjà alloue

    // On mémorise toutes les tuiles qui seront nécessaires pour constituer la ligne
    for ( int tileCol = 0; tileCol < tileWidthwise; tileCol++ ) {
        uint8_t* mem = memorizeRawTile ( tileSize, tileRow * tileWidthwise + tileCol);
        memcpy ( buf + tileCol * tileWidth * samplesperpixel, mem + tileLine * tileLineSize, tileLineSize );
    }

    return width * pixelSize;
}

