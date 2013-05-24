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
 * \file TiffNodataManager.cpp
 ** \~french
 * \brief Implémentation de la classe TiffNodataManager, permettant de modifier la couleur de nodata des images à canal entier
 ** \~english
 * \brief Implement classe TiffNodataManager, allowing to modify nodata color for byte sample image
 */

#include "TiffNodataManager.h"
#include "Logger.h"
#include "tiffio.h"
#include <cstdlib>
#include <stdint.h>
#include <iostream>
#include <string.h>
#include <queue>

using namespace std;

TiffNodataManager::TiffNodataManager ( uint16 channels, uint8_t *targetValue, bool touchEdges, uint8_t *dataValue, uint8_t *nodataValue ) :
    channels ( channels ), targetValue ( targetValue ), dataValue ( dataValue ), nodataValue ( nodataValue ), touchEdges(touchEdges) {
    if ( memcmp ( targetValue,nodataValue,channels ) ) {
        newNodataValue = true;
    } else {
        // La nouvelle valeur de non-donnée est la même que la couleur cible : on ne change pas la couleur de non-donnée
        newNodataValue = false;
    }

    if ( memcmp ( targetValue,dataValue,channels ) ) {
        // Pour changer la couleur des données contenant la couleur cible, on doit la supprimer complètement puis remettre le nodata à la couleur initiale
        removeTargetValue = true;
        newNodataValue = true;
    } else {
        // La nouvelle valeur de donnée est la même que la couleur cible : on ne supprime donc pas la couleur cible des données
        removeTargetValue = false;
    }
}

bool TiffNodataManager::treatNodata ( char* inputImage, char* outputImage, char* outputMask = 0 ) {
    if ( ! newNodataValue && ! removeTargetValue && ! outputMask) {
        LOGGER_INFO ( "Have nothing to do !" );
        return true;
    }

    uint32 rowsperstrip;
    uint16 bitspersample, photometric, compression , planarconfig, nb_extrasamples;

    TIFF *TIFF_FILE = 0;

    TIFF_FILE = TIFFOpen ( inputImage, "r" );
    if ( !TIFF_FILE ) {
        LOGGER_ERROR ( "Unable to open file for reading: " << inputImage );
        return false;
    }
    if ( ! TIFFGetField ( TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width )                            ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_IMAGELENGTH, &height )                       ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample )              ||
            ! TIFFGetFieldDefaulted ( TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig )       ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric )                  ||
            ! TIFFGetFieldDefaulted ( TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel ) ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_COMPRESSION, &compression )                 ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip ) ) {
        LOGGER_ERROR ( "Error reading file: " << inputImage );
        return false;
    }

    if ( planarconfig != 1 )  {
        LOGGER_ERROR ( "Sorry : only single image plane as planar configuration is handled" );
        return false;
    }
    if ( bitspersample != 8 )  {
        LOGGER_ERROR ( "Sorry : only byte sample are handled" );
        return false;
    }

    if ( samplesperpixel > channels )  {
        LOGGER_ERROR ( "The nodata manager is not adapted (samplesperpixel have to be " << channels <<
                       " or less) for the image " << inputImage << " (" << samplesperpixel << ")" );
        return false;
    }

    /*************** Chargement de l'image ***************/

    uint8_t *IM  = new uint8_t[width * height * samplesperpixel];

    for ( int h = 0; h < height; h++ ) {
        if ( TIFFReadScanline ( TIFF_FILE, IM + width*samplesperpixel*h, h ) == -1 ) {
            LOGGER_ERROR ( "Unable to read line to " + string ( inputImage ) );
            return false;
        }
    }

    TIFFClose ( TIFF_FILE );

    /************* Calcul du masque de données ***********/

    uint8_t *MSK = new uint8_t[width * height];

    identifyNodataPixels(IM, MSK);

    /*************** Modification des pixels *************/

    // 'targetValue' data pixels are replaced by 'dataValue' pixels
    if ( removeTargetValue ) {
        changeDataValue ( IM, MSK );
    }

    // nodata pixels which touch edges are replaced by 'nodataValue' pixels
    if ( newNodataValue ) {
        changeNodataValue ( IM, MSK );
    }

    /**************** Ecriture de l'images ****************/

    TIFF_FILE = TIFFOpen ( outputImage, "w" );
    if ( !TIFF_FILE ) {
        LOGGER_ERROR ( "Unable to open file for writting: " + string ( outputImage ) );
        return false;
    }
    if ( ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGEWIDTH, width )                ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGELENGTH, height )              ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_BITSPERSAMPLE, bitspersample )     ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel ) ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_PHOTOMETRIC, photometric )         ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip )       ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig )       ||
         ! TIFFSetField ( TIFF_FILE, TIFFTAG_COMPRESSION, compression ) ) {
        LOGGER_ERROR ( "Error writting file: " +  string ( outputImage ) );
        return false;
    }

    uint8_t *LINE = new uint8_t[width * samplesperpixel];

    // output image is written
    for ( int h = 0; h < height; h++ ) {
        memcpy ( LINE, IM+h*width*samplesperpixel, width * samplesperpixel );
        if ( TIFFWriteScanline ( TIFF_FILE, LINE, h ) == -1 ) {
            LOGGER_ERROR ( "Unable to write line to " + string ( outputImage ) );
            return false;
        }
    }

    TIFFClose ( TIFF_FILE );

    /**************** Ecriture du masque ? ****************/
    if (outputMask) {
        TIFF_FILE = TIFFOpen ( outputMask, "w" );
        if ( !TIFF_FILE ) {
            LOGGER_ERROR ( "Unable to open file for writting: " + string ( outputMask ) );
            return false;
        }
        if ( ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGEWIDTH, width )                ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGELENGTH, height )              ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_BITSPERSAMPLE, 8 )     ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, 1 ) ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT ) ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK )         ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip )       ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig )       ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE ) ) {
            LOGGER_ERROR ( "Error writting file: " +  string ( outputMask ) );
            return false;
        }

        uint8_t *LINE = new uint8_t[width];

        // output image is written
        for ( int h = 0; h < height; h++ ) {
            memcpy ( LINE, MSK+h*width, width);
            if ( TIFFWriteScanline ( TIFF_FILE, LINE, h ) == -1 ) {
                LOGGER_ERROR ( "Unable to write line to " + string ( outputMask ) );
                return false;
            }
        }

        TIFFClose ( TIFF_FILE );
    }

    
    delete[] IM;
    delete[] MSK;
    delete[] LINE;

    return true;
}

