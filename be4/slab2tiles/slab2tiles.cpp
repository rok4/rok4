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
 * \file slab2tiles.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Conversion d'une dalle fichier en tuiles indépendantes sur Ceph
 *
 * \~ \image html slab2tiles.png \~french
 *
 * Vision libimage : Rok4Image -> Rok4Image
 *
 * \~ \code
 * slab2tiles input.tif -pool test -ij 4 7 -t  16 16 output
 * \endcode
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include "Format.h"
#include "Logger.h"
#include "CephPoolContext.h"
#include "FileContext.h"
#include "SwiftContext.h"
#include "FileImage.h"
#include "Rok4Image.h"
#include "TiffNodataManager.h"
#include "../be4version.h"


/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande slab2tiles
 * \details L'affichage se fait dans le niveau de logger INFO
 * \~ \code
 * slab2tiles version X.Y.Z
 *
 * Convert file slab to independant tiles on ceph cluster.
 *
 * Usage: slab2tiles -pool <VAL> -t <VAL> <VAL> -ij <VAL> <VAL> <INPUT FILE> <OUTPUT PREFIX>
 *
 * Parameters:
 *      -pool : Ceph pool where data is
 *      -t tile size : number of tilewidthwise and tileheightwise in the input slab.
 *      -ij : slab indices (col row)
 *      -d : debug logger activation
 *
 * Examples
 *  slab2tiles input.tif -pool test -ij 4 7 -t 16 16 output
 */
void usage() {
    LOGGER_INFO ( "\tslab2tiles version " << ROK4_VERSION << "\n\n" <<

                  "Convert file slab to independant tiles on ceph cluster.\n\n"<<

                  "Usage: slab2tiles -pool <VAL> -t <VAL> <VAL> -ij <VAL> <VAL> <INPUT FILE> <OUTPUT PREFIX>\n\n"<<

                  "Parameters:\n"<<
                  "     -pool : Ceph pool where data is\n"<<
                  "     -t tile size : number of tilewidthwise and tileheightwise in the input slab.\n"<<
                  "     -ij : slab indices (col row)\n"<<
                  "     -d : debug logger activation\n\n"<<

                  "Example\n"<<
                  "     slab2tiles input.tif -pool test -ij 4 7 -t  16 16 prefix\n");
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
 * \brief Fonction principale de l'outil slab2tiles
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 en cas de succès, -1 sinon
 */
int main ( int argc, char **argv ) {

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

    /************* Récupération des paramètres *************/

    char* input = 0, *output = 0, *pool = 0;
    int tileWidthwise = -1, tileHeightwise = -1;
    int imageCol = -1;
    int imageRow = -1;
    bool debugLogger=false;

    for ( int i = 1; i < argc; i++ ) {
        if ( !strcmp ( argv[i],"-pool" ) ) {
            if ( ++i == argc ) {
                error("Error in -pool option", -1);
            }
            pool = argv[i];
            continue;
        }
        if ( !strcmp ( argv[i],"-ij" ) ) {
            if ( i+2 >= argc ) { error("Error in -ij option", -1 ); }
            imageCol = atoi ( argv[++i] );
            imageRow = atoi ( argv[++i] );
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
                case 't':
                    if ( i+2 >= argc ) { error("Error in -t option", -1 ); }
                    tileWidthwise = atoi ( argv[++i] );
                    tileHeightwise = atoi ( argv[++i] );
                    break;
                default:
                    error ( "Unknown option : " + string(argv[i]) ,-1 );
            }
        } else {
            if ( input == 0 ) input = argv[i];
            else if ( output == 0 ) output = argv[i];
            else { error ( "Argument must specify ONE input file and ONE output file/object", 2 ); }
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
        error ("Argument must specify one input file and one output file/object", -1);
    }

    if ( tileWidthwise == -1 || tileHeightwise == -1 ) {
        error ("Number of tiles in slab must be provided", -1);
    }

    if ( imageCol == -1 || imageRow == -1 ) {
        error ("slab indices must be provided", -1);
    }

    /************* Image en entrée : Rok4Image *************/

    Context* contextInput;
    LOGGER_DEBUG("Input is a file in a file system");
    contextInput = new FileContext("");


    if (! contextInput->connection()) {
        error("Unable to connect context", -1);
    }

    Rok4ImageFactory R4IF;
    Rok4Image* sourceImage = R4IF.createRok4ImageToRead(input, BoundingBox<double>(0.,0.,0.,0.), 0., 0., contextInput);
    if (sourceImage == NULL) {
        delete acc;
        error (std::string("Cannot create ROK4 image to read ") + input, -1);
    }

    if (debugLogger) {
        sourceImage->print();
    }

    /************* En sortie : des objets Ceph écrits sans Image *************/

    Context* contextOutput;

    if ( pool != 0 ) {
        LOGGER_DEBUG( std::string("Output is an object in the Ceph pool ") + pool);
        contextOutput = new CephPoolContext(pool);
    } else {
        delete sourceImage;
        delete contextInput;
        delete acc;
        error ("pool name must be provided", -1);
    }

    if (! contextOutput->connection()) {
        delete sourceImage;
        delete contextInput;
        delete contextOutput;
        delete acc;
        error("Unable to connect context", -1);
    }

    LOGGER_DEBUG ( "Write" );

    int tileColUL = imageCol * tileWidthwise;
    int tileRowUL = imageRow * tileHeightwise;

    uint8_t* tile = new uint8_t[2 * sourceImage->getRawTileSize() ];

    for ( int y = 0; y < tileHeightwise; y++ ) {

        for ( int x = 0; x < tileWidthwise; x++ ) {

            int tileInd = tileWidthwise * y + x;

            char tileName[256];
            sprintf(tileName, "%s_%d_%d", output, tileColUL + x, tileRowUL + y);

            int realSize = sourceImage->getEncodedTile(tile, tileInd);

            if (! contextOutput->openToWrite(std::string(tileName))) {
                LOGGER_ERROR("Unable to open output " << std::string(tileName));
                return -1;
            }
            if (! contextOutput->writeFull(tile, realSize, std::string(tileName))) {
                LOGGER_ERROR("Error writting full tile " << tileColUL + x << "," << tileRowUL + y);
                return -1;
            }
            if (! contextOutput->closeToWrite(std::string(tileName))) {
                LOGGER_ERROR("Unable to close output " << std::string(tileName));
                return -1;
            }
        }
    }

    
    /************* Nettoyage **************/

    LOGGER_DEBUG ( "Clean" );

    delete [] tile;
    delete acc;
    delete sourceImage;
    delete contextInput;
    delete contextOutput;
    return 0;
}