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
 * \file Rok4Image.h
 ** \~french
 * \brief Implémentation des classes Rok4Image et Rok4ImageFactory
 * \details
 * \li Rok4Image : gestion d'une image aux spécifications ROK4 Server (TIFF tuilé), en écriture et lecture
 * \li Rok4ImageFactory : usine de création d'objet Rok4Image
 ** \~english
 * \brief Implement classes Rok4Image and Rok4ImageFactory
 * \details
 * \li Rok4Image : manage a ROK4 Server specifications image (tiled TIFF), reading and writting
 * \li Rok4ImageFactory : factory to create Rok4Image object
 */

#include "Rok4Image.h"
#include "byteswap.h"
#include "lzwEncoder.h"
#include "pkbEncoder.h"
#include "FileDataSource.h"
#include "Decoder.h"
#include "Logger.h"
#include "Utils.h"
#include <iostream>
#include <algorithm>

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------- Fonctions pour le manager de sortie de la libjpeg -------------------- */

void init_destination ( jpeg_compress_struct *cinfo ) {
    return;
}
boolean empty_output_buffer ( jpeg_compress_struct *cinfo ) {
    return false;
}
void term_destination ( jpeg_compress_struct *cinfo ) {
    return;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------- Fonctions pour écrire des TIFFTAG dans l'en-tête --------------------- */

static inline void writeTIFFTAG (char** p,  uint16_t tag, uint16_t tagFormat, uint32_t card, uint32_t value ) {
    * ( ( uint16_t* ) *p ) = tag;
    * ( ( uint16_t* ) (*p + 2) ) = tagFormat;
    * ( ( uint32_t* ) (*p + 4) ) = card;
    * ( ( uint32_t* ) (*p + 8) ) = value;
    *p += 12;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONSTANTES ------------------------------------------ */

static const uint8_t PNG_IEND[12] = {
    0, 0, 0, 0, 'I', 'E', 'N', 'D',        // 8  | taille et type du chunck IHDR
    0xae, 0x42, 0x60, 0x82
}; // crc32


static const uint8_t PNG_HEADER[33] = {
    137, 80, 78, 71, 13, 10, 26, 10,       // 0  | 8 octets d'entête
    0, 0, 0, 13, 'I', 'H', 'D', 'R',       // 8  | taille et type du chunck IHDR
    0, 0, 1, 0,                            // 16 | width
    0, 0, 1, 0,                            // 20 | height
    8,                                     // 24 | bit depth
    0,                                     // 25 | Colour type
    0,                                     // 26 | Compression method
    0,                                     // 27 | Filter method
    0,                                     // 28 | Interlace method
    0, 0, 0, 0
}; // 29 | crc32

static const uint8_t white[4] = {255,255,255,255};

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */


static SampleFormat::eSampleFormat toROK4SampleFormat ( uint16_t sf ) {
    switch ( sf ) {
    case SAMPLEFORMAT_UINT :
        return SampleFormat::UINT;
    case SAMPLEFORMAT_IEEEFP :
        return SampleFormat::FLOAT;
    default :
        return SampleFormat::UNKNOWN;
    }
}

static uint16_t fromROK4SampleFormat ( SampleFormat::eSampleFormat sf ) {
    switch ( sf ) {
    case SampleFormat::UINT :
        return SAMPLEFORMAT_UINT;
    case SampleFormat::FLOAT :
        return SAMPLEFORMAT_IEEEFP;
    default :
        return 0;
    }
}

static Photometric::ePhotometric toROK4Photometric ( uint16_t ph ) {
    switch ( ph ) {
    case PHOTOMETRIC_MINISBLACK :
        return Photometric::GRAY;
    case PHOTOMETRIC_RGB :
        return Photometric::RGB;
    case PHOTOMETRIC_YCBCR :
        return Photometric::YCBCR;
    case PHOTOMETRIC_MASK :
        return Photometric::MASK;
    default :
        return Photometric::UNKNOWN;
    }
}

static uint16_t fromROK4Photometric ( Photometric::ePhotometric ph ) {
    switch ( ph ) {
    case Photometric::GRAY :
        return PHOTOMETRIC_MINISBLACK;
    case Photometric::RGB :
        return PHOTOMETRIC_RGB;
    case Photometric::YCBCR :
        return PHOTOMETRIC_YCBCR;
    case Photometric::MASK :
        return PHOTOMETRIC_MINISBLACK;
    default :
        return 0;
    }
}

static Compression::eCompression toROK4Compression ( uint16_t comp ) {
    switch ( comp ) {
    case COMPRESSION_NONE :
        return Compression::NONE;
    case COMPRESSION_ADOBE_DEFLATE :
        return Compression::DEFLATE;
    case COMPRESSION_JPEG :
        return Compression::JPEG;
    case COMPRESSION_DEFLATE :
        return Compression::DEFLATE;
    case COMPRESSION_LZW :
        return Compression::LZW;
    case COMPRESSION_PACKBITS :
        return Compression::PACKBITS;
    default :
        return Compression::UNKNOWN;
    }
}

static uint16_t fromROK4Compression ( Compression::eCompression comp ) {
    switch ( comp ) {
    case Compression::NONE :
        return COMPRESSION_NONE;
    case Compression::DEFLATE :
        return COMPRESSION_ADOBE_DEFLATE;
    case Compression::JPEG :
        return COMPRESSION_JPEG;
    case Compression::PNG :
        return COMPRESSION_ADOBE_DEFLATE;
    case Compression::LZW :
        return COMPRESSION_LZW;
    case Compression::PACKBITS :
        return COMPRESSION_PACKBITS;
    default :
        return 0;
    }
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

Rok4Image* Rok4ImageFactory::createRok4ImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {

    int width=0, height=0, channels=0, planarconfig=0, bitspersample=0, sampleformat=0, photometric=0, compression=0;
    int tileWidth=0, tileHeight=0;
    
    TIFF* tif = TIFFOpen ( filename, "r" );

    if ( tif == NULL ) {
        LOGGER_ERROR ( "Unable to open ROK4 TIFF image (to read) " << filename );
        return NULL;
    } else {
        /**************** DIMENSIONS GLOBALES ****************/
        if ( TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) < 1 ) {
            LOGGER_ERROR ( "Unable to read pixel width for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_IMAGELENGTH, &height ) < 1 ) {
            LOGGER_ERROR ( "Unable to read pixel height for file " << filename );
            return NULL;
        }

        /********************** TUILAGE **********************/
        if (! TIFFIsTiled ( tif ) ) {
            LOGGER_ERROR ( "Handled only tiled TIFF images" );
            return NULL;
        }
        if ( TIFFGetField ( tif, TIFFTAG_TILEWIDTH, &tileWidth ) < 1 ) {
            LOGGER_ERROR ( "Unable to read tile's width for file " << filename );
            return NULL;
        }
        if ( TIFFGetField ( tif, TIFFTAG_TILELENGTH, &tileHeight ) < 1 ) {
            LOGGER_ERROR ( "Unable to read tile's height for file " << filename );
            return NULL;
        }

        /************ FORMAT DES PIXELS ET CANAUX ************/
        if ( TIFFGetField ( tif, TIFFTAG_SAMPLESPERPIXEL,&channels ) < 1 ) {
            LOGGER_ERROR ( "Unable to read number of samples per pixel for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_PLANARCONFIG,&planarconfig ) < 1 ) {
            LOGGER_ERROR ( "Unable to read planar configuration for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_BITSPERSAMPLE,&bitspersample ) < 1 ) {
            LOGGER_ERROR ( "Unable to read number of bits per sample for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_SAMPLEFORMAT,&sampleformat ) < 1 ) {
            LOGGER_ERROR ( "Unable to read sample format for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_PHOTOMETRIC,&photometric ) < 1 ) {
            LOGGER_ERROR ( "Unable to read photometric for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_COMPRESSION,&compression ) < 1 ) {
            LOGGER_ERROR ( "Unable to read compression for file " << filename );
            return NULL;
        }

        uint16_t extrasamplesCount;
        uint16_t* extrasamples;
        if ( TIFFGetField ( tif, TIFFTAG_EXTRASAMPLES, &extrasamplesCount, &extrasamples ) > 0 ) {
            // On a des canaux en plus, si c'est de l'alpha, il ne doit pas être associé 
            if ( extrasamples[0] == EXTRASAMPLE_ASSOCALPHA ) {
                LOGGER_ERROR ( "Alpha sample should be unassociated for the rok4 image " << filename );
                return NULL;
            }
        }
    }

    if ( planarconfig != PLANARCONFIG_CONTIG ) {
        LOGGER_ERROR ( "Planar configuration have to be 'PLANARCONFIG_CONTIG' for file " << filename );
        return NULL;
    }

    TIFFClose ( tif );

    SampleFormat::eSampleFormat sf = toROK4SampleFormat ( sampleformat );

    if ( ! SampleFormat::isHandledSampleType ( sf, bitspersample ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
        // Arrondi a la valeur entiere la plus proche
        int calcWidth = lround ( ( bbox.xmax - bbox.xmin ) / ( resx ) );
        int calcHeight = lround ( ( bbox.ymax - bbox.ymin ) / ( resy ) );
        if ( calcWidth != width || calcHeight != height ) {
            LOGGER_ERROR ( "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" );
            LOGGER_ERROR ( "Height is " << height << " and calculation give " << calcHeight );
            LOGGER_ERROR ( "Width is " << width << " and calculation give " << calcWidth );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    return new Rok4Image (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, toROK4Photometric ( photometric ), toROK4Compression ( compression ),
        tileWidth, tileHeight
    );
}

Rok4Image* Rok4ImageFactory::createRok4ImageToWrite (
    char* filename, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
    Compression::eCompression compression, int tileWidth, int tileHeight ) {

    if (width % tileWidth != 0 || height % tileHeight != 0) {
        LOGGER_ERROR("Image's dimensions have to be a multiple of tile's dimensions");
        return NULL;
    }

    if (compression == Compression::JPEG) {
        if (photometric == Photometric::GRAY) {
            LOGGER_ERROR("Gray JPEG is not handled");
            return NULL;
        }

        if (sampleformat != SampleFormat::UINT || bitspersample != 8) {
            LOGGER_ERROR("JPEG compression just handle 8-bits integer samples");
            return NULL;
        }
    }
    
    if (compression == Compression::JPEG && photometric == Photometric::RGB)
        photometric = Photometric::YCBCR;

    if (compression != Compression::JPEG && photometric == Photometric::YCBCR)
        photometric = Photometric::RGB;

    if (compression == Compression::PNG) {
        if (sampleformat != SampleFormat::UINT || bitspersample != 8) {
            LOGGER_ERROR("PNG compression just handle 8-bits integer samples");
            return NULL;
        }
    }

    return new Rok4Image (
        width, height, resx, resy, channels, bbox, filename,
        sampleformat, bitspersample, photometric, compression, tileWidth, tileHeight
    );
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

Rok4Image::Rok4Image (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
    Compression::eCompression compression, int tileWidth, int tileHeight ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ), 
    tileWidth (tileWidth), tileHeight(tileHeight)
{
    tileWidthwise = width/tileWidth;
    tileHeightwise = height/tileHeight;
    tilesNumber = tileWidthwise * tileHeightwise;

    rawTileSize = tileWidth * tileHeight * pixelSize;

    rawTileLineSize = tileWidth * pixelSize;

    /* Initialisation des différents buffers : on choisit de mémoriser autant de tuile qu'il y en a dans le sens de la largeur
     * (pour faciliter la construction d'un ligne à partir des tuiles */
    memorySize = tileWidthwise;

    memorizedTiles = new uint8_t*[memorySize];
    memset ( memorizedTiles, 0, memorySize*sizeof ( uint8_t* ) );

    memorizedIndex = new int[memorySize];
    memset ( memorizedIndex, -1, memorySize*sizeof ( int ) );
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

uint8_t* Rok4Image::memorizeRawTile ( size_t& size, int tile )
{    
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        size = 0;
        return NULL;
    }

    size = rawTileSize;

    int index = tile%memorySize;

    if ( memorizedIndex[index] != tile ) {
        /* la tuile n'est pas mémorisée, on doit la récupérer et la stocker dans memorizedTiles */
        LOGGER_DEBUG ( "Not memorized tile (" << tile << "). We read, decompress, and memorize it");

        FileDataSource* encData = new FileDataSource(filename, ROK4_IMAGE_HEADER_SIZE + tile*4, ROK4_IMAGE_HEADER_SIZE + tilesNumber*4 + tile*4, "");

        DataSource* decData;
        size_t tmpSize;

        if ( compression == Compression::NONE ) {
            decData = encData;
        }
        else if ( compression == Compression::JPEG ) {
            decData = new DataSourceDecoder<JpegDecoder> ( encData );
        }
        else if ( compression == Compression::LZW ) {
            decData = new DataSourceDecoder<LzwDecoder> ( encData );
        }
        else if ( compression == Compression::PACKBITS ) {
            decData = new DataSourceDecoder<PackBitsDecoder> ( encData );
        }
        else if ( compression == Compression::DEFLATE || compression == Compression::PNG ) {
            /* Avec une telle compression dans l'en-tête TIFF, on peut avoir :
             *       - des tuiles compressée en deflate (format "officiel")
             *       - des tuiles en PNG, format propre à ROK4
             * Pour distinguer les deux cas (pas le même décodeur), on va tester la présence d'un en-tête PNG */
            const uint8_t* header = encData->getData(tmpSize);
            if (memcmp(PNG_HEADER, header, 8)) {
                decData = new DataSourceDecoder<DeflateDecoder> ( encData );
            } else {
                compression = Compression::PNG;
                decData = new DataSourceDecoder<PngDecoder> ( encData );
            }
        }
        else {
            LOGGER_ERROR ( "Unhandled compression : " << compression );
            return NULL;
        }

        const uint8_t* data = decData->getData(tmpSize);
        
        if (! data || tmpSize == 0) {
            LOGGER_ERROR("Unable to decompress tile " << tile);
            return NULL;
        } else if (tmpSize != rawTileSize) {
            LOGGER_WARN("Raw tile size should have been " << rawTileSize << ", and not " << tmpSize);
        }

        if ( ! memorizedTiles[index] ) memorizedTiles[index] = new uint8_t[rawTileSize];
        memcpy(memorizedTiles[index], data, rawTileSize );
        memorizedIndex[index] = tile;

        delete decData;
    }

    return memorizedTiles[index];
}

int Rok4Image::getRawTile ( uint8_t* buf, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        return 0;
    }

    size_t tileSize;

    uint8_t* memoryPlace = memorizeRawTile(tileSize, tile);
    if (memoryPlace == NULL) {
        LOGGER_ERROR ( "Cannot read raw tile " << tilesNumber );
        return 0;
    }

    buf = new uint8_t[tileSize];
    memcpy(buf, memoryPlace, tileSize );

    return tileSize;
}

int Rok4Image::getEncodedTile ( uint8_t* buf, int tile )
{
    if ( tile < 0 || tile >= tilesNumber ) {
        LOGGER_ERROR ( "Unvalid tile's indice (" << tile << "). Have to be between 0 and " << tilesNumber-1 );
        return 0;
    }

    FileDataSource encData(filename, ROK4_IMAGE_HEADER_SIZE + tile*4, ROK4_IMAGE_HEADER_SIZE + tilesNumber*4 + tile*4, "");
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

int Rok4Image::getline ( uint8_t* buffer, int line ) {
    int tileRow = line / tileHeight;
    int tileLine = line % tileHeight;
    size_t tileSize;

    // le buffer est déjà alloue

    // On mémorise toutes les tuiles qui seront nécessaires pour constituer la ligne
    for ( int tileCol = 0; tileCol < tileWidthwise; tileCol++ ) {
        uint8_t* mem = memorizeRawTile ( tileSize, tileRow * tileWidthwise + tileCol);
        memcpy ( buffer + tileCol * tileWidth * channels, mem + tileLine * rawTileLineSize, rawTileLineSize );
    }

    return width * pixelSize;
}

int Rok4Image::getline ( float* buffer, int line ) {
    int tileRow = line / tileHeight;
    int tileLine = line % tileHeight;

    // le buffer est déjà alloue

    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        // On convertit
        uint8_t* buffer_t = new uint8_t[width*channels];
        getline ( buffer_t,line );
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        
    } else { // float
        // On mémorise toutes les tuiles qui seront nécessaires pour constituer la ligne
        size_t tileSize;
        for ( int tileCol = 0; tileCol < tileWidthwise; tileCol++ ) {
            uint8_t* mem = memorizeRawTile ( tileSize, tileRow * tileWidthwise + tileCol);
            if (mem == NULL) {
                LOGGER_ERROR ( "Cannot read raw tile " << tilesNumber );
                return 0;
            }
            memcpy ( buffer + tileCol * tileWidth * channels, mem + tileLine * rawTileLineSize, rawTileLineSize );
        }
    }

    return width * channels;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- ECRITURE ------------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

/** \todo Écriture d'images ROK4 en JPEG gris */
int Rok4Image::writeImage ( Image* pIn, bool crop )
{
    if (compression != Compression::JPEG && crop) {
        LOGGER_WARN("Crop option is reserved for JPEG compression");
        crop = false;
    }
    
    if (! prepare()) {
        LOGGER_ERROR("Cannot write the ROK4 images header for " << filename);
        return -1;
    }

    int imageLineSize = width * channels;
    int tileLineSize = tileWidth * channels;
    uint8_t tile[tileHeight*rawTileLineSize];

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        uint8_t* lines = new uint8_t[tileHeight*imageLineSize];

        for ( int y = 0; y < tileHeightwise; y++ ) {
            // On récupère toutes les lignes pour cette ligne de tuiles
            for (int lig = 0; lig < tileHeight; lig++) {
                pIn->getline(lines + lig*imageLineSize, y*tileHeight + lig);
            }
            for ( int x = 0; x < tileWidthwise; x++ ) {
                // On constitue la tuile
                for (int lig = 0; lig < tileHeight; lig++) {
                    memcpy(tile + lig*rawTileLineSize, lines + lig*imageLineSize + x*tileLineSize, rawTileLineSize);
                }
                int tileInd = y*tileWidthwise + x;

                if (! writeTile(tileInd, tile, crop)) {
                    LOGGER_ERROR("Error writting tile " << tileInd << " for ROK4 image " << filename);
                    return -1;
                }
            }
        }
        
        delete [] lines;
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        float* lines = new float[tileHeight*imageLineSize];

        for ( int y = 0; y < tileHeightwise; y++ ) {
            // On récupère toutes les lignes pour cette ligne de tuiles
            for (int lig = 0; lig < tileHeight; lig++) {
                pIn->getline(lines + lig*imageLineSize, y*tileHeight + lig);
            }
            for ( int x = 0; x < tileWidthwise; x++ ) {
                // On constitue la tuile
                for (int lig = 0; lig < tileHeight; lig++) {
                    memcpy(tile + lig*rawTileLineSize, lines + lig*imageLineSize + x*tileLineSize, rawTileLineSize);
                }

                int tileInd = y*tileWidthwise + x;

                if (! writeTile(tileInd, tile, crop)) {
                    LOGGER_ERROR("Error writting tile " << tileInd << " for ROK4 image " << filename);
                    return -1;
                }
            }
        }
        delete [] lines;
    }

    if (! close()) {
        LOGGER_ERROR("Cannot close the ROK4 images (write index and clean) for " << filename);
        return -1;
    }
    
    return 0;
}

int Rok4Image::writeImage ( Image* pIn )
{
    return writeImage(pIn, false);
}

bool Rok4Image::prepare()
{
    output.open ( filename, std::ios_base::trunc | std::ios::binary );
    if ( !output ) LOGGER_ERROR("Unable to open output file " << filename);

    int quality = 0;
    if ( compression == Compression::PNG) quality = 5;
    if ( compression == Compression::DEFLATE ) quality = 6;
    if ( compression == Compression::JPEG ) quality = 75;

    char header[ROK4_IMAGE_HEADER_SIZE], *p = header;
    memset ( header, 0, sizeof ( header ) );

    * ( ( uint16_t* ) ( p ) )      = 0x4949;    // Little Endian
    * ( ( uint16_t* ) ( p + 2 ) ) = 42;        // Tiff specification
    * ( ( uint32_t* ) ( p + 4 ) ) = 16;        // Offset of the IFD
    p += 8;

    // write the number of entries in the IFD

    // We can have 4 samples per pixel, each sample with the same size
    * ( ( uint16_t* ) ( p ) ) = (uint16_t) bitspersample;
    * ( ( uint16_t* ) ( p + 2 ) ) = (uint16_t) bitspersample;
    * ( ( uint16_t* ) ( p + 4 ) ) = (uint16_t) bitspersample;
    * ( ( uint16_t* ) ( p + 6 ) ) = (uint16_t) bitspersample;
    p += 8;

    // Number of tags
    * ( ( uint16_t* ) p ) = 11;
    if ( photometric == Photometric::YCBCR ) * ( ( uint16_t* ) p ) += 1;
    if ( channels == 4 || channels == 2 ) * ( ( uint16_t* ) p ) += 1;
    p += 2;

    //  Offset of the IFD is here
    writeTIFFTAG(&p, TIFFTAG_IMAGEWIDTH, TIFF_LONG, 1, width);
    writeTIFFTAG(&p, TIFFTAG_IMAGELENGTH, TIFF_LONG, 1, height);

    if ( channels == 1 ) {
        writeTIFFTAG(&p, TIFFTAG_BITSPERSAMPLE, TIFF_SHORT, 1, bitspersample);
    } else if ( channels == 2 ) {
        * ( ( uint16_t* ) ( p ) ) = TIFFTAG_BITSPERSAMPLE;
        * ( ( uint16_t* ) ( p + 2 ) ) = TIFF_SHORT;
        * ( ( uint32_t* ) ( p + 4 ) ) = 2;
        * ( ( uint16_t* ) ( p + 8 ) ) = 8;
        * ( ( uint16_t* ) ( p + 10 ) )  = 8;
        p += 12;
    } else {
        writeTIFFTAG(&p, TIFFTAG_BITSPERSAMPLE, TIFF_SHORT, channels, 8);
    }

    writeTIFFTAG(&p, TIFFTAG_COMPRESSION, TIFF_SHORT, 1, fromROK4Compression(compression));
    writeTIFFTAG(&p, TIFFTAG_PHOTOMETRIC, TIFF_SHORT, 1, fromROK4Photometric(photometric));
    writeTIFFTAG(&p, TIFFTAG_SAMPLESPERPIXEL, TIFF_SHORT, 1, channels);
    writeTIFFTAG(&p, TIFFTAG_TILEWIDTH, TIFF_LONG, 1, tileWidth);
    writeTIFFTAG(&p, TIFFTAG_TILELENGTH, TIFF_LONG, 1, tileHeight);
    
    if ( tilesNumber == 1 ) {
        /* Dans le cas d'une tuile unique, le champs contient directement la valeur et pas l'adresse de la valeur.
         * Cependant, étant donnée le mode de foncionnement de Rok4, on doit laisser la valeur au début de l'image.
         * Voilà pourquoi on ajoute 8 à ROK4_IMAGE_HEADER_SIZE : 4 pour le TileOffset et 4 pour le TileByteCount.
         */
        writeTIFFTAG(&p, TIFFTAG_TILEOFFSETS, TIFF_LONG, tilesNumber, ROK4_IMAGE_HEADER_SIZE + 8);
    } else {
        writeTIFFTAG(&p, TIFFTAG_TILEOFFSETS, TIFF_LONG, tilesNumber, ROK4_IMAGE_HEADER_SIZE);
    }

    // Dans le cas d'un tuile unique, on vidra écraser la valeur mise ici avec directement sa taille
    writeTIFFTAG(&p, TIFFTAG_TILEBYTECOUNTS, TIFF_LONG, tilesNumber, ROK4_IMAGE_HEADER_SIZE + 4 * tilesNumber);
    
    if ( channels == 4 || channels == 2 ) {
        writeTIFFTAG(&p, TIFFTAG_EXTRASAMPLES, TIFF_SHORT, 1, EXTRASAMPLE_UNASSALPHA);
    }
    
    writeTIFFTAG(&p, TIFFTAG_SAMPLEFORMAT, TIFF_SHORT, 1, fromROK4SampleFormat(sampleformat));

    if ( photometric == Photometric::YCBCR ) {
        * ( ( uint16_t* ) ( p ) ) = TIFFTAG_YCBCRSUBSAMPLING;
        * ( ( uint16_t* ) ( p + 2 ) ) = TIFF_SHORT;
        * ( ( uint32_t* ) ( p + 4 ) ) = 2;
        * ( ( uint16_t* ) ( p + 8 ) ) = 2;
        * ( ( uint16_t* ) ( p + 10 ) )  = 2;
        p += 12;
    }

    // end of IFD
    * ( ( uint32_t* ) ( p ) ) = 0; 
    p += 4;
    
    output.write ( header, sizeof ( header ) );

    // variables initalizations
    tilesOffset = new uint32_t[tilesNumber];
    tilesByteCounts = new uint32_t[tilesNumber];
    memset ( tilesOffset, 0, tilesNumber*4 );
    memset ( tilesByteCounts, 0, tilesNumber*4 );
    position = ROK4_IMAGE_HEADER_SIZE + 8 * tilesNumber;

    BufferSize = 2*rawTileSize;
    Buffer = new uint8_t[BufferSize];

    //  z compression initalization
    if ( compression == Compression::PNG || compression == Compression::DEFLATE ) {
        zip_buffer = new uint8_t[rawTileSize];
        zstream.zalloc = Z_NULL;
        zstream.zfree  = Z_NULL;
        zstream.opaque = Z_NULL;
        zstream.data_type = Z_BINARY;
        deflateInit ( &zstream, quality );
    }

    if ( compression == Compression::JPEG ) {
        cinfo.err = jpeg_std_error ( &jerr );
        jpeg_create_compress ( &cinfo );

        cinfo.dest = new jpeg_destination_mgr;
        cinfo.dest->init_destination = init_destination;
        cinfo.dest->empty_output_buffer = empty_output_buffer;
        cinfo.dest->term_destination = term_destination;

        cinfo.image_width  = tileWidth;
        cinfo.image_height = tileHeight;
        cinfo.input_components = 3;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults ( &cinfo );
        jpeg_set_quality ( &cinfo, quality, true );
    }

    return true;
}

bool Rok4Image::writeTile( int tileInd, uint8_t* data, bool crop )
{
    LOGGER_DEBUG("Ecriture de la tuile " << tileInd);
    
    if ( tileInd > tilesNumber || tileInd < 0 ) {
        LOGGER_ERROR ( "Unvalid tile's indice to write (" << tileInd << "). Have to be between 0 and " << tilesNumber-1 );
        return false;
    }
    size_t size;

    switch ( compression ) {
    case Compression::NONE:
        size = computeRawTile ( Buffer, data );
        break;
    case Compression::LZW :
        size = computeLzwTile ( Buffer, data );
        break;
    case Compression::JPEG:
        size = computeJpegTile ( Buffer, data, crop );
        break;
    case Compression::PNG :
        size = computePngTile ( Buffer, data );
        break;
    case Compression::PACKBITS :
        size = computePackbitsTile ( Buffer, data );
        break;
    case Compression::DEFLATE :
        size = computeDeflateTile ( Buffer, data );
        break;
    }

    if ( size == 0 ) return false;

    if ( tilesNumber == 1 ) {
        // On écrit la taille de la tuile unique directemet dans l'en-tête, après le tag TIFFTAG_TILEBYTECOUNTS
        output.seekp ( 134 );
        uint32_t Size[1];
        Size[0] = ( uint32_t ) size;
        output.write ( ( char* ) Size,4 );
    }

    tilesOffset[tileInd] = position;
    tilesByteCounts[tileInd] = size;
    output.seekp ( position );
    output.write ( ( char* ) Buffer, size );
    if ( output.fail() ) return false;
    position = ( position + size + 15 ) & ~15; // Align the next position on 16byte

    return true;
}

bool Rok4Image::close() {
    output.seekp ( ROK4_IMAGE_HEADER_SIZE );
    output.write ( ( char* ) tilesOffset, 4 * tilesNumber );
    output.write ( ( char* ) tilesByteCounts, 4 * tilesNumber );
    output.close();
    if ( output.fail() ) return false;

    // Nettoyage des attributs propres à l'écriture
    delete[] tilesOffset;
    delete[] tilesByteCounts;
    delete[] Buffer;
    if ( compression == Compression::PNG || compression == Compression::DEFLATE ) {
        delete[] zip_buffer;
        deflateEnd ( &zstream );
    }
    if ( compression == Compression::JPEG ) {
        delete cinfo.dest;
        jpeg_destroy_compress ( &cinfo );
    }
    
    return true;
}


size_t Rok4Image::computeRawTile ( uint8_t *buffer, uint8_t *data ) {
    memcpy ( buffer, data, rawTileSize );
    return rawTileSize;
}

size_t Rok4Image::computeLzwTile ( uint8_t *buffer, uint8_t *data ) {

    size_t outSize;

    lzwEncoder LZWE;
    uint8_t* temp = LZWE.encode ( data, rawTileSize, outSize );

    if ( outSize > BufferSize ) {
        delete[] Buffer;
        BufferSize = outSize * 2;
        Buffer = new uint8_t[BufferSize];
    }
    memcpy ( buffer,temp,outSize );
    delete [] temp;

    return outSize;
}

size_t Rok4Image::computePackbitsTile ( uint8_t *buffer, uint8_t *data ) {

    uint8_t* pkbBuffer = new uint8_t[rawTileLineSize*tileHeight*2];
    size_t pkbBufferSize = 0;
    uint8_t* rawLine = new uint8_t[rawTileLineSize];
    int lRead = 0;
    pkbEncoder encoder;
    uint8_t * pkbLine;
    for ( ; lRead < tileHeight ; lRead++ ) {
        memcpy ( rawLine,data+lRead*rawTileLineSize,rawTileLineSize );
        size_t pkbLineSize = 0;
        pkbLine = encoder.encode ( rawLine, rawTileLineSize, pkbLineSize );
        memcpy ( pkbBuffer+pkbBufferSize,pkbLine,pkbLineSize );
        pkbBufferSize += pkbLineSize;
        delete[] pkbLine;
    }

    memcpy ( buffer,pkbBuffer,pkbBufferSize );
    delete[] pkbBuffer;
    delete[] rawLine;

    return pkbBufferSize;
}

size_t Rok4Image::computePngTile ( uint8_t *buffer, uint8_t *data ) {
    uint8_t *B = zip_buffer;
    for ( unsigned int h = 0; h < tileHeight; h++ ) {
        *B++ = 0; // on met un 0 devant chaque ligne (spec png -> mode de filtrage simple)
        memcpy ( B, data + h*rawTileLineSize, rawTileLineSize );
        B += rawTileLineSize;
    }

    memcpy ( buffer, PNG_HEADER, sizeof ( PNG_HEADER ) );
    * ( ( uint32_t* ) ( buffer+16 ) ) = bswap_32 ( tileWidth );
    * ( ( uint32_t* ) ( buffer+20 ) ) = bswap_32 ( tileHeight );
    if ( channels == 1 ) {
        buffer[25] = 0;    // gray
    } else if ( channels == 3 ) {
        buffer[25] = 2;    // RGB
    } else if ( channels == 4 ) {
        buffer[25] = 6;    // RGBA
    }

    uint32_t crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, buffer + 12, 17 );
    * ( ( uint32_t* ) ( buffer+29 ) ) = bswap_32 ( crc );

    zstream.next_out  = buffer + sizeof ( PNG_HEADER ) + 8;
    zstream.avail_out = 2*rawTileSize - 12 - sizeof ( PNG_HEADER ) - sizeof ( PNG_IEND );
    zstream.next_in   = zip_buffer;
    zstream.avail_in  = rawTileSize;

    if ( deflateReset ( &zstream ) != Z_OK ) return -1;
    if ( deflate ( &zstream, Z_FINISH ) != Z_STREAM_END ) return -1;

    * ( ( uint32_t* ) ( buffer+sizeof ( PNG_HEADER ) ) ) =  bswap_32 ( zstream.total_out );
    buffer[sizeof ( PNG_HEADER ) + 4] = 'I';
    buffer[sizeof ( PNG_HEADER ) + 5] = 'D';
    buffer[sizeof ( PNG_HEADER ) + 6] = 'A';
    buffer[sizeof ( PNG_HEADER ) + 7] = 'T';

    crc = crc32 ( 0, Z_NULL, 0 );
    crc = crc32 ( crc, buffer + sizeof ( PNG_HEADER ) + 4, zstream.total_out+4 );
    * ( ( uint32_t* ) zstream.next_out ) = bswap_32 ( crc );

    memcpy ( zstream.next_out + 4, PNG_IEND, sizeof ( PNG_IEND ) );
    return zstream.total_out + 12 + sizeof ( PNG_IEND ) + sizeof ( PNG_HEADER );
}

size_t Rok4Image::computeDeflateTile ( uint8_t *buffer, uint8_t *data ) {
    uint8_t *B = zip_buffer;
    for ( unsigned int h = 0; h < tileHeight; h++ ) {
        memcpy ( B, data + h*rawTileLineSize, rawTileLineSize );
        B += rawTileLineSize;
    }
    zstream.next_out  = buffer;
    zstream.avail_out = 2*rawTileSize;
    zstream.next_in   = zip_buffer;
    zstream.avail_in  = rawTileSize;

    if ( deflateReset ( &zstream ) != Z_OK ) return -1;
    if ( deflate ( &zstream, Z_FINISH ) != Z_STREAM_END ) return -1;
    
    return zstream.total_out;
}


size_t Rok4Image::computeJpegTile ( uint8_t *buffer, uint8_t *data, bool crop ) {

    cinfo.dest->next_output_byte = buffer;
    cinfo.dest->free_in_buffer = 2*rawTileSize;
    jpeg_start_compress ( &cinfo, true );

    int numLine = 0;

    while ( numLine < tileHeight ) {
        if ( numLine % JPEG_BLOC_SIZE == 0 && crop ) {
            int l = std::min ( JPEG_BLOC_SIZE,tileHeight-numLine );
            emptyWhiteBlock ( data + numLine*rawTileLineSize, l );
        }

        uint8_t* line = data + numLine*rawTileLineSize;

        if ( jpeg_write_scanlines ( &cinfo, &line, 1 ) != 1 ) return 0;
        numLine++;
    }

    jpeg_finish_compress ( &cinfo );

    return 2*rawTileSize - cinfo.dest->free_in_buffer;
}

void Rok4Image::emptyWhiteBlock ( uint8_t *buffer, int l ) {

    int I = 0;
    int J = 0;
    bool b = false; /* use to know if the current block has been fill with nodata*/

    int blocklinesize = JPEG_BLOC_SIZE*channels;

    while ( J<rawTileLineSize ) {
        while ( I<l ) {
            if ( !memcmp ( buffer + I*rawTileLineSize + J, white, channels ) ) {
                int jdeb = ( J/blocklinesize ) *blocklinesize;
                int jfin = std::min ( jdeb+blocklinesize,rawTileLineSize );
                for ( int i = 0; i<l; i++ ) {
                    for ( int j = jdeb; j<jfin; j+=channels ) {
                        memcpy ( buffer + i*rawTileLineSize + j, white, channels );
                    }
                }
                I = 0;
                J = jfin;
                b = true;
                break;

            } else {
                I++;
            }
        }
        if ( !b ) {
            I = 0;
            J += channels;
        }
        b = false;
    }
}

