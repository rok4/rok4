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
    
    tilesOffsets = new uint32_t[tilesNumber];
    tilesSizes = new uint32_t[tilesNumber];

    /* La lecture de l'image se fera grâce aux index des tuiles, par un flux standard vers le fichier.
     * On va pouvoir faire de nouveau contrôle pour vérifier qu'il s'agit bien d'une image type ROK
     */
    input = fopen ( filename,"rb" );
    if ( input == NULL ) {
        LOGGER_ERROR ( "Unable to open stream to the ROK4 image " << filename );
    }

    /* Contrôle de la taille du fichier : on sait qu'il y a au moins une en-tête de 2048 octets */
    fseek ( input, 0, SEEK_END );
    long file_size;
    file_size=ftell ( input );
    LOGGER_DEBUG ("ROK4 image's (" << filename << ") size '" << file_size );
    if ( file_size < 2048 ) {
        LOGGER_ERROR ( "The file is too short to be a ROK4 pyramid's image " << filename );
    }
    rewind ( input );

    /* Récupération des indices et tailles de chaque tuile */
    fseek ( input, ROK4_IMAGE_HEADER, SEEK_SET );
    if ( fread ( tilesOffsets, sizeof ( uint32_t ), tilesNumber, input ) != tilesNumber ) {
        LOGGER_ERROR ( "Cannot read tiles' offsets in the ROK4 image " << filename );
    }
    if ( fread ( tilesSizes  , sizeof ( uint32_t ), tilesNumber, input ) != tilesNumber ) {
        LOGGER_ERROR ( "Cannot read tiles' sizes in the ROK4 image " << filename );
    }

    /* Contrôle de la cohhérences des index des tuiles et de la taille totale de l'image ROK4*/
    if (tilesOffsets[tilesNumber-1] + tilesSizes[tilesNumber-1] != file_size) {
        LOGGER_ERROR ( "ROK4 image '" << filename << "' is not valid : tiles' indices and sizes are not consistent");
    }

    /* Initialisation des différents buffers : on choisit de mémoriser autant de tuile qu'il y en a dans le sens de la largeur
     * (pour faciliter la construction d'un ligne à partir des tuiles */
    memorySize = tileWidthwise;
    
    tmpLine = new uint8_t[pixelSize*width];
    tmpTile = new uint8_t[pixelSize*tileWidth*tileHeight];

    memorizedTiles = new uint8_t*[memorySize];
    memset ( memorizedTiles, 0, memorySize*sizeof ( uint8_t* ) );
    
    memorizedIndex = new int[memorySize];
    memset ( memorizedIndex, -1, memorySize*sizeof ( int ) );
}

int TiledTiffReader::getRawTile ( uint8_t* buf, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        return 0;
    }
    
    int tileSize = tileWidth*tileHeight*pixelSize;
    buf = new uint8_t[tileSize];
    int index = tile%memorySize;
    
    if ( memorizedIndex[index] != tile ) {
        /* la tuile n'est pas mémorisée, on doit la récupérer et la stocker dans memorizedTiles */
        LOGGER_DEBUG ( "Not memorized tile (" << tile << "). We read and decompress it");
        
        if ( ! memorizedTiles[index] ) memorizedTiles[index] = new uint8_t[tileSize];
        
        int encodedSize = getEncodedTile ( tmpTile, tile );
    }

    memcpy(buf, memorizedTiles[tile%memorySize], tileSize );
    
    return tileSize;
}
    
int TiledTiffReader::getEncodedTile ( uint8_t* buf, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        return 0;
    }
    
    int tileSize = tilesSizes[tile];
    buf = new uint8_t[tileSize];
    
    fseek ( input, tilesOffsets[tile], SEEK_SET );
    if ( fread ( buf, sizeof ( uint8_t ), tileSize, input ) != tileSize ) {
        LOGGER_ERROR ( "Cannot read tile number" << tile << " in the ROK4 image " << filename );
        return 0;
    }
    
    return tileSize;
}

int TiledTiffReader::getLine ( uint8_t* buf, int line )
{/*
    if ( offset > width ) offset = width;
    if ( size > width - offset ) size = width - offset;

    if (size == 0) {
        return NULL;
    }
    
    int xmin = offset / tileWidth;
    int xmax = ( offset + size - 1 ) / tileWidth;
    int firstTile = ( line / tileHeight ) * ( ( width + tileWidth - 1 ) / tileWidth ) + xmin; // tile number.
    int tileLineSize = tileHeight*pixelSize;
    int tileOffset = ( line % tileHeight ) * tileLineSize;
    for ( int x = xmin; x <= xmax; x++ ) {
        uint8_t *tile = getRawTile ( firstTile++ );
        memcpy ( lineBuffer + tileLineSize*x, tile + tileOffset, tileLineSize );
    }*/
    return 0;
}

