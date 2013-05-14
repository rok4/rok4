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
 * \file overlayNtiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Fusion de N images aux mêmes dimensions, selon différentes méthodes
 * \~english \brief Merge N images with same dimensions, according to different merge methods
 *
 * \details Ce programme est destine à être utilisé dans la chaîne de génération de cache joinCache. Il est appele pour calculer les dalles avec plusieurs sources.
 *
 * Les images en entrée et celle en sortie peuvent :
 * \li avoir des nombres de canaux différents
 *
 * Les images en entrée et celle en sortie doivent avoir les même composantes suivantes :
 * \li hauteur et largeur en pixels
 * \li format des canaux
 *
 * Les formats des canaux gérés sont :
 * \li entier non signé sur 8 bits
 * \li flottant sur 32 bits
 *
 * On doit préciser en paramètre de la commande :
 * \li Un fichier texte contenant l'image finale, puis les images sources. On peut trouver également les masques associés aux images. L'ordre a de l'importance, les premières images sources seront considérées comme allant en dessous, quelque soit la méthode utilisée pour la fusion.
 * Format d'une ligne du fichier : \code<CHEMIN DE L'IMAGE> [<CHEMIN DU MASQUE ASSOCIÉ>]\endcode
 * Exemple de configuration :
 * \~ \code{.txt}
 * IMAGE.tif MASK.tif
 * IMG sources/image1.tif  sources/mask1.tif
 * IMG sources/image2.tif
 * \endcode
 * \~french
 * \li La compression de l'image de sortie
 * \li La méthode de fusion
 * \li Le nombre de canaux par pixel en sortie
 * \li La valeur à considérer comme transparente
 * \li la valeur à utiliser comme fond
 *
 * Les méthodes de fusion disponibles sont :
 * \~
 * \li TRANSPARENCY
 * \image html merge_transparency.png
 * \li MULTIPLY
 * \image html merge_multiply.png
 * \li MASK
 * \image html merge_mask.png
 * \~french
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
#include "Logger.h"
#include "LibtiffImage.h"
#include "MergeImage.h"
#include "Format.h"
#include "math.h"
#include "../be4version.h"

/** \~french Chemin du fichier de configuration des images */
char imageListFilename[256];
/** \~french Nombre de canaux par pixel de l'image en sortie */
uint16_t samplesperpixel = 0;
/** \~french Mode de fusion des images */
SampleType sampleType ( 0,0 );
/** \~french Photométrie (rgb, gray), pour les images en sortie */
uint16_t photometric = PHOTOMETRIC_RGB;
/** \~french Compression de l'image de sortie */
uint16_t compression = COMPRESSION_NONE;
/** \~french Mode de fusion des images */
Merge::MergeType mergeMethod = Merge::UNKNOWN;

/** \~french Couleur à considérer comme transparent dans le images en entrée. Vrai couleur (sur 3 canaux). Peut ne pas être définie */
int* transparent;
/** \~french Couleur à utiliser comme fond. Doit comporter autant de valeur qu'on veut de canaux dans l'image finale. Si un canal alpha est présent, il doit également être prémultiplié aux autres canaux */
int* background;

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande overlayNtiff
 * \details L'affichage se fait dans le niveau de logger INFO
 * \~ \code
 * overlayNtiff version X.X.X
 *
 * Create one TIFF image, from several images with same dimensions, with different available merge methods.
 * Sources and output image can have different numbers of samples per pixel. The sample type have to be the same for all sdources and will be the output one
 *
 * Usage: overlayNtiff -f <FILE> -m <VAL> -c <VAL> -s <VAL> -p <VAL [-n <VAL>] -b <VAL>
 * Parameters:
 *     -f configuration file : list of output and source images and masks
 *     -c output compression :
 *             raw     no compression
 *             none    no compression
 *             jpg     Jpeg encoding
 *             lzw     Lempel-Ziv & Welch encoding
 *             pkb     PackBits encoding
 *             zip     Deflate encoding
 *     -t value to consider as transparent, 3 integers, separated with comma. Optionnal
 *     -b value to use as background, one integer per output sample, separated with comma. If an alpha sample is provided, other samples hav to be pre-multiplied with it.
 *     -m merge method : used to merge input images, associated masks are always used if provided :
 *             ALPHATOP       images are merged by alpha blending
 *             MULTIPLY       samples are multiplied one by one
 *             TOP            only the top data pixel is kept
 *     -s samples per pixel : 1, 3 or 4
 *     -p photometric :
 *             gray    min is black
 *             rgb     for image with alpha too
 *
 * Examples
 *     - for gray orthophotography, with transparency (white is transparent)
 *     overlayNtiff -f conf.txt -m ALPHATOP -s 1 -c zip -p gray -n 255,255,255,255 -b 0
 *     - for DTM, considering masks only
 *     overlayNtiff -f conf.txt -m TOP -s 1 -c zip -p gray -b -99999
 * \endcode
 */
