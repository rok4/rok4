/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file overlayNtiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Fusion de N images aux mêmes dimensions, selon différentes méthodes
 * \~english \brief Merge N images with same dimensions, according to different merge methods
 */

#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "tiffio.h"
#include "tiff.h"

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
namespace logging = boost::log;
namespace keywords = boost::log::keywords;

#include "LibtiffImage.h"
#include "MergeImage.h"
#include "Format.h"
#include "math.h"
#include "../../../rok4version.h"

/** \~french Chemin du fichier de configuration des images */
char imageListFilename[256];
/** \~french Nombre de canaux par pixel de l'image en sortie */
uint16_t samplesperpixel = 0;
/** \~french Nombre de bits occupé par un canal */
uint16_t bitspersample;
/** \~french Format du canal (entier, flottant, signé ou non...), dans les images en entrée et celle en sortie */
SampleFormat::eSampleFormat sampleformat;
/** \~french Photométrie (rgb, gray), pour les images en sortie */
Photometric::ePhotometric photometric = Photometric::RGB;
/** \~french Compression de l'image de sortie */
Compression::eCompression compression = Compression::NONE;
/** \~french Mode de fusion des images */
Merge::eMergeType mergeMethod = Merge::UNKNOWN;

/** \~french Couleur à considérer comme transparent dans le images en entrée. Vrai couleur (sur 3 canaux). Peut ne pas être définie */
int* transparent;
/** \~french Couleur à utiliser comme fond. Doit comporter autant de valeur qu'on veut de canaux dans l'image finale. Si un canal alpha est présent, il ne doit pas être prémultiplié aux autres canaux */
int* background;

/** \~french Activation du niveau de log debug. Faux par défaut */
bool debugLogger=false;


/** \~french Message d'usage de la commande overlayNtiff */
std::string help = std::string("\noverlayNtiff version ") + std::string(ROK4_VERSION) + "\n\n"

    "Create one TIFF image, from several images with same dimensions, with different available merge methods.\n"
    "Sources and output image can have different numbers of samples per pixel. The sample type have to be the same for all sources and will be the output one\n\n"

    "Usage: overlayNtiff -f <FILE> -m <VAL> -c <VAL> -s <VAL> -p <VAL [-n <VAL>] -b <VAL>\n"

    "Parameters:\n"
    "    -f configuration file : list of output and source images and masks\n"
    "    -c output compression :\n"
    "            raw     no compression\n"
    "            none    no compression\n"
    "            jpg     Jpeg encoding (rate 75)\n"
    "            jpg90   Jpeg encoding (rate 90)\n"
    "            lzw     Lempel-Ziv & Welch encoding\n"
    "            pkb     PackBits encoding\n"
    "            zip     Deflate encoding\n"
    "    -t value to consider as transparent, 3 integers, separated with comma. Optionnal\n"
    "    -b value to use as background, one integer per output sample, separated with comma\n"
    "    -m merge method : used to merge input images, associated masks are always used if provided :\n"
    "            ALPHATOP       images are merged by alpha blending\n"
    "            MULTIPLY       samples are multiplied one by one\n"
    "            TOP            only the top data pixel is kept\n"
    "    -s output samples per pixel : 1, 2, 3 or 4\n"
    "    -p output photometric :\n"
    "            gray    min is black\n"
    "            rgb     for image with alpha too\n"
    "    -d debug logger activation\n\n"

    "Examples\n"
    "    - for gray orthophotography, with transparency (white is transparent)\n"
    "    overlayNtiff -f conf.txt -m ALPHATOP -s 1 -c zip -p gray -t 255,255,255 -b 0\n"
    "    - for DTM, considering masks only\n"
    "    overlayNtiff -f conf.txt -m TOP -s 1 -c zip -p gray -b -99999\n\n";

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande overlayNtiff #help
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
    BOOST_LOG_TRIVIAL(error) <<  "Configuration file : " << imageListFilename ;
    usage();
    sleep ( 1 );
    exit ( errorCode );
}

