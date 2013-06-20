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
 * \file tiff2tile.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Formattage d'une image, respectant les spécifications d'une pyramide de données ROK4
 * \~english \brief Image formatting, according to ROK4 specifications
 * \~french \details Le serveur ROK4 s'attend à trouver dans une pyramide d'images des images au format TIFF, tuilées, potentiellement compressées, avec une en-tête de taille fixe (2048 octets). C'est cet outil, via l'utilisation de la classe TiledTiffWriter, qui va "mettre au normes" les images.
 *
 * Les compressions disponibles sont :
 * \li Aucune
 * \li JPEG
 * \li Deflate
 * \li PackBits
 * \li LZW
 * \li PNG. Cette compression a la particularité de ne pas être un standard du TIFF. Une image dans ce format, propre à ROK4, contient des tuiles qui sont des images PNG indépendantes, avec les en-têtes PNG. Cela permet de renvoyer sans traitement une tuile au format PNG. Ce fonctionnement est calqué sur le format TIFF/JPEG.
 *
 * On va également définir la taille des tuiles, qui doit être cohérente avec la taille de l'image entière (on veut un nombre de tuiles entier).
 * 
 * \~ \code
 * tiff2tile input.tif -c zip -p rgb -t 100 100 -b 8 -a uint -s 3 output.tif
 * \endcode
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include "Format.h"
#include "Logger.h"
#include "TiffReader.h"
#include "FileImage.h"
#include "TiledTiffWriter.h"
#include "TiffNodataManager.h"
#include "../be4version.h"

/** \~french Presque blanc, en RGBA. Utilisé pour supprimer le blanc pur des données quand l'option "crop" est active */
int fastWhite[4] = {254,254,254,255};
/** \~french Blanc, en RGBA. Utilisé pour supprimer le blanc pur des données quand l'option "crop" est active */
int white[4] = {255,255,255,255};

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande tiff2tile
 * \details L'affichage se fait dans le niveau de logger INFO
 * \~ \code
 * tiff2tile version X.Y.Z
 * 
 * Make image tiled and compressed, in TIFF format, respecting ROK4 specifications.
 * 
 * Usage: tiff2tile -c <VAL> -a <VAL> -s <VAL> -b <VAL> -p <VAL> -t <VAL> <VAL> <FILE> <FILE> [-crop]
 * 
 * Parameters:
 *      -c output compression :
 *              raw     no compression
 *              none    no compression
 *              jpg     Jpeg encoding
 *              lzw     Lempel-Ziv & Welch encoding
 *              pkb     PackBits encoding
 *              zip     Deflate encoding
 *              png     Non-official TIFF compression, each tile is an independant PNG image (with PNG header)
 * 
 *      -p photometric :
 *              gray    min is black
 *              rgb     for image with alpha too
 * 
 *      -a sample format : uint (unsigned integer) or float
 * 
 *      -s samples per pixel : 1, 3 or 4
 * 
 *      -b bits per sample : 8 (for unsigned 8-bit integer) or 32 (for 32-bit float)
 * 
 *      -t tile size : widthwise and heightwise. Have to be a divisor of the global image's size
 * 
 *      -crop : blocks (used by JPEG compression) wich contain a white pixel are filled with white
 * 
 * Examples
 *      - for orthophotography
 *      tiff2tile input.tif -c png -p rgb -t 256 256 -b 8 -a uint -s 3 output.tif
 *      - for DTM
 *      tiff2tile input.tif -c zip -p gray -t 256 256 -b 32 -a float -s 1 output.tif
 * 
 * \endcode
 */
