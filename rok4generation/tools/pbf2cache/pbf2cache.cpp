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
 * \file pbf2cache.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Consitution d'une dalle vecteur ROK4 à partir des tuiles PBF
 * \~english \brief Build a vector ROK4 slab from PBD tiles
 */

#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include "Format.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "FileContext.h"
#include "FileImage.h"
#include "CurlPool.h"
#include "Rok4Image.h"
#include "../../../rok4version.h"

#if BUILD_OBJECT
    #include "SwiftContext.h"
    #include "S3Context.h"
    #include "CephPoolContext.h"
#endif

/** \~french Message d'usage de la commande pbf2cache */
std::string help = std::string("\npbf2cache version ") + std::string(ROK4_VERSION) + "\n\n"

    "Make image tiled and compressed, in TIFF format, respecting ROK4 specifications.\n\n"

    "Usage: pbf2cache -r <DIRECTORY> -t <VAL> <VAL> -ultile <VAL> <VAL> <OUTPUT FILE/OBJECT> [-d]\n\n"

    "Parameters:\n"
    "     -r directory containing the PBF tiles : tile I,J is stored to path <DIRECTORY>/I/J.pbf\n"
    "     -t number of tiles in the slab : widthwise and heightwise.\n"
    "     -ultile upper left tile indices\n"
    "     -pool Ceph pool where data is. INPUT FILE is interpreted as a Ceph object (ONLY IF OBJECT COMPILATION)\n"
    "     -container Swift container where data is. Then OUTPUT FILE is interpreted as a Swift object name (ONLY IF OBJECT COMPILATION)\n"
    "     -ks in Swift storage case, activate keystone authentication (ONLY IF OBJECT COMPILATION)\n"
    "     -bucket S3 bucket where data is. Then OUTPUT FILE is interpreted as a S3 object name (ONLY IF OBJECT COMPILATION)\n"
    "     -d debug logger activation\n\n";

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande pbf2cache #help
 * \details L'affichage se fait dans le niveau de logger INFO
 */
void usage() {
    BOOST_LOG_TRIVIAL(info) << help;
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur
 * \param[in] message message d'erreur
 * \param[in] errorCode code de retour
 */
void error ( std::string message, int errorCode ) {
    BOOST_LOG_TRIVIAL(error) <<  message ;
    usage();
    sleep ( 1 );
    exit ( errorCode );
}

/**
 ** \~french
 * \brief Fonction principale de l'outil pbf2cache
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 en cas de succès, -1 sinon
 ** \~english
 * \brief Main function for tool pbf2cache
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return return code, 0 if success, -1 otherwise
 */
int main ( int argc, char **argv ) {

    char* output = 0, *rootDirectory = 0;
    int tilePerWidth = 16, tilePerHeight = 16;
    int ulCol = -1;
    int ulRow = -1;


    bool debugLogger=false;

#if BUILD_OBJECT
    char *pool = 0, *container = 0, *bucket = 0;
    bool onCeph = false;
    bool onSwift = false;
    bool onS3 = false;
    bool keystone = false;
#endif

    /* Initialisation des Loggers */
    boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::info );

    // Récupération des paramètres
    for ( int i = 1; i < argc; i++ ) {

#if BUILD_OBJECT
        if ( !strcmp ( argv[i],"-pool" ) ) {
            if ( ++i == argc ) {
                error("Error in -pool option", -1);
            }
            pool = argv[i];
            continue;
        }
        if ( !strcmp ( argv[i],"-bucket" ) ) {
            if ( ++i == argc ) {
                error("Error in -bucket option", -1);
            }
            bucket = argv[i];
            continue;
        }
        if ( !strcmp ( argv[i],"-container" ) ) {
            if ( ++i == argc ) {
                error("Error in -container option", -1);
            }
            container = argv[i];
            continue;
        }
        if ( !strcmp ( argv[i],"-ks" ) ) {
            keystone = true;
            continue;
        }
#endif
        if ( !strcmp ( argv[i],"-ultile" ) ) {
            if ( i+2 >= argc ) { error("Error in -ultile option", -1 ); }
            ulCol = atoi ( argv[++i] );
            ulRow = atoi ( argv[++i] );
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
                case 'r': // root directory
                    if ( i++ >= argc ) {
                        BOOST_LOG_TRIVIAL(error) <<  "Error in option -r" ;
                        return -1;
                    }
                    rootDirectory = argv[i];
                    break;
                case 't':
                    if ( i+2 >= argc ) { error("Error in -t option", -1 ); }
                    tilePerWidth = atoi ( argv[++i] );
                    tilePerHeight = atoi ( argv[++i] );
                    break;

                default:
                    error ( "Unknown option : " + std::string(argv[i]) ,-1 );
            }
        } else {
            if ( output == 0 ) output = argv[i];
            else { error ( "Argument must specify ONE output file/object", -1 ); }
        }
    }

    if (debugLogger) {
        // le niveau debug du logger est activé
        boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::debug );
    }

    if ( rootDirectory == 0 || output == 0 ) {
        error ("Argument must specify one output file/object and one root directory", -1);
    }

    BOOST_LOG_TRIVIAL(debug) << "Output : " << output;
    BOOST_LOG_TRIVIAL(debug) << "PBF root directory : " << rootDirectory;

    if ( ulRow == -1 || ulCol == -1 ) {
        error ("Upper left tile indices have to be provided (with option -ultile)", -1);
    }

    Context* context;

#if BUILD_OBJECT

    if ( pool != 0 ) {
        onCeph = true;

        BOOST_LOG_TRIVIAL(debug) <<  std::string("Output is an object in the Ceph pool ") + pool;
        context = new CephPoolContext(pool);

    } else if (bucket != 0) {
        onS3 = true;

        curl_global_init(CURL_GLOBAL_ALL);

        BOOST_LOG_TRIVIAL(debug) <<  std::string("Output is an object in the S3 bucket ") + bucket;
        context = new S3Context(bucket);

    } else if (container != 0) {
        onSwift = true;

        curl_global_init(CURL_GLOBAL_ALL);

        BOOST_LOG_TRIVIAL(debug) <<  std::string("Output is an object in the Swift bucket ") + container;
        context = new SwiftContext(container);
    } else {
#endif

        BOOST_LOG_TRIVIAL(debug) << "Output is a file in a file system";
        context = new FileContext("");

#if BUILD_OBJECT
    }  
#endif

    if (! context->connection()) {
        error("Unable to connect context", -1);
    }


    Rok4ImageFactory R4IF;
    Rok4Image* rok4Image = R4IF.createRok4ImageToWrite( output, tilePerWidth, tilePerHeight, context );
    
    if (rok4Image == NULL) {
        error("Cannot create the ROK4 image to write", -1);
    }

    if (debugLogger) {
        rok4Image->print();
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Write" ;

    if (rok4Image->writePbfTiles(ulCol, ulRow, rootDirectory) < 0) {
        error("Cannot write ROK4 image from PBF tiles", -1);
    }

#if BUILD_OBJECT
    if (onSwift || onS3) {
        // Un environnement CURL a été créé et utilisé, il faut le nettoyer
        CurlPool::cleanCurlPool();
        curl_global_cleanup();
    }
#endif

    BOOST_LOG_TRIVIAL(debug) <<  "Clean" ;
    // Nettoyage
    delete rok4Image;
    delete context;

    return 0;
}
