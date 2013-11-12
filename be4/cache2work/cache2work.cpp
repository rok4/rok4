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
 * \file cache2work.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Convertit une image d'une pyramide ROK4 (TIFF tuilé) en une image TIFF, lisible avec LibtiffImage
 * \~french \details Certains formats sont propres aux spécifications d'une pyramide d'images ROK4 (format PNG).
 * \~english \brief Convert a ROK4 pyramide's image (tiled TIFF) to a TIFF image, readable with LibtiffImage
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include "Format.h"
#include "Logger.h"
#include "Rok4Image.h"
#include "FileImage.h"
#include "../be4version.h"

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande cache2work
 * \details L'affichage se fait dans la sortie d'erreur
 * \~ \code
 * cache2work version X.X.X
 * 
 * Convert a ROK4 pyramid's TIFF image to untiled TIFF image
 * 
 * Usage: cache2work <INPUT FILE> [-c <VAL>] <OUTPUT FILE>
 * 
 * Parameters:
 *      -c output compression : default value : none
 *              raw     no compression
 *              none    no compression
 *              jpg     Jpeg encoding
 *              lzw     Lempel-Ziv & Welch encoding
 *              pkb     PackBits encoding
 *              zip     Deflate encoding
 *     -d debug logger activation
 * 
 * Example
 *      createNodata JpegTiled.tif -c zip ZipUntiled.tif
 * \endcode
 */
void usage() {

    LOGGER_INFO ( "\ncache2work version " << BE4_VERSION << "\n\n" <<

                  "Convert a ROK4 pyramid's TIFF image to untiled TIFF image\n\n" <<

                  "Usage: cache2work <INPUT FILE> [-c <VAL>] <OUTPUT FILE>\n\n" <<

                  "Parameters:\n" <<
                  "     -c output compression : default value : none\n" <<
                  "             raw     no compression\n" <<
                  "             none    no compression\n" <<
                  "             jpg     Jpeg encoding\n" <<
                  "             lzw     Lempel-Ziv & Welch encoding\n" <<
                  "             pkb     PackBits encoding\n" <<
                  "             zip     Deflate encoding\n" <<
                  "    -d debug logger activation\n\n" <<

                  "Example\n" <<
                  "     cache2work JpegTiled.tif -c zip ZipUntiled.tif\n" );
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
 * \details Tout est contenu dans cette fonction.
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 en cas de succès, -1 sinon
 ** \~english
 * \brief Main function for tool createNodata
 * \details All instructions are in this function.
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return return code, 0 if success, -1 otherwise
 */
int main ( int argc, char **argv )
{
    char* input = 0, *output = 0;
    Compression::eCompression compression = Compression::NONE;
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

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
                break;
            case 'd': // debug logs
                debugLogger = true;
                break;
            case 'c': // compression
                if ( ++i == argc ) {
                    error("Error in -c option", -1);
                }
                if ( strncmp ( argv[i], "none",4 ) == 0 || strncmp ( argv[i], "raw",3 ) == 0 ) {
                    compression = Compression::NONE;
                } else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) {
                    compression = Compression::JPEG;
                } else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) {
                    compression = Compression::LZW;
                } else if ( strncmp ( argv[i], "zip",3 ) == 0 ) {
                    compression = Compression::DEFLATE;
                } else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) {
                    compression = Compression::PACKBITS;
                } else {
                    error ( "Unknown compression : " + argv[i][1], -1 );
                }
                break;
            default:
                error ( "Unknown option : -" + argv[i][1] ,-1 );
            }
        } else {
            if ( input == 0 ) input = argv[i];
            else if ( output == 0 ) output = argv[i];
            else {
                error ("Argument must specify ONE input file and ONE output file", -1);
            }
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

    Rok4ImageFactory R4IF;
    Rok4Image* rok4image = R4IF.createRok4ImageToRead(input, BoundingBox<double>(0.,0.,0.,0.), 0., 0.);
    if (rok4image == NULL) {
        error (std::string("Cannot create ROK4 image to read ") + input, -1);
    }

    FileImageFactory FIF;
    FileImage* outputImage = FIF.createImageToWrite(
        output, rok4image->getBbox(), rok4image->getResX(), rok4image->getResY(), rok4image->getWidth(), rok4image->getHeight(),
        rok4image->channels, rok4image->getSampleFormat(), rok4image->getBitsPerSample(), rok4image->getPhotometric(), compression
    );

    if (outputImage == NULL) {
        error (std::string("Cannot create image to write ") + output, -1);
    }
    
    LOGGER_DEBUG ( "Write" );
    if (outputImage->writeImage(rok4image) < 0) {
        error("Cannot write image", -1);
    }

    delete rok4image;
    delete outputImage;

    return 0;
}