/**
 * \~french
 * \brief Récupère les valeurs passées en paramètres de la commande, et les stocke dans les variables globales
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 si réussi, -1 sinon
 */
int parseCommandLine ( int argc, char** argv ) {

    char strTransparent[256];
    memset ( strTransparent, 0, 256 );
    char strBg[256];
    memset ( strBg, 0, 256 );

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
            case 'd': // debug logs
                debugLogger = true;
                break;
            case 'f': // Images' list file
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with images' list file (option -f)" ;
                    return -1;
                }
                strcpy ( imageListFilename,argv[i] );
                break;
            case 'm': // image merge method
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with merge method (option -m)" ;
                    return -1;
                }
                mergeMethod = Merge::fromString ( argv[i] );
                if ( mergeMethod == Merge::UNKNOWN ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for merge method (option -m) : " << argv[i] ;
                    return -1;
                }
                break;
            case 's': // samplesperpixel
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with samples per pixel (option -s)" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                else if ( strncmp ( argv[i], "2",1 ) == 0 ) samplesperpixel = 2 ;
                else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for samples per pixel (option -s) : " << argv[i] ;
                    return -1;
                }
                break;
            case 'c': // compression
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with compression (option -c)" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = Compression::DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = Compression::PACKBITS;
                else if ( strncmp ( argv[i], "jpg90",5 ) == 0 ) compression = Compression::JPEG90;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = Compression::JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = Compression::LZW;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for compression (option -c) : " << argv[i] ;
                    return -1;
                }
                break;
            case 'p': // photometric
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with photometric (option -p)" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "gray",4 ) == 0 ) photometric = Photometric::GRAY;
                else if ( strncmp ( argv[i], "rgb",3 ) == 0 ) photometric = Photometric::RGB;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for photometric (option -p) : " << argv[i] ;
                    return -1;
                }
                break;
            case 't': // transparent color
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with transparent color (option -t)" ;
                    return -1;
                }
                strcpy ( strTransparent,argv[i] );
                break;
            case 'b': // background color
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error with background color (option -b)" ;
                    return -1;
                }
                strcpy ( strBg,argv[i] );
                break;
            default:
                BOOST_LOG_TRIVIAL(error) <<  "Unknown option : -" << argv[i][1] ;
                return -1;
            }
        }
    }

    // Merge method control
    if ( mergeMethod == Merge::UNKNOWN ) {
        BOOST_LOG_TRIVIAL(error) <<  "We need to know the merge method (option -m)" ;
        return -1;
    }

    // Image list file control
    if ( strlen ( imageListFilename ) == 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "We need to have one images' list (text file, option -f)" ;
        return -1;
    }

    // Samples per pixel control
    if ( samplesperpixel == 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "We need to know the number of samples per pixel in the output image (option -s)" ;
        return -1;
    }

    if (mergeMethod == Merge::ALPHATOP && strlen ( strTransparent ) != 0 ) {
        transparent = new int[3];

        // Transparent interpretation
        char* charValue = strtok ( strTransparent,"," );
        if ( charValue == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Error with option -t : 3 integers values separated by comma" ;
            return -1;
        }
        int value = atoi ( charValue );
        transparent[0] = value;
        for ( int i = 1; i < 3; i++ ) {
            charValue = strtok ( NULL, "," );
            if ( charValue == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  "Error with option -t : 3 integers values separated by comma" ;
                return -1;
            }
            value = atoi ( charValue );
            transparent[i] = value;
        }
    }

    if ( strlen ( strBg ) != 0 ) {
        background = new int[samplesperpixel];

        // Background interpretation
        char* charValue = strtok ( strBg,"," );
        if ( charValue == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Error with option -b : one integer value per final sample separated by comma" ;
            return -1;
        }
        int value = atoi ( charValue );
        background[0] = value;

        for ( int i = 1; i < samplesperpixel; i++ ) {
            charValue = strtok ( NULL, "," );
            if ( charValue == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  "Error with option -b : one integer value per final sample separated by comma" ;
                return -1;
            }
            value = atoi ( charValue );
            background[i] = value;
        }

    } else {
        BOOST_LOG_TRIVIAL(error) <<  "We need to know the background value for the output image (option -b)" ;
        return -1;
    }

    return 0;
}