void usage() {
    LOGGER_INFO ( "\noverlayNtiff version " << BE4_VERSION << "\n\n" <<

                  "Create one TIFF image, from several images with same dimensions, with different available merge methods.\n" <<
                  "Sources and output image can have different numbers of samples per pixel. The sample type have to be the same for all sdources and will be the output one\n\n" <<

                  "Usage: overlayNtiff -f <FILE> -m <VAL> -c <VAL> -s <VAL> -p <VAL [-n <VAL>] -b <VAL>\n" <<

                  "Parameters:\n" <<
                  "    -f configuration file : list of output and source images and masks\n" <<
                  "    -c output compression :\n" <<
                  "            raw     no compression\n" <<
                  "            none    no compression\n" <<
                  "            jpg     Jpeg encoding\n" <<
                  "            lzw     Lempel-Ziv & Welch encoding\n" <<
                  "            pkb     PackBits encoding\n" <<
                  "            zip     Deflate encoding\n" <<
                  "    -t value to consider as transparent, 3 integers, separated with comma. Optionnal\n" <<
                  "    -b value to use as background, one integer per output sample, separated with comma. If an alpha sample is provided, other samples hav to be pre-multiplied with it\n" <<
                  "    -m merge method : used to merge input images, associated masks are always used if provided :\n" <<
                  "            ALPHATOP       images are merged by alpha blending\n" <<
                  "            MULTIPLY       samples are multiplied one by one\n" <<
                  "            TOP            only the top data pixel is kept\n" <<
                  "    -s samples per pixel : 1, 2, 3 or 4\n" <<
                  "    -p photometric :\n" <<
                  "            gray    min is black\n" <<
                  "            rgb     for image with alpha too\n\n" <<

                  "Examples\n" <<
                  "    - for gray orthophotography, with transparency (white is transparent)\n" <<
                  "    overlayNtiff -f conf.txt -m ALPHATOP -s 1 -c zip -p gray -n 255,255,255,255 -b 0\n" <<
                  "    - for DTM, considering masks only\n" <<
                  "    overlayNtiff -f conf.txt -m TOP -s 1 -c zip -p gray -b -99999\n\n" );
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur
 * \param[in] message message d'erreur
 * \param[in] errorCode code de retour
 */
void error ( std::string message, int errorCode ) {
    LOGGER_ERROR ( message );
    LOGGER_ERROR ( "Configuration file : " << imageListFilename );
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
            case 'f': // Images' list file
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with images' list file (option -f)" );
                    return -1;
                }
                strcpy ( imageListFilename,argv[i] );
                break;
            case 'm': // image merge method
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with merge method (option -m)" );
                    return -1;
                }
                mergeMethod = Merge::fromString ( argv[i] );
                if ( mergeMethod == Merge::UNKNOWN ) {
                    LOGGER_ERROR ( "Unknown value for merge method (option -m) : " << argv[i] );
                    return -1;
                }
                break;
            case 's': // samplesperpixel
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with samples per pixel (option -s)" );
                    return -1;
                }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                else if ( strncmp ( argv[i], "2",1 ) == 0 ) samplesperpixel = 2 ;
                else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                else {
                    LOGGER_ERROR ( "Unknown value for samples per pixel (option -s) : " << argv[i] );
                    return -1;
                }
                break;
            case 'c': // compression
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with compression (option -c)" );
                    return -1;
                }
                if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = COMPRESSION_ADOBE_DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = COMPRESSION_PACKBITS;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = COMPRESSION_JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = COMPRESSION_LZW;
                else {
                    LOGGER_ERROR ( "Unknown value for compression (option -c) : " << argv[i] );
                    return -1;
                }
                break;
            case 'p': // photometric
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with photometric (option -p)" );
                    return -1;
                }
                if ( strncmp ( argv[i], "gray",4 ) == 0 ) photometric = PHOTOMETRIC_MINISBLACK;
                else if ( strncmp ( argv[i], "rgb",3 ) == 0 ) photometric = PHOTOMETRIC_RGB;
                else {
                    LOGGER_ERROR ( "Unknown value for photometric (option -p) : " << argv[i] );
                    return -1;
                }
                break;
            case 't': // transparent color
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with transparent color (option -t)" );
                    return -1;
                }
                strcpy ( strTransparent,argv[i] );
                break;
            case 'b': // background color
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error with background color (option -b)" );
                    return -1;
                }
                strcpy ( strBg,argv[i] );
                break;
            default:
                LOGGER_ERROR ( "Unknown option : -" << argv[i][1] );
                return -1;
            }
        }
    }

    // Merge method control
    if ( mergeMethod == Merge::UNKNOWN ) {
        LOGGER_ERROR ( "We need to know the merge method (option -m)" );
        return -1;
    }

    // Image list file control
    if ( strlen ( imageListFilename ) == 0 ) {
        LOGGER_ERROR ( "We need to have one images' list (text file, option -f)" );
        return -1;
    }

    // Samples per pixel control
    if ( samplesperpixel == 0 ) {
        LOGGER_ERROR ( "We need to know the number of samples per pixel in the output image (option -s)" );
        return -1;
    }

    if ( strlen ( strTransparent ) != 0 ) {
        transparent = new int[3];

        // Transparent interpretation
        char* charValue = strtok ( strTransparent,"," );
        if ( charValue == NULL ) {
            LOGGER_ERROR ( "Error with option -t : 3 integers values separated by comma" );
            return -1;
        }
        int value = atoi ( charValue );
        transparent[0] = value;
        for ( int i = 1; i < 3; i++ ) {
            charValue = strtok ( NULL, "," );
            if ( charValue == NULL ) {
                LOGGER_ERROR ( "Error with option -t : 3 integers values separated by comma" );
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
            LOGGER_ERROR ( "Error with option -b : one integer value per final sample separated by comma" );
            return -1;
        }
        int value = atoi ( charValue );
        background[0] = value;

        for ( int i = 1; i < samplesperpixel; i++ ) {
            charValue = strtok ( NULL, "," );
            if ( charValue == NULL ) {
                LOGGER_ERROR ( "Error with option -b : one integer value per final sample separated by comma" );
                return -1;
            }
            value = atoi ( charValue );
            background[i] = value;
        }
    } else {
        LOGGER_ERROR ( "We need to know the background value for the output image (option -b)" );
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
            LOGGER_DEBUG ( "Configuration file end reached" );
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
int loadImages ( LibtiffImage** ppImageOut, LibtiffImage** ppMaskOut, MergeImage** ppMergeIn ) {
    char inputImagePath[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    char inputMaskPath[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];

    char outputImagePath[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    char outputMaskPath[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];

    std::vector<Image*> ImageIn;
    BoundingBox<double> fakeBbox ( 0.,0.,0.,0. );

    uint16_t bitspersample, sampleformat;
    int width, height;

    bool hasMask, hasOutMask;
    LibtiffImageFactory LIF;
    MergeImageFactory MIF;

    // Ouverture du fichier texte listant les images
    std::ifstream file;

    file.open ( imageListFilename );
    if ( !file ) {
        LOGGER_ERROR ( "Cannot open the file " << imageListFilename );
        return -1;
    }

    // Lecture de l'image de sortie
    if ( readFileLine ( file,outputImagePath,&hasOutMask,outputMaskPath ) ) {
        LOGGER_ERROR ( "Cannot read output image in the file : " << imageListFilename );
        return -1;
    }

    // On doit connaître les dimensions des images en entrée pour pouvoir créer les images de sortie

    // Lecture et création des images sources
    int inputNb = 0;
    int out = 0;
    while ( ( out = readFileLine ( file,inputImagePath,&hasMask,inputMaskPath ) ) == 0 ) {
        LibtiffImage* pImage = LIF.createLibtiffImageToRead ( inputImagePath, fakeBbox, -1., -1. );
        if ( pImage == NULL ) {
            LOGGER_ERROR ( "Cannot create a LibtiffImage from the file " << inputImagePath );
            return -1;
        }

        if ( inputNb == 0 ) {
            // C'est notre première image en entrée, on mémorise lse caractéristiques)
            bitspersample = pImage->getBitsPerSample();
            sampleformat = pImage->getSampleFormat();
            width = pImage->width;
            height = pImage->height;
        } else {
            // Toutes les images en entrée doivent avoir certaines caractéristiques en commun
            if ( bitspersample != pImage->getBitsPerSample() || sampleformat != pImage->getSampleFormat() ||
                    width != pImage->width || height != pImage->height ) {

                LOGGER_ERROR ( "All input images must have same dimension and sample format" );
                return -1;
            }
        }

        if ( hasMask ) {
            /* On a un masque associé, on en fait une image à lire et on vérifie qu'elle est cohérentes :
             *          - même dimensions que l'image
             *          - 1 seul canal (entier)
             */
            LibtiffImage* pMask = LIF.createLibtiffImageToRead ( inputMaskPath, fakeBbox, -1., -1. );
            if ( pMask == NULL ) {
                LOGGER_ERROR ( "Cannot create a LibtiffImage (mask) from the file " << inputMaskPath );
                return -1;
            }

            if ( ! pImage->setMask ( pMask ) ) {
                LOGGER_ERROR ( "Cannot add mask " << inputMaskPath );
                return -1;
            }
        }

        ImageIn.push_back ( pImage );
        inputNb++;
    }

    if ( out != -1 ) {
        LOGGER_ERROR ( "Failure reading the file " << imageListFilename );
        return -1;
    }

    // Fermeture du fichier
    file.close();

    sampleType = SampleType ( bitspersample, sampleformat );

    if ( ! sampleType.isSupported() ) {
        error ( "Supported sample format are :\n" + sampleType.getHandledFormat(),-1 );
    }

    // On crée notre MergeImage, qui s'occupera des calculs de fusion des pixels

    *ppMergeIn = MIF.createMergeImage ( ImageIn, samplesperpixel, background, transparent, mergeMethod );

    // Le masque fusionné est ajouté
    MergeMask* pMM = new MergeMask ( *ppMergeIn );

    if ( ! ( *ppMergeIn )->setMask ( pMM ) ) {
        LOGGER_ERROR ( "Cannot add mask to the merged image" );
        return -1;
    }

    // Création des sorties
    *ppImageOut = LIF.createLibtiffImageToWrite ( outputImagePath, fakeBbox, -1., -1., width, height, samplesperpixel,
                  sampleType, photometric,compression,16 );

    if ( *ppImageOut == NULL ) {
        LOGGER_ERROR ( "Impossible de creer l'image " << outputImagePath );
        return -1;
    }

    if ( hasOutMask ) {
        *ppMaskOut = LIF.createLibtiffImageToWrite ( outputMaskPath, fakeBbox, -1., -1., width, height, 1,
                     SampleType ( 8,SAMPLEFORMAT_UINT ),
                     PHOTOMETRIC_MINISBLACK,COMPRESSION_PACKBITS,16 );

        if ( *ppMaskOut == NULL ) {
            LOGGER_ERROR ( "Impossible de creer le masque " << outputMaskPath );
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

    LibtiffImage* pImageOut ;
    LibtiffImage* pMaskOut = NULL;
    MergeImage* pMergeIn;

    /* Initialisation des Loggers */
    Logger::setOutput ( STANDARD_OUTPUT_STREAM_FOR_ERRORS );

    Accumulator* acc = new StreamAccumulator();
    //Logger::setAccumulator(DEBUG, acc);
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
    // Lecture des parametres de la ligne de commande
    if ( parseCommandLine ( argc,argv ) < 0 ) {
        error ( "Cannot parse command line",-1 );
    }

    LOGGER_DEBUG ( "Load" );
    // Chargement des images
    if ( loadImages ( &pImageOut,&pMaskOut,&pMergeIn ) < 0 ) {
        error ( "Cannot load images from the configuration file",-1 );
    }

    LOGGER_DEBUG ( "Save image" );
    // Enregistrement de l'image fusionnée
    if ( pImageOut->writeImage ( pMergeIn ) < 0 ) {
        error ( "Cannot write the merged image",-1 );
    }

    LOGGER_DEBUG ( "Save mask" );
    // Enregistrement du masque fusionné, si demandé
    if ( pMaskOut != NULL && pMaskOut->writeImage ( pMergeIn->Image::getMask() ) < 0 ) {
        error ( "Cannot write the merged mask",-1 );
    }

    delete acc;
    delete pMergeIn;
    delete pImageOut;
    delete pMaskOut;

    delete [] background;
    if ( transparent != NULL ) {
        delete [] transparent;
    }

    return 0;
}