void usage() {
    LOGGER_INFO ( "\ttiff2tile version " << BE4_VERSION << "\n\n" <<

                  "Make image tiled and compressed, in TIFF format, respecting ROK4 specifications.\n\n" <<

                  "Usage: tiff2tile -c <VAL> -a <VAL> -s <VAL> -b <VAL> -p <VAL> -t <VAL> <VAL> <FILE> <FILE> [-crop]\n\n" <<

                  "Parameters:\n" <<
                  "     -c output compression :\n" <<
                  "             raw     no compression\n" <<
                  "             none    no compression\n" <<
                  "             jpg     Jpeg encoding\n" <<
                  "             lzw     Lempel-Ziv & Welch encoding\n" <<
                  "             pkb     PackBits encoding\n" <<
                  "             zip     Deflate encoding\n" <<
                  "             png     Non-official TIFF compression, each tile is an independant PNG image (with PNG header)\n\n" <<

                  "     -p photometric :\n" <<
                  "             gray    min is black\n" <<
                  "             rgb     for image with alpha too\n\n" <<
                  
                  "     -a sample format : uint (unsigned integer) or float\n\n" <<
                  
                  "     -s samples per pixel : 1, 3 or 4\n\n" <<

                  "     -b bits per sample : 8 (for unsigned 8-bit integer) or 32 (for 32-bit float)\n\n" <<
                  
                  "     -t tile size : widthwise and heightwise. Have to be a divisor of the global image's size\n\n" <<

                  "     -crop : blocks (used by JPEG compression) wich contain a white pixel are filled with white\n\n" <<

                  "Examples\n" <<
                  "     - for orthophotography\n" <<
                  "     tiff2tile input.tif -c png -p rgb -t 256 256 -b 8 -a uint -s 3 output.tif\n" <<
                  "     - for DTM\n" <<
                  "     tiff2tile input.tif -c zip -p gray -t 256 256 -b 32 -a float -s 1 output.tif\n\n" );
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur
 * \param[in] message message d'erreur
 * \param[in] errorCode code de retour
 */
void error ( std::string message, int errorCode ) {
    LOGGER_ERROR ( message );
    usage();
    sleep ( 1 );
    exit ( errorCode );
}

/**
 ** \~french
 * \brief Fonction principale de l'outil createNodata
 * \details Tout est contenu dans cette fonction. Le "cropage" se fait grâce à la classe TiffNodataManager, et le tuilage / compression est géré par TiledTiffWriter
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 en cas de succès, -1 sinon
 ** \~english
 * \brief Main function for tool createNodata
 * \details All instrcutions are in this function. the crop is handled by the class TiffNodataManager and TiledTiffWriter make image tiled and compressed.
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return return code, 0 if success, -1 otherwise
 */
int main ( int argc, char **argv ) {

    char* input = 0, *output = 0;
    uint32_t tilewidth = 256, tilelength = 256;
    uint16_t compression = COMPRESSION_NONE;
    uint16_t photometric = PHOTOMETRIC_RGB;
    uint32_t bitspersample = 0;
    uint16_t samplesperpixel = 0;
    bool crop = false;
    uint16_t sampleformat = SAMPLEFORMAT_UINT; // Autre possibilite : SAMPLEFORMAT_IEEEFP
    int quality = -1;

    /* Initialisation des Loggers */
    Logger::setOutput ( STANDARD_OUTPUT_STREAM_FOR_ERRORS );

    Accumulator* acc = new StreamAccumulator();
    //Logger::setAccumulator ( DEBUG, acc );
    Logger::setAccumulator ( INFO , acc );
    Logger::setAccumulator ( WARN , acc );
    Logger::setAccumulator ( ERROR, acc );
    Logger::setAccumulator ( FATAL, acc );

    std::ostream &logd = LOGGER ( DEBUG );
    logd.precision ( 16 );
    logd.setf ( std::ios::fixed,std::ios::floatfield );

    std::ostream &logw = LOGGER ( WARN );
    logw.precision ( 16 );
    logw.setf ( std::ios::fixed,std::ios::floatfield );

    LOGGER_DEBUG ( "Read parameters" );
    // Récupération des paramètres
    for ( int i = 1; i < argc; i++ ) {
        if ( !strcmp ( argv[i],"-crop" ) ) {
            crop = true;
            continue;
        }
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
            case 'c': // compression
                if ( ++i == argc ) { error ( "Error in -c option", 2 ); }
                if ( strncmp ( argv[i], "none",4 ) == 0 || strncmp ( argv[i], "raw",3 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "png",3 ) == 0 ) {
                    compression = COMPRESSION_PNG;
                    if ( argv[i][3] == ':' ) quality = atoi ( argv[i]+4 );
                } else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) {
                    compression = COMPRESSION_JPEG;
                    if ( argv[i][4] == ':' ) quality = atoi ( argv[i]+5 );
                } else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) {
                    compression = COMPRESSION_LZW;
                } else if ( strncmp ( argv[i], "zip",3 ) == 0 ) {
                    compression = COMPRESSION_DEFLATE;
                } else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) {
                    compression = COMPRESSION_PACKBITS;
                } else { error("Error : unknown compression (" + string(argv[i]) + ")", 2); }
                break;
            case 'p': // photometric
                if ( ++i == argc ) { error("Error in -p option", 2 ); }
                if ( strncmp ( argv[i], "gray",4 ) == 0 ) photometric = PHOTOMETRIC_MINISBLACK;
                else if ( strncmp ( argv[i], "rgb",3 ) == 0 ) photometric = PHOTOMETRIC_RGB;
                else { error("Error : unknown photometric (" + string(argv[i]) + ")", 2); }
                break;
            case 't':
                if ( i+2 >= argc ) { error("Error in -t option", 2 ); }
                tilewidth = atoi ( argv[++i] );
                tilelength = atoi ( argv[++i] );
                break;
            case 'a':
                if ( ++i == argc ) { error( "Error in -a option", 2 ); }
                if ( strncmp ( argv[i],"uint",4 ) ==0 ) {
                    sampleformat = SAMPLEFORMAT_UINT;
                } else if ( strncmp ( argv[i],"float",5 ) ==0 ) {
                    sampleformat = SAMPLEFORMAT_IEEEFP;
                } else { error ( "Error in -a option. Possibilities are uint or float.", 2 ); }
                break;
            case 's': // samplesperpixel
                if ( ++i == argc ) { error ( "Error in -s option", 2 ); }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                else { error ( "Error in -s option. Possibilities are 1,3 or 4.", 2 ); }
                break;
            case 'b':
                if ( i+1 >= argc ) { error ( "Error in -b option", 2 ); }
                bitspersample = atoi ( argv[++i] );
                break;
            default:
                error ( "Unknown option : -" + argv[i][1] ,-1 );
            }
        } else {
            if ( input == 0 ) input = argv[i];
            else if ( output == 0 ) output = argv[i];
            else { error ( "Argument must specify ONE input file and ONE output file", 2 ); }
        }
    }

    if ( output == 0 ) {
        error ( "Argument must specify one input file and one output file", 2 );
    }
    if ( bitspersample == 0 ) {
        error ( "Number of bits per sample have to be precised (option -b)", 2 );
    }
    if ( samplesperpixel == 0 ) {
        error ( "Number of samples per pixel have to be precised (option -s)", 2 );
    }
    if ( photometric == PHOTOMETRIC_MINISBLACK && compression == COMPRESSION_JPEG ) {
        error ( "Gray jpeg not supported", 2 );
    }

    if ( ! ( bitspersample == 32 && sampleformat == SAMPLEFORMAT_IEEEFP ) && ! ( bitspersample == 8 && sampleformat == SAMPLEFORMAT_UINT ) ) {
        error ( "Unknown sample type (sample format + bits per sample)", 2 );
    }

    // For jpeg compression with crop option, we have to remove white pixel, to avoid empty bloc in data
    if ( crop ) {

        if ( bitspersample == 8 && sampleformat == SAMPLEFORMAT_UINT ) {
            TiffNodataManager<uint8_t> TNM ( samplesperpixel,white, true, fastWhite,white );
            if ( ! TNM.treatNodata ( input,input ) ) {
                error ( "Unable to treat white pixels in this image : " + std::string(input), 2 );
            }
        } else if ( bitspersample == 32 && sampleformat == SAMPLEFORMAT_IEEEFP ) {
            LOGGER_WARN( "No crop for the float image : " + std::string(input));
        }

    }

    FileImageFactory FIF;
    FileImage* sourceImage = FIF.createImageToRead(input);

    int width = sourceImage->getWidth();
    int length = sourceImage->getHeight();

    if ( width % tilewidth || length % tilelength ) { error ( "Image size must be a multiple of tile size", 2 ); }

    TiledTiffWriter W ( output, width, length, photometric, compression, quality, tilewidth, tilelength,bitspersample,samplesperpixel,sampleformat );

    int tilex = width / tilewidth;
    int tiley = length / tilelength;

    size_t dataSize = tilelength*tilewidth*sourceImage->getPixelByteSize();
    uint8_t* data = new uint8_t[dataSize];

    for ( int y = 0; y < tiley; y++ ) {
        for ( int x = 0; x < tilex; x++ ) {
            sourceImage->getTile ( x*tilewidth, y*tilelength, tilewidth, tilelength, data );
            if ( W.WriteTile ( x, y, data, crop ) < 0 ) {
                std::stringstream sstm;
                sstm << x << "," << y;
                error ( "Error while writting tile (" + sstm.str() + ")", 2);
            }
        }
    }

    if ( W.close() < 0 ) { error ( "Error while writting index", 2); }
    
    LOGGER_DEBUG ( "Clean" );
    // Nettoyage
    delete acc;
    delete sourceImage;

    return 0;
}