/**
 * \~french
 * \brief Lit une ligne du fichier de configuration
 * \details Une ligne contient le chemin vers une image, potentiellement suivi du chemin vers le masque associé.
 * \param[in,out] file flux de lecture vers le fichier de configuration
 * \param[out] imageFileName chemin de l'image lu dans le fichier de configuration
 * \param[out] hasMask précise si l'image possède un masque
 * \param[out] maskFileName chemin du masque lu dans le fichier de configuration
 * \return code de retour, 0 en cas de succès, -1 si la fin du fichier est atteinte, 1 en cas d'erreur
 */
int readFileLine ( std::ifstream& file, char* imageFileName, bool* hasMask, char* maskFileName ) {
    std::string str;

    while ( str.empty() ) {
        if ( file.eof() ) {
            BOOST_LOG_TRIVIAL(debug) <<  "Configuration file end reached" ;
            return -1;
        }
        std::getline ( file,str );
    }

    if ( std::sscanf ( str.c_str(),"%s %s", imageFileName, maskFileName ) == 2 ) {
        *hasMask = true;
    } else {
        *hasMask = false;
    }

    return 0;
}

/**
 * \~french
 * \brief Charge les images en entrée et en sortie depuis le fichier de configuration
 * \details On va récupérer toutes les informations de toutes les images et masques présents dans le fichier de configuration et créer les objets LibtiffImage correspondant. Toutes les images ici manipulées sont de vraies images (physiques) dans ce sens où elles sont des fichiers soit lus, soit qui seront écrits.
 *
 * \param[out] ppImageOut image résultante de l'outil
 * \param[out] ppMaskOut masque résultat de l'outil, si demandé
 * \param[out] pImageIn ensemble des images en entrée
 * \return code de retour, 0 si réussi, -1 sinon
 */
