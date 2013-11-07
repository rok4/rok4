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
 * \file TiledTiffReader.h
 ** \~french
 * \brief Définition de la classe TiffReader
 * \details Image physique, attaché à un fichier. Utilise la librairie libtiff.
 ** \~english
 * \brief Define class TiffReader
 * \details Physical image, linked to a file. Use libtiff library.
 */

#ifndef _TIFFREADER_
#define _TIFFREADER_

#include <stdint.h>
#include "tiffio.h"
#include "Logger.h"

#define ROK4_IMAGE_HEADER 2048

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Lecture d'une image TIFF tuilée, type ROK4.
 * \details Cette classe va utiliser la librairie TIFF afin de lire les données et de récupérer les informations sur les images.
 *
 * Les images tuilées sont gérées.
 */
class TiledTiffReader {
public:
    /**
     * \~french \brief Chemin du fichier image
     * \~english \brief Path to the image file
     */
    char* filename;
    
    /**
     * \~french \brief Flux vers le fichier image, permettant la lecture des données
     * \~english \brief Stream to image file, allowing to read data
     */
    FILE *input;

    /**
     * \~french \brief Largeur de l'image en pixel
     * \~english \brief Image's width, in pixel
     */
    uint32_t width;
    /**
     * \~french \brief Hauteur de l'image en pixel
     * \~english \brief Image's height, in pixel
     */
    uint32_t height;
    /**
     * \~french \brief Photométrie des données (rgb, gray...)
     * \~english \brief Data photometric (rgb, gray...)
     */
    uint16_t photometric;
    /**
     * \~french \brief Compression des données (jpeg, packbits...)
     * \~english \brief Data compression (jpeg, packbits...)
     */
    uint16_t compression;
    
    /**
     * \~french \brief Largeur d'une tuile de l'image en pixel
     * \~english \brief Image's tile width, in pixel
     */
    uint32_t tileWidth;
    /**
     * \~french \brief Hauteur d'une tuile de l'image en pixel
     * \~english \brief Image's tile height, in pixel
     */
    uint32_t tileHeight;

    /**
     * \~french \brief Nombre de tuile dans le sens de la largeur
     * \~english \brief Number of tiles, widthwise
     */
    int tileWidthwise;
    /**
     * \~french \brief Nombre de tuile dans le sens de la hauteur
     * \~english \brief Number of tiles, heightwise
     */
    int tileHeightwise;

    /**
     * \~french \brief Nombre de tuile dans l'image
     * \~english \brief Number of tiles
     */
    int tilesNumber;

    /**
     * \~french \brief Nombre de bits par canal
     * \~english \brief Number of bits per sample
     */
    uint16_t bitspersample;

    /**
     * \~french \brief Nombre de canaux de l'image
     * \~english \brief Number of samples per pixel
     */
    uint16_t samplesperpixel;

    /**
     * \~french \brief Taille d'un octet en pixel
     * \~english \brief Byte pixel's size
     */
    int pixelSize;

    /**
     * \~french \brief Indices des débuts des tuiles de l'image
     * \~english \brief Image's tiles' indices
     */
    uint32_t *tilesOffsets;

    /**
     * \~french \brief Tailles des tuiles de l'image
     * \~english \brief Image's tiles' sizes
     */
    uint32_t *tilesSizes;

    /**
     * \~french \brief Buffer temporaire pour une ligne
     * \details Utilisé pour récupérer une ligne depuis les tuiles.
     * \~english \brief Temporary buffer for a line
     * \details Used to get line from tiles.
     */
    uint8_t *tmpLine;
    /**
     * \~french \brief Buffer temporaire pour une tuile
     * \details Utilisé pour récupérer une tuile compressée et la décompresser.
     * \~english \brief Temporary buffer for a tile
     * \details Used to get a compressed line and uncompress it.
     */
    uint8_t *tmpTile;

    /**
     * \~french \brief Nombre de tuiles mémorisées
     * \~english \brief Number of memorized tiles
     */
    int memorySize;
    
    /**
     * \~french \brief Buffer contenant les tuiles mémorisées
     * \details On mémorise les tuiles décompressées
     * Taille : memorySize
     * \~english \brief Buffer containing memorized tiles
     * \details We memorize uncompressed tiles
     * Size : memorySize
     */
    uint8_t **memorizedTiles;

    /**
     * \~french \brief Buffer précisant pour chaque tuile mémorisée dans memorizedTiles son indice
     * \details -1 si aucune tuile n'est mémorisée à cet emplacement dans memorizedTiles
     * Taille : memorySize.
     * \~english \brief Buffer precising for each memorized tile's indice
     * \details -1 if no tile is memorized into this place in memorizedTiles
     * Size : memorySize
     */
    int* memorizedIndex;

public:
    
    int getRawTile ( uint8_t* buf, int tile );
    int getEncodedTile ( uint8_t* buf, int tile );
    int getLine ( uint8_t* buf, int line );

    TiledTiffReader ( const char* name );

    /**
     * \~french
     * \brief Retourne la largeur en pixel de l'image
     * \return largeur
     * \~english
     * \brief Return the image's width
     * \return width
     */
    uint32_t getWidth() {
        return width;
    }
    /**
     * \~french
     * \brief Retourne la hauteur en pixel de l'image
     * \return hauteur
     * \~english
     * \brief Return the image's height
     * \return height
     */
    uint32_t getHeight() {
        return height;
    }

    /**
     * \~french
     * \brief Retourne la photométrie des données image (rgb, gray...)
     * \return photométrie
     * \~english
     * \brief Return data photometric (rgb, gray...)
     * \return photometric
     */
    uint16_t getPhotometric() {
        return photometric;
    }

    /**
     * \~french
     * \brief Retourne la taille en octet d'un pixel
     * \return taille d'un octet en pixel
     * \~english
     * \brief Return the byte pixel's size
     * \return byte pixel's size
     */
    int getPixelSize() {
        return pixelSize;
    }

    /**
     * \~french
     * \brief Retourne le nombre de bits par canal
     * \return nombre de bits par canal
     * \~english
     * \brief Return number of bits per sample
     * \return number of bits per sample
     */
    uint16_t getBitsPerSample() {
        return bitspersample;
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~TiledTiffReader() {
        fclose ( input );
        for ( int i = 0; i < memorySize; i++ ) if ( memorizedTiles[i] ) delete[] memorizedTiles[i];
        delete[] lineBuffer;
        delete[] memorizedTiles;
        delete[] memorizedIndex;
        delete[] filename;
    }

    /**
     * \~french
     * \brief Sortie des informations sur le lectuer d'image
     * \~english
     * \brief Reader description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ TiffReader -------" );
        LOGGER_INFO ( "\t- filepath = " << filename );
        LOGGER_INFO ( "\t- width = " << width << ", height = " << height );
        LOGGER_INFO ( "\t- tile width = " << tileWidth << ", tile height = " << tileHeight );
        LOGGER_INFO ( "\t- bits per sample = " << bitspersample );
        LOGGER_INFO ( "\t- bytes per pixel = " << pixelSize );
        LOGGER_INFO ( "\t- photometric = " << photometric );
    }
};

#endif // _TIFFREADER_