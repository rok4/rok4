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
 * Vision libimage : FileImage -> Rok4Image
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
#include "FileImage.h"
#include "Rok4Image.h"
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
 * Usage: tiff2tile -c <VAL> -t <VAL> <VAL> <INPUT FILE> <OUTPUT FILE> [-crop] [-a <VAL> -b <VAL> -s <VAL>]
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
 *      -t tile size : widthwise and heightwise. Have to be a divisor of the global image's size
 *      -crop : blocks (used by JPEG compression) wich contain a white pixel are filled with white
 *      -a sample format : (float or uint)
 *      -b bits per sample : (8 or 32)
 *      -s samples per pixel : (1, 2, 3 or 4)
 *      -d debug logger activation
 *
 * If bitspersample, sampleformat or samplesperpixel is not provided, those 3 informations are read from the image sources (all have to own the same). If 3 are provided, conversion may be done.
 * 
 * Examples
 *      - for orthophotography
 *      tiff2tile input.tif -c png -t 256 256 output.tif
 *      - for DTM
 *      tiff2tile input.tif -c zip -t 256 256 output.tif
 * 
 * \endcode
 */
void usage() {
    LOGGER_INFO ( "\ttiff2tile version " << BE4_VERSION << "\n\n" <<

                  "Make image tiled and compressed, in TIFF format, respecting ROK4 specifications.\n\n" <<

                  "Usage: tiff2tile -c <VAL> -t <VAL> <VAL> <INPUT FILE> <OUTPUT FILE> [-crop]\n\n" <<

                  "Parameters:\n" <<
                  "     -c output compression :\n" <<
                  "             raw     no compression\n" <<
                  "             none    no compression\n" <<
                  "             jpg     Jpeg encoding\n" <<
                  "             lzw     Lempel-Ziv & Welch encoding\n" <<
                  "             pkb     PackBits encoding\n" <<
                  "             zip     Deflate encoding\n" <<
                  "             png     Non-official TIFF compression, each tile is an independant PNG image (with PNG header)\n" <<
                  "     -t tile size : widthwise and heightwise. Have to be a divisor of the global image's size\n" <<
                  "     -crop : blocks (used by JPEG compression) wich contain a white pixel are filled with white\n" <<
                  "     -a sample format : (float or uint)\n" <<
                  "     -b bits per sample : (8 or 32)\n" <<
                  "     -s samples per pixel : (1, 2, 3 or 4)\n" <<
                  "     -d : debug logger activation\n\n" <<

                  "If bitspersample, sampleformat or samplesperpixel is not provided, those 3 informations are read from the image sources (all have to own the same). If 3 are provided, conversion may be done.\n\n" <<

                  "Examples\n" <<
                  "     - for orthophotography\n" <<
                  "     tiff2tile input.tif -c png -t 256 256 output.tif\n" <<
                  "     - for DTM\n" <<
                  "     tiff2tile input.tif -c zip -t 256 256 output.tif\n\n" );
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
    int tileWidth = 256, tileHeight = 256;
    Compression::eCompression compression = Compression::NONE;

    bool outputProvided = false;
    uint16_t samplesperpixel = 0;
    uint16_t bitspersample = 0;
    SampleFormat::eSampleFormat sampleformat = SampleFormat::UNKNOWN;

    Photometric::ePhotometric photometric;

    bool crop = false;
    bool debugLogger=false;

    /* Initialisation des Loggers */
    Logger::setOutput ( STANDARD_OUTPUT_STREAM_FOR_ERRORS );

    Accumulator* acc = new StreamAccumulator();
    Logger::setAccumulator ( INFO , acc );
    Logger::setAccumulator ( WARN , acc );
    Logger::setAccumulator ( ERROR, acc );
    Logger::setAccumulator ( FATAL, acc );

    std::ostream &logw = LOGGER ( WARN );
    logw.precision ( 16 );
    logw.setf ( std::ios::fixed,std::ios::floatfield );

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
                case 'd': // debug logs
                    debugLogger = true;
                    break;
                case 'c': // compression
                    if ( ++i == argc ) { error ( "Error in -c option", -1 ); }
                    if ( strncmp ( argv[i], "none",4 ) == 0 || strncmp ( argv[i], "raw",3 ) == 0 ) {
                        compression = Compression::NONE;
                    } else if ( strncmp ( argv[i], "png",3 ) == 0 ) {
                        compression = Compression::PNG;
                    } else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) {
                        compression = Compression::JPEG;
                    } else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) {
                        compression = Compression::LZW;
                    } else if ( strncmp ( argv[i], "zip",3 ) == 0 ) {
                        compression = Compression::DEFLATE;
                    } else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) {
                        compression = Compression::PACKBITS;
                    } else {
                        error ( "Unknown compression : " + string(argv[i]), -1 );
                    }
                    break;
                case 't':
                    if ( i+2 >= argc ) { error("Error in -t option", -1 ); }
                    tileWidth = atoi ( argv[++i] );
                    tileHeight = atoi ( argv[++i] );
                    break;

                /****************** OPTIONNEL, POUR FORCER DES CONVERSIONS **********************/
                case 's': // samplesperpixel
                    if ( i++ >= argc ) {
                        error ( "Error in option -s", -1 );
                    }
                    if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                    else if ( strncmp ( argv[i], "2",1 ) == 0 ) samplesperpixel = 2 ;
                    else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                    else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                    else {
                        error ( "Unknown value for option -s : " + string(argv[i]), -1 );
                    }
                    break;
                case 'b': // bitspersample
                    if ( i++ >= argc ) {
                        error ( "Error in option -b", -1 );
                    }
                    if ( strncmp ( argv[i], "8",1 ) == 0 ) bitspersample = 8 ;
                    else if ( strncmp ( argv[i], "32",2 ) == 0 ) bitspersample = 32 ;
                    else {
                        error ( "Unknown value for option -b : " + string(argv[i]), -1 );
                    }
                    break;
                case 'a': // sampleformat
                    if ( i++ >= argc ) {
                        error ( "Error in option -a", -1 );
                    }
                    if ( strncmp ( argv[i],"uint",4 ) == 0 ) sampleformat = SampleFormat::UINT ;
                    else if ( strncmp ( argv[i],"float",5 ) == 0 ) sampleformat = SampleFormat::FLOAT;
                    else {
                        error ( "Unknown value for option -a : " + string(argv[i]), -1 );
                    }
                    break;
                /*******************************************************************************/

                default:
                    error ( "Unknown option : " + string(argv[i]) ,-1 );
            }
        } else {
            if ( input == 0 ) input = argv[i];
            else if ( output == 0 ) output = argv[i];
            else { error ( "Argument must specify ONE input file and ONE output file", 2 ); }
        }
    }

    if (debugLogger) {
        // le niveau debug du logger est activé
        Logger::setAccumulator ( DEBUG, acc);
        std::ostream &logd = LOGGER ( DEBUG );
        logd.precision ( 16 );
        logd.setf ( std::ios::fixed,std::ios::floatfield );
    }

    if ( input == 0 || output == 0 ) {
        error ("Argument must specify one input file and one output file", -1);
    }

    FileImageFactory FIF;

    if (crop && compression != Compression::JPEG) {
        LOGGER_WARN("Crop option is reserved for JPEG compression");
        crop = false;
    }

    // For jpeg compression with crop option, we have to remove white pixel, to avoid empty bloc in data
    if ( crop ) {
        LOGGER_DEBUG ( "Open image to read" );
        // On récupère les informations nécessaires pour appeler le nodata manager
        FileImage* tmpSourceImage = FIF.createImageToRead(input);
        int spp = tmpSourceImage->getChannels();
        int bps = tmpSourceImage->getBitsPerSample();
        SampleFormat::eSampleFormat sf = tmpSourceImage->getSampleFormat();
        delete tmpSourceImage;

        if ( bps == 8 && sf == SampleFormat::UINT ) {
            TiffNodataManager<uint8_t> TNM ( spp, white, true, fastWhite,white );
            if ( ! TNM.treatNodata ( input,input ) ) {
                error ( "Unable to treat white pixels in this image : " + string(input), -1 );
            }
        } else {
            LOGGER_WARN( "Crop option ignored (only for 8-bit integer images) for the image : " << input);
        }
    }

    LOGGER_DEBUG ( "Open image to read" );
    FileImage* sourceImage = FIF.createImageToRead(input);
    if (sourceImage == NULL) {
        error("Cannot read the source image", -1);
    }

    // On regarde si on a tout précisé en sortie, pour voir si des conversions sont possibles
    if (sampleformat != SampleFormat::UNKNOWN && bitspersample != 0 && samplesperpixel !=0) {
        outputProvided = true;
        // La photométrie est déduite du nombre de canaux
        if (samplesperpixel == 1) {
            photometric = Photometric::GRAY;
        } else if (samplesperpixel == 2) {
            photometric = Photometric::GRAY;
        } else {
            photometric = Photometric::RGB;
        }

        if (! sourceImage->addConverter ( sampleformat, bitspersample, samplesperpixel ) ) {
            error ( "Cannot add converter to the input FileImage " + string(input), -1 );
        }
    } else {
        // On n'a pas précisé de format de sortie
        // La sortie aura ce format
        bitspersample = sourceImage->getBitsPerSample();
        photometric = sourceImage->getPhotometric();
        sampleformat = sourceImage->getSampleFormat();
        samplesperpixel = sourceImage->getChannels();
    }
    
    if (debugLogger) {
        sourceImage->print();
    }

    Rok4ImageFactory R4IF;
    Rok4Image* rok4Image = R4IF.createRok4ImageToWrite(
        output, BoundingBox<double>(0.,0.,0.,0.), -1, -1, sourceImage->getWidth(), sourceImage->getHeight(), samplesperpixel,
        sampleformat, bitspersample, photometric, compression,
        tileWidth, tileHeight
    );
    
    rok4Image->setExtraSample(sourceImage->getExtraSample());
    
    if (rok4Image == NULL) {
        error("Cannot create the ROK4 image to write", -1);
    }
    
    if (debugLogger) {
        rok4Image->print();
    }

    LOGGER_DEBUG ( "Write" );
    if (rok4Image->writeImage(sourceImage, crop) < 0) {
        error("Cannot write ROK4 image", -1);
    }
    
    LOGGER_DEBUG ( "Clean" );
    // Nettoyage
    delete acc;
    delete sourceImage;
    delete rok4Image;

    return 0;
}