int loadImages ( FileImage** ppImageOut, FileImage** ppMaskOut, MergeImage** ppMergeIn ) {
    char inputImagePath[IMAGE_MAX_FILENAME_LENGTH];
    char inputMaskPath[IMAGE_MAX_FILENAME_LENGTH];

    char outputImagePath[IMAGE_MAX_FILENAME_LENGTH];
    char outputMaskPath[IMAGE_MAX_FILENAME_LENGTH];

    std::vector<Image*> ImageIn;
    BoundingBox<double> fakeBbox ( 0.,0.,0.,0. );

    int width, height;

    bool hasMask, hasOutMask;
    FileImageFactory FIF;
    MergeImageFactory MIF;

    // Ouverture du fichier texte listant les images
    std::ifstream file;

    file.open ( imageListFilename );
    if ( !file ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot open the file " << imageListFilename ;
        return -1;
    }

    // Lecture de l'image de sortie
    if ( readFileLine ( file,outputImagePath,&hasOutMask,outputMaskPath ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot read output image in the file : " << imageListFilename ;
        return -1;
    }

    // On doit connaître les dimensions des images en entrée pour pouvoir créer les images de sortie

    // Lecture et création des images sources
    int inputNb = 0;
    int out = 0;
    while ( ( out = readFileLine ( file,inputImagePath,&hasMask,inputMaskPath ) ) == 0 ) {
        FileImage* pImage = FIF.createImageToRead ( inputImagePath );
        if ( pImage == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot create a FileImage from the file " << inputImagePath ;
            return -1;
        }

        if ( inputNb == 0 ) {
            // C'est notre première image en entrée, on mémorise les caractéristiques)
            bitspersample = pImage->getBitsPerSample();
            sampleformat = pImage->getSampleFormat();
            width = pImage->getWidth();
            height = pImage->getHeight();
        } else {
            // Toutes les images en entrée doivent avoir certaines caractéristiques en commun
            if ( bitspersample != pImage->getBitsPerSample() ||
                    sampleformat != pImage->getSampleFormat() ||
                    width != pImage->getWidth() || height != pImage->getHeight() ) {

                BOOST_LOG_TRIVIAL(error) <<  "All input images must have same dimension and sample type" ;
                return -1;
            }
        }

        if ( hasMask ) {
            /* On a un masque associé, on en fait une image à lire et on vérifie qu'elle est cohérentes :
             *          - même dimensions que l'image
             *          - 1 seul canal (entier)
             */
            FileImage* pMask = FIF.createImageToRead ( inputMaskPath );
            if ( pMask == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot create a FileImage (mask) from the file " << inputMaskPath ;
                return -1;
            }

            if ( ! pImage->setMask ( pMask ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask " << inputMaskPath ;
                return -1;
            }
        }

        ImageIn.push_back ( pImage );
        inputNb++;
    }

    if ( out != -1 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Failure reading the file " << imageListFilename ;
        return -1;
    }

    // Fermeture du fichier
    file.close();

    // On crée notre MergeImage, qui s'occupera des calculs de fusion des pixels

    *ppMergeIn = MIF.createMergeImage ( ImageIn, samplesperpixel, background, transparent, mergeMethod );

    // Le masque fusionné est ajouté
    MergeMask* pMM = new MergeMask ( *ppMergeIn );

    if ( ! ( *ppMergeIn )->setMask ( pMM ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask to the merged image" ;
        return -1;
    }

    // Création des sorties
    *ppImageOut = FIF.createImageToWrite ( outputImagePath, fakeBbox, -1., -1., width, height, samplesperpixel,
                  sampleformat, bitspersample, photometric,compression );

    if ( *ppImageOut == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Impossible de creer l'image " << outputImagePath ;
        return -1;
    }

    if ( hasOutMask ) {
        *ppMaskOut = FIF.createImageToWrite ( outputMaskPath, fakeBbox, -1., -1., width, height, 1,
                     SampleFormat::UINT, 8, Photometric::MASK, Compression::DEFLATE );

        if ( *ppMaskOut == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Impossible de creer le masque " << outputMaskPath ;
            return -1;
        }
    }

    return 0;
}

/**
 ** \~french
 * \brief Fonction principale de l'outil overlayNtiff
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 si réussi, -1 sinon
 ** \~english
 * \brief Main function for tool overlayNtiff
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return 0 if success, -1 otherwise
 */
int main ( int argc, char **argv ) {

    FileImage* pImageOut ;
    FileImage* pMaskOut = NULL;
    MergeImage* pMergeIn;

    /* Initialisation des Loggers */
    boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::info );
    logging::add_common_attributes();
    boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
    logging::add_console_log (
        std::cout,
        keywords::format = "%Severity%\t%Message%"
    );

    BOOST_LOG_TRIVIAL(debug) <<  "Read parameters" ;
    // Lecture des parametres de la ligne de commande
    if ( parseCommandLine ( argc,argv ) < 0 ) {
        error ( "Cannot parse command line",-1 );
    }

    // On sait maintenant si on doit activer le niveau de log DEBUG
    if (debugLogger) {
        boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::debug );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Load" ;
    // Chargement des images
    if ( loadImages ( &pImageOut,&pMaskOut,&pMergeIn ) < 0 ) {
        error ( "Cannot load images from the configuration file",-1 );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Save image" ;
    // Enregistrement de l'image fusionnée
    if ( pImageOut->writeImage ( pMergeIn ) < 0 ) {
        error ( "Cannot write the merged image",-1 );
    }

    // Enregistrement du masque fusionné, si demandé
    if ( pMaskOut != NULL) {
        BOOST_LOG_TRIVIAL(debug) <<  "Save mask" ;
        if ( pMaskOut->writeImage ( pMergeIn->Image::getMask() ) < 0 ) {
            error ( "Cannot write the merged mask",-1 );
        }
    }

    delete pMergeIn;
    delete pImageOut;
    delete pMaskOut;

    delete [] background;
    if ( transparent != NULL ) {
        delete [] transparent;
    }

    return 0;
}