void TiffNodataManager::identifyNodataPixels( uint8_t* IM, uint8_t* MSK ) {
    
    if (touchEdges) {
        // On utilise la couleur targetValue et on part des bords
        
        queue<int> Q;
        memset ( MSK, 255, width * height );

        // Initialisation : we identify front pixels which are lightGray
        for ( int pos = 0; pos < width; pos++ ) { // top
            if ( ! memcmp ( IM + samplesperpixel * pos, targetValue, samplesperpixel ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( int pos = width* ( height-1 ); pos < width*height; pos++ ) { // bottom
            if ( ! memcmp ( IM + samplesperpixel * pos, targetValue, samplesperpixel ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( int pos = 0; pos < width*height; pos += width ) { // left
            if ( ! memcmp ( IM + samplesperpixel * pos, targetValue, samplesperpixel ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( int pos = width -1; pos < width*height; pos+= width ) { // right
            if ( ! memcmp ( IM + samplesperpixel * pos, targetValue, samplesperpixel ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }

        if ( Q.empty() ) {
            // No nodata pixel identified, nothing to do
            return ;
        }

        // while there are 'targetValue' pixels which can propagate, we do it
        while ( !Q.empty() ) {
            int pos = Q.front();
            Q.pop();
            int newpos;
            if ( pos % width > 0 ) {
                newpos = pos - 1;
                if ( MSK[newpos] && ! memcmp ( IM + newpos*samplesperpixel, targetValue, samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos % width < width - 1 ) {
                newpos = pos + 1;
                if ( MSK[newpos] && ! memcmp ( IM + newpos*samplesperpixel, targetValue, samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos / width > 0 ) {
                newpos = pos - width;
                if ( MSK[newpos] && ! memcmp ( IM + newpos*samplesperpixel, targetValue, samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos / width < height - 1 ) {
                newpos = pos + width;
                if ( MSK[newpos] && ! memcmp ( IM + newpos*samplesperpixel, targetValue, samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
        }

    } else {
        // Tous les pixels de la couleur targetValue sont à considérer comme du nodata
        
        for ( int i = 0; i < width * height; i++ ) {
            if ( ! memcmp ( IM+i*samplesperpixel,targetValue,samplesperpixel ) ) {
                MSK[i] = 0;
            }
        }

    }
}


void TiffNodataManager::changeNodataValue ( uint8_t* IM, uint8_t* MSK ) {

    for ( int i = 0; i < width * height; i ++ ) {
        if ( ! MSK[i] ) {
            memcpy ( IM+i*samplesperpixel,nodataValue,samplesperpixel );
        }
    }
}

void TiffNodataManager::changeDataValue ( uint8_t* IM, uint8_t* MSK ) {

    for ( int i = 0; i < width * height; i ++ ) {
        if ( MSK[i] && ! memcmp ( IM+i*samplesperpixel,targetValue,samplesperpixel ) ) {
            memcpy ( IM+i*samplesperpixel,dataValue,samplesperpixel );
        }
    }
}

