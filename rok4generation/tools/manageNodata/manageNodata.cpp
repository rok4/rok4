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
 * \file manageNodata.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gère la couleur des pixels de nodata
 * \~english \brief Manage nodata pixel color
 * \~french \details Cet outil est destiné à :
 * \li identifier les pixels de nodata à partir d'une valeur et d'une tolérance
 * \li modifier les pixels qui contiennent cette valeur
 * \li écrire le masque de données associé à l'image
 *
 * L'outil gère les images à canaux entiers non signés sur 8 bits ou flottant sur 32 bits.
 *
 * Les paramètres sont les suivants :
 *
 * \li l'image en entrée (obligatoire)
 * \li l'image en sortie, au format TIFF. Si on ne la précise pas, les éventuelles modifications de l'image écraseront l'image source.
 * \li le masque de sortie, au format TIFF. (pas écrit si non précisé).
 *
 * Dans le cas où seul le masque associé nous intéresse, on ne réecrira jamais de nouvelle image, même si un chemin de sortie différent de l'entrée était précisé.
 *
 * Si l'image traitée ne contient pas de nodata, le masque n'est pas écrit. En effet, on considère une image sans masque associé comme une image pleine.
 *
 * On peut également définir 3 couleurs :
 * \li la couleur cible (obligatoire) : les pixels de cette couleur sont ceux potentiellement considérés comme du nodata et modifiés. On peut également préciser une tolérance en complément de la valeur. L'option "touche les bords" précise la façon dont on identifie les pixels de nodata.
 * \li la nouvelle couleur de nodata : si elle n'est pas précisée, cela veut dire qu'on ne veut pas la modifier
 * \li la nouvelle couleur de donnée : si elle n'est pas précisée, cela veut dire qu'on ne veut pas la modifier
 *
 * \~ \image html manageNodata.png \~french
 *
 * Cet outil n'est qu'une interface permettant l'utilisation de la classe TiffNodataManager, qui réalise réellement tous les traitements.
 *
 * Le nombre de canaux du fichier en entrée et les valeurs de nodata renseignée doivent être cohérents.
 */

using namespace std;

#include "TiffNodataManager.h"
#include "Format.h"
#include "Logger.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "../../../rok4version.h"

/** \~french Message d'usage de la commande manageNodata */
std::string help = std::string("\nmanageNodata version ") + std::string(ROK4_VERSION) + "\n\n"

        "Manage nodata pixel color in a TIFF file, byte samples\n\n"

        "Usage: manageNodata -target <VAL> [-tolerance <VAL>] [-touch-edges] -format <VAL> [-nodata <VAL>] [-data <VAL>] <INPUT FILE> [<OUTPUT FILE>] [-mask-out <VAL>]\n\n"

        "Colors are provided in decimal format, one integer value per sample\n"
        "Parameters:\n"
        "      -target         color to consider as nodata / modify\n"
        "      -tolerance      a positive integer, to define a delta for target value's comparison\n"
        "      -touche-edges   method to identify nodata pixels (all 'target value' pixels or just those at the edges\n"
        "      -data           new color for data pixel which contained target color\n"
        "      -nodata         new color for nodata pixel\n"
        "      -mask-out       path to the mask to write\n"
        "      -format         image's samples' format : uint8 or float32\n"
        "      -channels       samples per pixel,number of samples in provided colors\n"
        "      -d              debug logger activation\n\n"

        "Examples :\n"
        "      - to keep pure white for nodata, and write a new image :\n"
        "              manageNodata -target 255,255,255 -touch-edges -data 254,254,254 input_image.tif output_image.tif -channels 3 -format uint8\n"
        "      - to write the associated mask (all '-99999' pixels are nodata, with a tolerance):\n"
        "              manageNodata -target -99999 -tolerance 10 input_image.tif -mask-out mask.tif -channels 1 -format float32\n\n";
    
/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande manageNodata #help
 * \details L'affichage se fait dans la sortie d'erreur
 */
void usage() {
    LOGGER_INFO (help);
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur
 * \param[in] message message d'erreur
 * \param[in] errorCode code de retour, -1 par défaut
 */
void error ( std::string message, int errorCode = -1 ) {
    LOGGER_ERROR ( message );
    usage();
    sleep ( 1 );
    exit ( errorCode );
}

/**
 ** \~french
 * \brief Fonction principale de l'outil manageNodata
 * \details Tout est contenu dans cette fonction.
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 en cas de succès, -1 sinon
 ** \~english
 * \brief Main function for tool manageNodata
 * \details All instructions are in this function.
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return return code, 0 if success, -1 otherwise
 */
int main ( int argc, char* argv[] ) {
    char* inputImage = 0;
    char* outputImage = 0;

    char* outputMask = 0;

    char* strTargetValue = 0;
    char* strNewNodata = 0;
    char* strNewData = 0;

    int channels = 0;
    uint16_t bitspersample = 0, sampleformat = 0;

    bool touchEdges = false;
    int tolerance = 0;
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

        if ( !strcmp ( argv[i],"-h" ) ) {
            usage();
            exit ( 0 ) ;
        }
        
        if ( !strcmp ( argv[i],"-d" ) ) { // debug logs
            debugLogger = true;
            break;
        }

        if ( !strcmp ( argv[i],"-touch-edges" ) ) {
            touchEdges = true;
            continue;

        } else if ( !strcmp ( argv[i],"-tolerance" ) ) {
            if ( i++ >= argc ) error ( "Error with option -tolerance",-1 );
            tolerance = atoi ( argv[i] );
            if ( tolerance < 0 ) error ( "Error with option -tolerance : have to be a positive integer",-1 );
            continue;

        } else if ( !strcmp ( argv[i],"-target" ) ) {
            if ( i++ >= argc ) error ( "Error with option -target",-1 );
            strTargetValue = argv[i];
            continue;

        } else if ( !strcmp ( argv[i],"-nodata" ) ) {
            if ( i++ >= argc ) error ( "Error with option -nodata",-1 );
            strNewNodata = argv[i];
            continue;

        } else if ( !strcmp ( argv[i],"-data" ) ) {
            if ( i++ >= argc ) error ( "Error with option -data",-1 );
            strNewData = argv[i];
            continue;

        } else if ( !strcmp ( argv[i],"-format" ) ) {
            if ( i++ >= argc ) error ( "Error with option -format",-1 );
            if ( strncmp ( argv[i], "uint8", 5 ) == 0 ) {
                bitspersample = 8;
                sampleformat = SampleFormat::UINT;
            } else if ( strncmp ( argv[i], "float32", 7 ) == 0 ) {
                bitspersample = 32;
                sampleformat = SampleFormat::FLOAT;
            } else error ( "Unknown value for option -format : " + string ( argv[i] ), -1 );
            continue;

        } else if ( !strcmp ( argv[i],"-channels" ) ) {
            if ( i++ >= argc ) error ( "Error with option -channels",-1 );
            channels = atoi ( argv[i] );
            continue;

        } else if ( !strcmp ( argv[i],"-mask-out" ) ) {
            if ( i++ >= argc ) error ( "Error with option -mask-out",-1 );
            outputMask = argv[i];
            continue;

        } else if ( !inputImage ) {
            inputImage = argv[i];

        } else if ( !outputImage ) {
            outputImage = argv[i];

        } else {
            error ( "Error : unknown option : " + string ( argv[i] ),-1 );
        }
    }

    if (debugLogger) {
        // le niveau debug du logger est activé
        Logger::setAccumulator ( DEBUG, acc);
        std::ostream &logd = LOGGER ( DEBUG );
        logd.precision ( 16 );
        logd.setf ( std::ios::fixed,std::ios::floatfield );
    }

    /***************** VERIFICATION DES PARAMETRES FOURNIS *********************/

    if ( ! inputImage ) error ( "Missing input file",-1 );

    if ( ! outputImage ) {
        LOGGER_INFO ( "If the input image have to be modify, it will be overwrite" );
        outputImage = new char[sizeof ( inputImage ) +1];
        memcpy ( outputImage, inputImage, sizeof ( inputImage ) );
    }

    if ( ! channels ) error ( "Missing number of samples per pixel",-1 );
    if ( ! bitspersample ) error ( "Missing sample format",-1 );

    if ( ! strTargetValue )
        error ( "How to identify the nodata in the input image ? Provide a target color (-target)",-1 );

    if ( ! strNewNodata && ! strNewData && ! outputMask )
        error ( "What have we to do with the target color ? Precise a new nodata or data color, or a mask to write",-1 );

    int* targetValue = new int[channels];
    int* newNodata = new int[channels];
    int* newData = new int[channels];

    /***************** INTERPRETATION DES COULEURS FOURNIES ********************/
    LOGGER_DEBUG ( "Color interpretation" );

    // Target value
    char* charValue = strtok ( strTargetValue,"," );
    if ( charValue == NULL ) {
        error ( "Error with option -target : integer values seperated by comma",-1 );
    }
    targetValue[0] = atoi ( charValue );
    for ( int i = 1; i < channels; i++ ) {
        charValue = strtok ( NULL, "," );
        if ( charValue == NULL ) {
            error ( "Error with option -oldValue : integer values seperated by comma",-1 );
        }
        targetValue[i] = atoi ( charValue );
    }

    // New nodata
    if ( strNewNodata ) {
        charValue = strtok ( strNewNodata,"," );
        if ( charValue == NULL ) {
            error ( "Error with option -nodata : integer values seperated by comma",-1 );
        }
        newNodata[0] = atoi ( charValue );
        for ( int i = 1; i < channels; i++ ) {
            charValue = strtok ( NULL, "," );
            if ( charValue == NULL ) {
                error ( "Error with option -nodata : integer values seperated by comma",-1 );
            }
            newNodata[i] = atoi ( charValue );
        }
    } else {
        // On ne précise pas de nouvelle couleur de non-donnée, elle est la même que la couleur cible.
        memcpy ( newNodata, targetValue, channels*sizeof ( int ) );
    }

    // New data
    if ( strNewData ) {
        charValue = strtok ( strNewData,"," );
        if ( charValue == NULL ) {
            error ( "Error with option -data : integer values seperated by comma",-1 );
        }
        newData[0] = atoi ( charValue );
        for ( int i = 1; i < channels; i++ ) {
            charValue = strtok ( NULL, "," );
            if ( charValue == NULL ) {
                error ( "Error with option -data : integer values seperated by comma",-1 );
            }
            newData[i] = atoi ( charValue );
        }
    } else {
        // Pas de nouvelle couleur pour la donnée : elle a la valeur de la couleur cible
        memcpy ( newData, targetValue, channels*sizeof ( int ) );
    }

    /******************* APPEL A LA CLASSE TIFFNODATAMANAGER *******************/

    if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        LOGGER_DEBUG ( "Target color treatment (uint8)" );
        TiffNodataManager<float> TNM ( channels, targetValue, touchEdges, newData, newNodata, tolerance );
        if ( ! TNM.treatNodata ( inputImage, outputImage, outputMask ) ) {
            error ( "Error : unable to treat nodata for this 8-bit integer image : " + string ( inputImage ), -1 );
        }
    } else if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        LOGGER_DEBUG ( "Target color treatment (float)" );
        TiffNodataManager<uint8_t> TNM ( channels, targetValue, touchEdges, newData, newNodata, tolerance );
        if ( ! TNM.treatNodata ( inputImage, outputImage, outputMask ) ) {
            error ( "Error : unable to treat nodata for this 32-bit float image : " + string ( inputImage ), -1 );
        }
    }

    LOGGER_DEBUG ( "Clean" );
    Logger::stopLogger();
    if ( acc ) {
        delete acc;
    }
    delete[] targetValue;
    delete[] newData;
    delete[] newNodata;

    return 0;
}
