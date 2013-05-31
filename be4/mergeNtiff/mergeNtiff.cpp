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
 * \file mergeNtiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Création d'une image TIFF géoréférencée à partir de n images TIFF sources géoréférencées
 * \~english \brief Create one georeferenced TIFF image from several georeferenced TIFF images
 * \~ \image html mergeNtiff.png \~french
 * \details Les images en entrée peuvent :
 * \li être de différentes résolutions
 * \li ne pas couvrir entièrement l'emprise de l'image de sortie
 * \li être recouvrantes entre elles
 * \li être dans des systèmes spatiaux différents
 *
 * Les images en entrée et celle en sortie doivent avoir les même composantes suivantes :
 * \li le même nombre de canaux
 * \li le même format de canal
 *
 * Les formats des canaux gérés sont :
 * \li entier non signé sur 8 bits
 * \li flottant sur 32 bits
 *
 * On doit préciser en paramètre de la commande :
 * \li Un fichier texte contenant l'image finale, puis les images sources, avec leur georeferencement (resolution, emprise, SRS). On peut trouver également les masques associés aux images.
 * Format d'une ligne du fichier : \code<TYPE> <CHEMIN> <CRS> <XMIN> <YMAX> <XMAX> <YMIN> <RESX> <RESY>\endcode
 * Le chemin peut contenir un point d'interrogation comme premier caractère, cela voudra dire qu'on veut utiliser la racine placée en paramètre (option -r). Si celle-ci n'est pas précisée, le point d'interrogation est juste supprimé.
 * Exemple de configuration :
 * \~ \code{.txt}
 * IMG ?IMAGE.tif               EPSG:2154       -499    1501    1501    -499    2       2
 * MSK ?MASK.tif
 * IMG sources/imagefond.tif    EPSG:2154       -499    1501    1501    -499    4       4
 * MSK sources/maskfond.tif
 * IMG sources/image1.tif       IGNF:LAMB93     0       1000    1000    0       1       1
 * IMG sources/image2.tif       IGNF:LAMB93     500     1500    1500    500     1       1
 * MSK sources/mask2.tif
 * \endcode
 * \~french
 * \li On peut préciser une racine (un répertoire) à ajouter au chemin de l'image de sortie (et de son masque). Le chemin du répertoire doit finir par le séparateur de dossier (slash en linux).
 * \li La compression de l'image de sortie
 * \li Le mode d'interpolation
 * \li Le nombre de canaux par pixel
 * \li Le nombre de bits par canal
 * \li Le format du canal (entier ou flottant)
 * \li La valeur de non-donnée
 *
 * La légende utilisée dans tous les schémas de la documentation de ce fichier sera la suivante
 *
 * \~ \image html mergeNtiff_legende.png \~french
 *
 * Pour réaliser la fusion des images en entrée, on traite différemment :
 * \li les images qui sont superposables à l'image de sortie (même SRS, mêmes résolutions, mêmes phases) : on parle alors d'images compatibles, pas de réechantillonnage nécessaire.
 * \li les images non compatibles mais de même SRS : un passage par le réechantillonnage (plus lourd en calcul) est indispensable.
 * \li les images non compatibles et de SRS différents : un passage par la reprojection (encore plus lourd en calcul) est indispensable.
 *
 * Exemple d'appel à la commande :
 * \li pour des ortho-images \~english \li for orthoimage
 * \~ \code
 * mergeNtiff -f conf.txt -r /home/ign/results/ -c zip -i bicubic -s 3 -b 8 -p rgb -a uint -n 255,255,255
 * \endcode
 * \~french \li pour du MNT \~english \li for DTM
 * \~ \code
 * mergeNtiff -f conf.txt -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999
 * \endcode
 ** \~french
 * \todo Gérer correctement un canal alpha
 * \todo Permettre l'ajout ou la suppression à la volée d'un canal alpha
 */

#include <proj_api.h>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "tiffio.h"
#include "Logger.h"

#include "LibtiffImage.h"
#include "ResampledImage.h"
#include "ReprojectedImage.h"
#include "ExtendedCompoundImage.h"

#include "CRS.h"
#include "Interpolation.h"
#include "Format.h"
#include "math.h"
#include "../be4version.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

// Paramètres de la ligne de commande déclarés en global
/** \~french Chemin du fichier de configuration des images */
char imageListFilename[256];
/** \~french Racine pour les images de sortie */
char outImagesRoot[256];
/** \~french Valeur de nodata sour forme de chaîne de caractère (passée en paramètre de la commande) */
char strnodata[256];
/** \~french Nombre de canaux par pixel, dans les images en entrée et celle en sortie */
uint16_t samplesperpixel;
/** \~french Type du canal (entier, flottant, signé ou non...), dans les images en entrée et celle en sortie */
SampleType sampleType ( 0,0 );
/** \~french Photométrie (rgb, gray), dans les images en entrée et celle en sortie */
uint16_t photometric;
/** \~french Compression de l'image de sortie */
uint16_t compression;
/** \~french Type de donnée traitée : image (1) ou meta-donnée (0, non implémenté) */
int type=-1;
/** \~french Interpolation utilisée pour le réechantillonnage ou la reprojection */
Interpolation::KernelType interpolation;

/** \~french Thread utilisé par PROJ4 pour les outils de reprojection */
static pthread_mutex_t mutex_proj= PTHREAD_MUTEX_INITIALIZER;

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande mergeNtiff
 * \details L'affichage se fait dans le niveau de logger INFO
 * \~ \code
 * mergeNtiff version X.X.X
 *
 * Usage: mergeNtiff -f <FILE> [-r <DIR>] -c <VAL> -a <VAL> -i <VAL> -n <VAL> -s <VAL> -b <VAL> -p <VAL>
 *
 * Parameters:
 *      -f configuration file : list of output and source images and masks
 *      -r root : root directory for output files, have to end with a '/'
 *      -c output compression :
 *              raw     no compression
 *              none    no compression
 *              jpg     Jpeg encoding
 *              lzw     Lempel-Ziv & Welch encoding
 *              pkb     PackBits encoding
 *              zip     Deflate encoding
 *      -a sample format : uint (unsigned integer) or float
 *      -i interpolation : used for resampling :
 *              nn      nearest neighbor
 *              linear
 *              bicubic
 *              lanczos lanczos 3
 *      -n nodata value, one interger per sample, seperated with comma. Examples
 *              -99999 for DTM
 *              255,255,255 for orthophotography
 *      -s samples per pixel : 1, 3 or 4
 *      -b bits per sample : 8 (for unsigned 8-bit integer) or 32 (for 32-bit float)
 *      -p photometric :
 *              gray    min is black
 *              rgb     for image with alpha too
 *
 * Examples
 *      - for orthophotography
 *      mergeNtiff -f conf.txt -c zip -i bicubic -s 3 -b 8 -p rgb -a uint -n 255,255,255
 *      - for DTM
 *      mergeNtiff -f conf.txt -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999
 * \endcode
 */
void usage() {
    LOGGER_INFO ( "\nmergeNtiff version " << BE4_VERSION << "\n\n" <<

                  "Create one georeferenced TIFF image from several georeferenced TIFF images.\n\n" <<

                  "Usage: mergeNtiff -f <FILE> [-r <DIR>] -c <VAL> -a <VAL> -i <VAL> -n <VAL> -s <VAL> -b <VAL> -p <VAL>\n" <<

                  "Parameters:\n" <<
                  "    -f configuration file : list of output and source images and masks\n" <<
                  "    -r output root : root directory for output files, have to end with a '/'\n" <<
                  "    -c output compression :\n" <<
                  "            raw     no compression\n" <<
                  "            none    no compression\n" <<
                  "            jpg     Jpeg encoding\n" <<
                  "            lzw     Lempel-Ziv & Welch encoding\n" <<
                  "            pkb     PackBits encoding\n" <<
                  "            zip     Deflate encoding\n" <<
                  "    -a sample format : uint (unsigned integer) or float\n" <<
                  "    -i interpolation : used for resampling :\n" <<
                  "            nn      nearest neighbor\n" <<
                  "            linear\n" <<
                  "            bicubic\n" <<
                  "            lanczos lanczos 3\n" <<
                  "    -n nodata value, one interger per sample, seperated with comma. Examples\n" <<
                  "            -99999 for DTM\n" <<
                  "            255,255,255 for orthophotography\n" <<
                  "    -s samples per pixel : 1, 3 or 4\n" <<
                  "    -b bits per sample : 8 (for unsigned 8-bit integer) or 32 (for 32-bit float)\n" <<
                  "    -p photometric :\n" <<
                  "            gray    min is black\n" <<
                  "            rgb     for image with alpha too\n\n" <<

                  "Examples\n" <<
                  "    - for orthophotography\n" <<
                  "    mergeNtiff -f conf.txt -c zip -i bicubic -s 3 -b 8 -p rgb -a uint -n 255,255,255\n" <<
                  "    - for DTM\n" <<
                  "    mergeNtiff -f conf.txt -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999\n\n" );
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

    uint16_t bitspersample = 0, sampleformat = 0;

    if ( argc < 17 && argc != 2 ) {
        LOGGER_ERROR ( "Unvalid parameters number : is " << argc << " and have to be 17 or more (2 to request help)" );
        return -1;
    }

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
            case 'f': // fichier de liste des images source
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -f" );
                    return -1;
                }
                strcpy ( imageListFilename,argv[i] );
                break;
            case 'r': // racine pour le fichier de configuration
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -r" );
                    return -1;
                }
                strcpy ( outImagesRoot,argv[i] );
                break;
            case 'i': // interpolation
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -i" );
                    return -1;
                }
                if ( strncmp ( argv[i], "lanczos",7 ) == 0 ) interpolation = Interpolation::LANCZOS_3;
                else if ( strncmp ( argv[i], "nn",2 ) == 0 ) interpolation = Interpolation::NEAREST_NEIGHBOUR;
                else if ( strncmp ( argv[i], "bicubic",7 ) == 0 ) interpolation = Interpolation::CUBIC;
                else if ( strncmp ( argv[i], "linear",6 ) == 0 ) interpolation = Interpolation::LINEAR;
                else {
                    LOGGER_ERROR ( "Unknown value for option -i : " << argv[i] );
                    return -1;
                }
                break;
            case 'n': // nodata
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -n" );
                    return -1;
                }
                strcpy ( strnodata,argv[i] );
                break;
            case 's': // samplesperpixel
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -s" );
                    return -1;
                }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                else {
                    LOGGER_ERROR ( "Unknown value for option -s : " << argv[i] );
                    return -1;
                }
                break;
            case 'b': // bitspersample
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -b" );
                    return -1;
                }
                if ( strncmp ( argv[i], "8",1 ) == 0 ) bitspersample = 8 ;
                else if ( strncmp ( argv[i], "32",2 ) == 0 ) bitspersample = 32 ;
                else {
                    LOGGER_ERROR ( "Unknown value for option -b : " << argv[i] );
                    return -1;
                }
                break;
            case 'a': // sampleformat
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -a" );
                    return -1;
                }
                if ( strncmp ( argv[i],"uint",4 ) == 0 ) sampleformat = SAMPLEFORMAT_UINT ;
                else if ( strncmp ( argv[i],"float",5 ) == 0 ) sampleformat = SAMPLEFORMAT_IEEEFP ;
                else {
                    LOGGER_ERROR ( "Unknown value for option -a : " << argv[i] );
                    return -1;
                }
                break;
            case 'p': // photometric
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -p" );
                    return -1;
                }
                if ( strncmp ( argv[i], "gray",4 ) == 0 ) photometric = PHOTOMETRIC_MINISBLACK;
                else if ( strncmp ( argv[i], "rgb",3 ) == 0 ) photometric = PHOTOMETRIC_RGB;
                else {
                    LOGGER_ERROR ( "Unknown value for option -p : " << argv[i] );
                    return -1;
                }
                break;
            case 'c': // compression
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -c" );
                    return -1;
                }
                if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = COMPRESSION_ADOBE_DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = COMPRESSION_PACKBITS;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = COMPRESSION_JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = COMPRESSION_LZW;
                else {
                    LOGGER_ERROR ( "Unknown value for option -c : " << argv[i] );
                    return -1;
                }
                break;
            default:
                LOGGER_ERROR ( "Unknown option : -" << argv[i][1] );
                return -1;
            }
        }
    }

    LOGGER_DEBUG ( "mergeNtiff -f " << imageListFilename );

    sampleType = SampleType ( bitspersample,sampleformat );

    if ( ! sampleType.isSupported() ) {
        LOGGER_ERROR ( "Supported sample format are :\n" + sampleType.getHandledFormat() );
        return -1;
    }

    return 0;
}

/**
 * \~french
 * \brief Enregistre une image TIFF, avec passage de ses composantes (pour le déboguage)
 * \details Toutes les informations nécessaires à l'écriture d'une image n'étant pas stockées dans un objet Image, on se doit de les préciser en paramètre de la fonction. Cette fonction est utilisée pour le déboguage pour enregistrer des images intermédiaires. Pour l'image finale, on utilisera la fonction d'enregistrement propre aux objets de la classe LibtiffImage
 * \param[in] pImage image à enregistrer
 * \param[in] pName chemin de l'image à écrire
 * \param[in] bps nombre de bits par canal de l'image TIFF
 * \param[in] sf format des canaux (entier ou fottant)
 * \param[in] ph photométrie de l'image à écrire
 * \param[in] comp compression de l'image à écrire
 * \return code de retour, 0 si réussi, -1 sinon
 */
int saveImage ( Image *pImage, char* pName, uint16_t bps, uint16_t sf, uint16_t ph, uint16_t comp ) {
    // Ouverture du fichier
    TIFF* output=TIFFOpen ( pName,"w" );
    if ( output==NULL ) {
        LOGGER_ERROR ( "Impossible d'ouvrir le fichier " << pName << " en ecriture" );
        return -1;
    }

    // Ecriture de l'en-tete
    TIFFSetField ( output, TIFFTAG_IMAGEWIDTH, pImage->getWidth() );
    TIFFSetField ( output, TIFFTAG_IMAGELENGTH, pImage->getHeight() );
    TIFFSetField ( output, TIFFTAG_SAMPLESPERPIXEL, pImage->channels );
    TIFFSetField ( output, TIFFTAG_BITSPERSAMPLE, bps );
    TIFFSetField ( output, TIFFTAG_SAMPLEFORMAT, sf );
    TIFFSetField ( output, TIFFTAG_PHOTOMETRIC, ph );
    TIFFSetField ( output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
    TIFFSetField ( output, TIFFTAG_COMPRESSION, comp );
    TIFFSetField ( output, TIFFTAG_ROWSPERSTRIP, 1 );
    TIFFSetField ( output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE );

    // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if ( sf==SAMPLEFORMAT_UINT ) {
        buf_u = ( unsigned char* ) _TIFFmalloc ( pImage->getWidth() * pImage->channels * bps / 8 );
        for ( int line = 0; line < pImage->getHeight(); line++ ) {
            pImage->getline ( buf_u,line );
            TIFFWriteScanline ( output, buf_u, line, 0 );
        }
    } else if ( sf==SAMPLEFORMAT_IEEEFP ) {
        buf_f = ( float* ) _TIFFmalloc ( pImage->getWidth()*pImage->channels*bps/8 );
        for ( int line = 0; line < pImage->getHeight(); line++ ) {
            pImage->getline ( buf_f,line );
            TIFFWriteScanline ( output, buf_f, line, 0 );
        }
    }

    // Liberation
    if ( buf_u ) _TIFFfree ( buf_u );
    if ( buf_f ) _TIFFfree ( buf_f );
    TIFFClose ( output );
    return 0;
}

/**
 * \~french
 * \brief Lit une ligne (ou deux si présence d'un masque) du fichier de configuration
 * \details On parse la ligne courante du fichier de configuration, en stockant les valeurs dans les variables fournies. On saute les lignes vides. On lit ensuite la ligne suivante :
 * \li si elle correspond à un masque, on complète les informations
 * \li si elle ne correspond pas à un masque, on recule le pointeur
 *
 * \param[in,out] file flux de lecture vers le fichier de configuration
 * \param[out] imageFileName chemin de l'image lu dans le fichier de configuration
 * \param[out] hasMask précise si l'image possède un masque
 * \param[out] maskFileName chemin du masque lu dans le fichier de configuration
 * \param[out] crs SRS du rectangle englobant
 * \param[out] bbox rectangle englobant de l'image lue (et de son masque)
 * \param[out] resx résolution en X de l'image lue (et de son masque)
 * \param[out] resy résolution en Y de l'image lue (et de son masque)
 * \return code de retour, 0 en cas de succès, -1 si la fin du fichier est atteinte, 1 en cas d'erreur
 */
int readFileLine ( std::ifstream& file, char* imageFileName, bool* hasMask, char* maskFileName, std::string* crs, BoundingBox<double>* bbox, double* resx, double* resy ) {
    std::string str;
    char tmpPath[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    int rootLength = strlen ( outImagesRoot );

    memset ( imageFileName, 0, LIBTIFFIMAGE_MAX_FILENAME_LENGTH );
    memset ( maskFileName, 0, LIBTIFFIMAGE_MAX_FILENAME_LENGTH );

    while ( str.empty() ) {
        if ( file.eof() ) {
            LOGGER_DEBUG ( "Configuration file end reached" );
            return -1;
        }
        std::getline ( file,str );
    }

    int pos;
    int nb;

    char type[3];
    char tmpCRS[20];

    if ( ( nb = std::sscanf ( str.c_str(),"%s %s %s %lf %lf %lf %lf %lf %lf",
                              type, tmpPath, tmpCRS, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, resx, resy ) ) == 9 ) {
        if ( memcmp ( type,"IMG",3 ) ) {
            LOGGER_ERROR ( "We have to read an image information at first." );
            return 1;
        }

        pos = file.tellg();
    } else {
        LOGGER_ERROR ( "We have to read 9 values, we have " << nb );
        LOGGER_ERROR ( "\t line : " << str );
        return 1;
    }

    crs->assign(tmpCRS);

    if ( ! strncmp ( tmpPath,"?",1 ) ) {
        strcpy ( imageFileName,outImagesRoot );
        strcpy ( & ( imageFileName[rootLength] ),& ( tmpPath[1] ) );
    } else {
        strcpy ( imageFileName,tmpPath );
    }

    str.clear();

    // Récupération d'un éventuel masque
    while ( str.empty() ) {
        if ( file.eof() ) {
            *hasMask = false;
            return 0;
        }
        std::getline ( file,str );
    }

    if ( ( std::sscanf ( str.c_str(),"%s %s", type, tmpPath ) ) != 2 || memcmp ( type,"MSK",3 ) ) {
        /* La ligne ne correspond pas au masque associé à l'image lue juste avant.
         * C'est en fait l'image suivante (ou une erreur). On doit donc remettre le
         * pointeur de manière à ce que cette ligne soit lue au prochain appel de
         * readFileLine.
         */
        *hasMask = false;
        file.seekg ( pos );
    } else {
        if ( ! strncmp ( tmpPath,"?",1 ) ) {
            strcpy ( maskFileName,outImagesRoot );
            strcpy ( & ( maskFileName[rootLength] ),& ( tmpPath[1] ) );
        } else {
            strcpy ( maskFileName,tmpPath );
        }
        *hasMask = true;
    }

    return 0;
}

/**
 * \~french
 * \brief Charge les images en entrée et en sortie depuis le fichier de configuration
 * \details On va récupérer toutes les informations de toutes les images et masques présents dans le fichier de configuration et créer les objets LibtiffImage correspondant. Toutes les images ici manipulées sont de vraies images (physiques) dans ce sens où elles sont des fichiers soit lus, soit qui seront écrits.
 *
 * Le chemin vers le fichier de configuration est stocké dans la variables globale imageListFilename et outImagesRoot va être concaténer au chemin vers les fichiers de sortie.
 * \param[out] ppImageOut image résultante de l'outil
 * \param[out] ppMaskOut masque résultat de l'outil, si demandé
 * \param[out] pImageIn ensemble des images en entrée
 * \return code de retour, 0 si réussi, -1 sinon
 */
int loadImages ( LibtiffImage** ppImageOut, LibtiffImage** ppMaskOut, std::vector<LibtiffImage*>* pImageIn ) {
    char imageFileName[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    char maskFileName[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    BoundingBox<double> bbox ( 0.,0.,0.,0. );
    int width, height;
    bool hasMask;
    std::string stringCRS;
    double resx, resy;
    LibtiffImageFactory factory;


    // Ouverture du fichier texte listant les images
    std::ifstream file;

    file.open ( imageListFilename );
    if ( !file ) {
        LOGGER_ERROR ( "Impossible d'ouvrir le fichier " << imageListFilename );
        return -1;
    }

    // Lecture et creation de l image de sortie
    if ( readFileLine ( file,imageFileName,&hasMask,maskFileName, &stringCRS, &bbox,&resx,&resy ) ) {
        LOGGER_ERROR ( "Erreur lecture des premieres lignes du fichier de parametres: " << imageListFilename );
        return -1;
    }

    CRS crs(stringCRS);

    // Arrondi a la valeur entiere la plus proche
    width = lround ( ( bbox.xmax - bbox.xmin ) / ( resx ) );
    height = lround ( ( bbox.ymax - bbox.ymin ) / ( resy ) );

    *ppImageOut = factory.createLibtiffImageToWrite ( imageFileName, bbox,resx, resy, width, height, samplesperpixel,
                  sampleType, photometric,COMPRESSION_NONE,16 );

    (*ppImageOut)->setCRS(crs);

    if ( *ppImageOut == NULL ) {
        LOGGER_ERROR ( "Impossible de creer l'image " << imageFileName );
        return -1;
    }

    if ( hasMask ) {

        *ppMaskOut = factory.createLibtiffImageToWrite ( maskFileName, bbox,resx, resy, width, height, 1,
                     SampleType ( 8,SAMPLEFORMAT_UINT ),
                     PHOTOMETRIC_MINISBLACK,COMPRESSION_PACKBITS,16 );

        (*ppMaskOut)->setCRS(crs);

        if ( *ppMaskOut == NULL ) {
            LOGGER_ERROR ( "Impossible de creer le masque " << maskFileName );
            return -1;
        }
    }

    // Lecture et creation des images sources
    int out=0;
    while ( ( out = readFileLine ( file,imageFileName,&hasMask,maskFileName, &stringCRS,&bbox,&resx,&resy ) ) == 0 ) {

        crs.setRequestCode(stringCRS);

        LibtiffImage* pImage=factory.createLibtiffImageToRead ( imageFileName, bbox, resx, resy );
        if ( pImage == NULL ) {
            LOGGER_ERROR ( "Impossible de creer une image a partir de " << imageFileName );
            return -1;
        }
        pImage->setCRS(crs);

        if ( hasMask ) {
            LibtiffImage* pMask=factory.createLibtiffImageToRead ( maskFileName, bbox, resx, resy );
            if ( pMask == NULL ) {
                LOGGER_ERROR ( "Impossible de creer un masque a partir de " << maskFileName );
                return -1;
            }
            pMask->setCRS(crs);

            if ( ! pImage->setMask ( pMask ) ) {
                LOGGER_ERROR ( "Cannot add mask to the input LibtiffImage" );
                return -1;
            }
        }

        pImageIn->push_back ( pImage );
    }

    if ( out != -1 ) {
        LOGGER_ERROR ( "Erreur lecture du fichier de parametres: " << imageListFilename );
        return -1;
    }

    // Fermeture du fichier
    file.close();

    if ( pImageIn->size() == 0 ) {
        LOGGER_ERROR ( "Erreur lecture du fichier de parametres '" << imageListFilename << "' : pas de données en entrée." );
        return -1;
    }

    return 0;
}

/**
 * \~french
 * \brief Contrôle la cohérence des images en entrée et celle en sortie
 * \details On vérifie que les résolutions fournies ne sont pas nulles et que le format des canaux est le même pour les images en entrée et pour celle en sortie
 * \param[in] pImageOut image résultante de l'outil
 * \param[in] ImageIn images en entrée
 * \return code de retour, 0 si réussi, -1 sinon
 * \todo Contrôler les éventuels masques
 */
int checkImages ( LibtiffImage* pImageOut, std::vector<LibtiffImage*>& ImageIn ) {
    for ( unsigned int i=0; i < ImageIn.size(); i++ ) {
        if ( ImageIn.at ( i )->getResX() *ImageIn.at ( i )->getResY() == 0. ) {
            LOGGER_ERROR ( "Source image " << i+1 << " is not valid (resolutions)" );
            ImageIn.at ( i )->print();
            return -1;
        }
        if ( ImageIn.at ( i )->channels != pImageOut->channels ) {
            LOGGER_ERROR ( "Source image " << i+1 << " is not valid (samples per pixel have to be " << pImageOut->channels << ")" );
            ImageIn.at ( i )->print();
            return -1;
        }
    }
    if ( pImageOut->getResX() *pImageOut->getResY() == 0. ) {
        LOGGER_ERROR ( "Output image (" << pImageOut->getFilename() << ") is not valid (resolutions)" );
        pImageOut->print();
        return -1;
    }

    return 0;
}

/**
 * \~french
 * \brief Trie les images sources en paquets d'images superposables
 * \details On réunit les images en paquets, dans lesquels :
 * \li toutes les images ont la même résolution, en X et en Y
 * \li toutes les images ont la même phase, en X et en Y
 *
 * On conserve cependant l'ordre originale des images, quitte à augmenter le nombre de paquets final.
 * Ce tri sert à simplifier le traitement des images et leur réechantillonnage.
 *
 * \~ \image html mergeNtiff_package.png \~french
 *
 * \param[in] ImageIn images en entrée
 * \param[out] pTabImageIn images en entrée, triées en paquets compatibles
 * \return code de retour, 0 si réussi, -1 sinon
 */
int sortImages ( std::vector<LibtiffImage*> ImageIn, std::vector<std::vector<Image*> >* pTabImageIn ) {
    std::vector<Image*> vTmpImg;
    std::vector<LibtiffImage*>::iterator itiniImg = ImageIn.begin();

    /* we create consistent images' vectors (X/Y resolution and X/Y phases)
     * Masks are moved in parallel with images
     */
    for ( std::vector<LibtiffImage*>::iterator itImg = ImageIn.begin(); itImg < ImageIn.end()-1; itImg++ ) {

        if ( ! ( *itImg )->isCompatibleWith ( * ( itImg+1 ) ) ) {
            // two following images are not compatible, we split images' vector
            vTmpImg.assign ( itiniImg,itImg+1 );
            itiniImg = itImg+1;
            pTabImageIn->push_back ( vTmpImg );
        }
    }

    // we don't forget to store last images in pTabImageIn
    // images
    vTmpImg.assign ( itiniImg,ImageIn.end() );
    pTabImageIn->push_back ( vTmpImg );

    return 0;
}

/**
 * \~french
 * \brief Modifie le rectangle englobant pour le rendre en phase avec l'image
 * \details Les 4 valeurs du rectangle englobant seront modifiée (au minimum) afin d'avoir les mêmes phases que celles de l'image fournie. Cependant, l'étendue finale ser incluse dans celle initiale (pas d'agrandissement de la bounding box).
 *
 * \param[in] pImage image avec laquelle la bounding box doit être mise en phase
 * \param[in,out] bbox rectangle englobant à mettre en phase
 */
void makePhase ( Image* pImage, BoundingBox<double> *bbox ) {
    double resx_dst = pImage->getResX(), resy_dst = pImage->getResY();
    
    double intpart;
    double phi = 0;
    double phaseDiff = 0;

    double phaseX = pImage->getPhaseX();
    double phaseY = pImage->getPhaseY();

    LOGGER_DEBUG ( "Bbox avant rephasage : " << bbox->toString() );

    // Mise en phase de xmin (sans que celui ci puisse être plus petit)
    phi = modf ( bbox->xmin/resx_dst, &intpart );
    if ( phi < 0. ) {
        phi += 1.0;
    }

    if ( fabs ( phi-phaseX ) > 0.01 && fabs ( phi-phaseX ) < 0.99 ) {
        LOGGER_DEBUG("Mise en phase de xmin");
        phaseDiff = phaseX - phi;
        if ( phaseDiff < 0. ) {
            phaseDiff += 1.0;
        }
        bbox->xmin += phaseDiff*resx_dst;
    }

    // Mise en phase de xmax (sans que celui ci puisse être plus grand)
    phi = modf ( bbox->xmax/resx_dst, &intpart );
    if ( phi < 0. ) {
        phi += 1.0;
    }

    if ( fabs ( phi-phaseX ) > 0.01 && fabs ( phi-phaseX ) < 0.99 ) {
        LOGGER_DEBUG("Mise en phase de xmax");
        phaseDiff = phaseX - phi;
        if ( phaseDiff > 0. ) {
            phaseDiff -= 1.0;
        }
        bbox->xmax += phaseDiff*resx_dst;
    }

    // Mise en phase de ymin (sans que celui ci puisse être plus petit)
    phi = modf ( bbox->ymin/resy_dst, &intpart );
    if ( phi < 0. ) {
        phi += 1.0;
    }

    if ( fabs ( phi-phaseY ) > 0.01 && fabs ( phi-phaseY ) < 0.99 ) {
        LOGGER_DEBUG("Mise en phase de ymin");
        phaseDiff = phaseY - phi;
        if ( phaseDiff < 0. ) {
            phaseDiff += 1.0;
        }
        bbox->ymin += phaseDiff*resy_dst;
    }

    // Mise en phase de ymax (sans que celui ci puisse être plus grand)
    phi = modf ( bbox->ymax/resy_dst, &intpart );
    if ( phi < 0. ) {
        phi += 1.0;
    }

    if ( fabs ( phi-phaseY ) > 0.01 && fabs ( phi-phaseY ) < 0.99 ) {
        LOGGER_DEBUG("Mise en phase de ymax");
        phaseDiff = phaseY - phi;
        if ( phaseDiff > 0. ) {
            phaseDiff -= 1.0;
        }
        bbox->ymax += phaseDiff*resy_dst;
    }

    LOGGER_DEBUG ( "Bbox après rephasage : " << bbox->toString() );
}

/**
 * \~french
 * \brief Réechantillonne un paquet d'images compatibles
 * \details On crée l'objet ResampledImage correspondant au réechantillonnage du paquet d'images, afin de le rendre compatible avec l'image de sortie. On veut que l'emprise de l'image réechantillonnée ne dépasse ni de l'image de sortie, ni des images en entrée (sans prendre en compte les miroirs, données virtuelles).
 * \param[in] pImageOut image résultante de l'outil
 * \param[in] pECI paquet d'images compatibles, à réechantillonner
 * \param[in] ppRImage image réechantillonnée
 * \return VRAI si succès, FAUX sinon
 */
bool resampleImages ( LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, ResampledImage** ppRImage ) {

    double resx_dst = pImageOut->getResX(), resy_dst = pImageOut->getResY();
    
    const Kernel& K = Kernel::getInstance ( interpolation );

    // Ajout des miroirs
    // Valeurs utilisées pour déterminer la taille des miroirs en pixel (taille optimale en fonction du noyau utilisé)
    int mirrorSizeX = ceil ( K.size ( resx_dst / pECI->getResX() ) ) + 1;
    int mirrorSizeY = ceil ( K.size ( resy_dst / pECI->getResY() ) ) + 1;

    int mirrorSize = std::max(mirrorSizeX, mirrorSizeY);

    LOGGER_DEBUG("Mirror's size : " << mirrorSize);

    // On mémorise la bbox d'origine, sans les miroirs
    BoundingBox<double> realBbox = pECI->getBbox();

    if ( ! pECI->addMirrors ( mirrorSize ) ) {
        LOGGER_ERROR ( "Unable to add mirrors" );
        return false;
    }

    /* L'image reechantillonnee est limitee a l'intersection entre l'image de sortie et les images sources
     * (sans compter les miroirs)
     */
    double xmin_dst=__max ( realBbox.xmin,pImageOut->getXmin() );
    double xmax_dst=__min ( realBbox.xmax,pImageOut->getXmax() );
    double ymin_dst=__max ( realBbox.ymin,pImageOut->getYmin() );
    double ymax_dst=__min ( realBbox.ymax,pImageOut->getYmax() );
    
    BoundingBox<double> bbox_dst ( xmin_dst, ymin_dst, xmax_dst, ymax_dst );

    /* Nous avons maintenant les limites de l'image réechantillonée. N'oublions pas que celle ci doit être compatible
     * avec l'image de sortie. Il faut donc modifier la bounding box afin qu'elle remplisse les conditions de compatibilité
     * (phases égales en x et en y).
     */
    makePhase(pImageOut, &bbox_dst );

    // Dimension de l'image reechantillonnee
    int width_dst = int ( ( xmax_dst-xmin_dst ) / resx_dst + 0.5 );
    int height_dst = int ( ( ymax_dst-ymin_dst ) / resy_dst + 0.5 );

    if (width_dst == 0 || height_dst == 0) {
        LOGGER_WARN("A ResampledImage's dimension would have been null");
        return true;
    }

    // On réechantillonne le masque : TOUJOURS EN PPV, sans utilisation de masque pour l'interpolation
    ResampledImage* pRMask = new ResampledImage ( pECI->Image::getMask(), width_dst, height_dst, resx_dst, resy_dst, bbox_dst,
                                                  Interpolation::NEAREST_NEIGHBOUR, false );

    // Reechantillonnage
    *ppRImage = new ResampledImage ( pECI, width_dst, height_dst, resx_dst, resy_dst, bbox_dst,
                                                   interpolation, pECI->useMasks() );

    if ( ! (*ppRImage)->setMask ( pRMask ) ) {
        LOGGER_ERROR ( "Cannot add mask to the ResampledImage" );
        return false;
    }

    return true;
}

/**
 * \~french
 * \brief Reprojette un paquet d'images compatibles
 * \details On crée l'objet ReprojectedImage correspondant à la reprojection du paquet d'images, afin de le rendre compatible avec l'image de sortie. On veut que l'emprise de l'image réechantillonnée ne dépasse ni de l'image de sortie, ni des images en entrée (sans prendre en compte les miroirs, données virtuelles).
 *
 * L'image reprojetée doit être strictement incluse dans l'image source utilisée, c'est pourquoi on va artificiellement agrandir l'image source (avec du nodata) pour être sur de l'inclusion stricte.
 *
 * L'image reprojetée peut être nulle, dans le cas où l'image source ne recouvrait pas suffisemment l'image de sortie pour permettre le calcul d'une image (une dimensions de l'image reprojetée aurait été nulle).
 * 
 * \param[in] pImageOut image résultante de l'outil
 * \param[in] pECI paquet d'images compatibles, à reprojeter
 * \param[in] ppRImage image reprojetée
 * \return VRAI si succès, FAUX sinon
 */
bool reprojectImages ( LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, ReprojectedImage** ppRImage ) {

    double resx_dst = pImageOut->getResX(), resy_dst = pImageOut->getResY();
    double resx_src = pECI->getResX(), resy_src = pECI->getResY();

    const Kernel& K = Kernel::getInstance ( interpolation );

    BoundingBox<double> BBOX_src = pECI->getBbox();

    /************ Initialisation des outils de conversion PROJ4 *********/

    std::string from_srs = pECI->getCRS().getProj4Code();
    std::string to_srs = pImageOut->getCRS().getProj4Code();
    
    projCtx ctx = pj_ctx_alloc();
    
    projPJ pj_src, pj_dst;
    if ( ! ( pj_src = pj_init_plus_ctx ( ctx, ( "+init=" + from_srs +" +wktext" ).c_str() ) ) ) {
        // Initialisation du système de projection source
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        LOGGER_ERROR ( "erreur d initialisation " << from_srs << " " << msg );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }
    
    if ( ! ( pj_dst = pj_init_plus_ctx ( ctx, ( "+init=" + to_srs +" +wktext +over" ).c_str() ) ) ) {
        // Initialisation du système de projection destination
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        LOGGER_ERROR ( "erreur d initialisation " << to_srs << " " << msg );
        pj_free ( pj_src );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    if ( BBOX_src.reproject(pj_src, pj_dst) ) {
        LOGGER_ERROR ( "Erreur reprojection bbox src -> dst" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    LOGGER_DEBUG("BBOX source reprojetée (en " << to_srs << " ) : " << BBOX_src.toString());
    LOGGER_DEBUG("      BBOX destination (en " << to_srs << " ) : " << pImageOut->getBbox().toString());

    /* On valcule les résolutions de l'image source "équivalente" dans le SRS de destination, pour pouvoir calculer le ratio
     * des réolutions pour la taille des miroirs */
    double resx_calc = (BBOX_src.xmax - BBOX_src.xmin) / double(pECI->getWidth());
    double resy_calc = (BBOX_src.ymax - BBOX_src.ymin) / double(pECI->getHeight());

    /******************** Image reprojetée : dimensions *****************/

    double xmin_dst = __max ( BBOX_src.xmin,pImageOut->getXmin() );
    double xmax_dst = __min ( BBOX_src.xmax,pImageOut->getXmax() );
    double ymin_dst = __max ( BBOX_src.ymin,pImageOut->getYmin() );
    double ymax_dst = __min ( BBOX_src.ymax,pImageOut->getYmax() );
    
    BoundingBox<double> BBOX_dst(xmin_dst,ymin_dst,xmax_dst,ymax_dst);

    /* Nous avons maintenant les limites de l'image reprojetée. N'oublions pas que celle ci doit être compatible
     * avec l'image de sortie. Il faut donc modifier la bounding box afin qu'elle remplisse les conditions de compatibilité
     * (phases égales en x et en y).
     */
    makePhase(pImageOut, &BBOX_dst );

    // Dimension de l'image reechantillonnee
    LOGGER_DEBUG("Calculated destination width (float) : " << ( xmax_dst-xmin_dst ) / resx_dst);
    LOGGER_DEBUG("Calculated destination height (float) : " << ( ymax_dst-ymin_dst ) / resy_dst);
    int width_dst = int ( ( xmax_dst-xmin_dst ) / resx_dst + 0.5 );
    int height_dst = int ( ( ymax_dst-ymin_dst ) / resy_dst + 0.5 );

    if (width_dst == 0 || height_dst == 0) {
        LOGGER_WARN("A ReprojectedImage's dimension would have been null");
        return true;
    }

    /************************ Ajout des miroirs *************************/

    double ratioX = resx_dst / resx_calc;
    double ratioY = resy_dst / resy_calc;

    // Ajout des miroirs
    int mirrorSizeX = ceil ( K.size ( ratioX ) ) + 1;
    int mirrorSizeY = ceil ( K.size ( ratioY ) ) + 1;

    int mirrorSize = std::max(mirrorSizeX, mirrorSizeY);

    LOGGER_DEBUG("Mirror's size : " << mirrorSize);

    if ( ! pECI->addMirrors ( mirrorSize ) ) {
        LOGGER_ERROR ( "Unable to add mirrors" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    /********************** Image source agrandie ***********************/

    BBOX_src = pECI->getBbox();

    if ( BBOX_src.reproject(pj_src, pj_dst) ) {
        LOGGER_ERROR ( "Erreur reprojection bbox src -> dst" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    if ( BBOX_src.reproject(pj_dst, pj_src) ) {
        LOGGER_ERROR ( "Erreur reprojection bbox dst -> src" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    makePhase(pECI, &BBOX_src );

    if ( ! pECI->changeExtent(BBOX_src) ) {
        LOGGER_ERROR ( "Unable to change the source image extent for the reprojection" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    /********************** Grille de reprojection **********************/
    
    Grid* grid = new Grid ( width_dst, height_dst, BBOX_dst );
    
    if ( ! ( grid->reproject ( to_srs, from_srs ) ) ) {
        LOGGER_ERROR ( "Bbox image invalide" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }
    
    grid->affine_transform ( 1./resx_src, -BBOX_src.xmin/resx_src - 0.5,
                             -1./resy_src, BBOX_src.ymax/resy_src - 0.5 );

    pj_free ( pj_src );
    pj_free ( pj_dst );
    pj_ctx_free ( ctx );
    pthread_mutex_unlock ( & mutex_proj );

    /*************************** Image reprojetée ***********************/

    // On  reprojete le masque : TOUJOURS EN PPV, sans utilisation de masque pour l'interpolation
    ReprojectedImage* pRMask = new ReprojectedImage ( pECI->Image::getMask(), BBOX_dst, resx_dst, resy_dst, grid,
                                                      Interpolation::NEAREST_NEIGHBOUR, false);

    //saveImage(pRMask, "/home/theo/TEST/reprojection/reprojMask.tif",8,SAMPLEFORMAT_UINT,PHOTOMETRIC_MINISBLACK,COMPRESSION_ADOBE_DEFLATE);

    // Reprojection de l'image
    *ppRImage = new ReprojectedImage ( pECI, BBOX_dst, resx_dst, resy_dst, grid, interpolation, pECI->useMasks() );

    //saveImage(pRImage, "/home/theo/TEST/reprojection/reprojImage.tif",8,SAMPLEFORMAT_UINT,PHOTOMETRIC_RGB,COMPRESSION_NONE);

    if ( ! (*ppRImage)->setMask ( pRMask ) ) {
        LOGGER_ERROR ( "Cannot add mask to the ReprojectedImage" );
        return false;
    }

    return true;
}

/**
 * \~french \brief Traite chaque paquet d'images en entrée
 * \~english \brief Treat each input images pack
 * \~french \details On a préalablement trié les images par compatibilité. Pour chaque paquet, on va créer un objet de la classe ExtendedCompoundImage. Ce dernier est à considérer comme une image simple.
 * Cette image peut être :
 * \li superposable avec l'image de sortie. Elle est directement ajoutée à une liste d'image.
 * \li non superposable avec l'image de sortie mais dans le même système de coordonnées. On va alors la réechantillonner, en utilisant la classe ResampledImage. C'est l'image réechantillonnée que l'on ajoute à la liste d'image.
 * \li non superposable avec l'image de sortie et dans un système de coordonnées différent. On va alors la reprojeter, en utilisant la classe ReprojectedImage. C'est l'image reprojetée que l'on ajoute à la liste d'image.
 *
 * \~ \image html mergeNtiff_composition.png \~french
 *
 * On obtient donc une liste d'images superposables avec celle de sortie, que l'on va réunir sous un objet de la classe ExtendedCompoundImage, qui sera la source unique utilisée pour écrire l'image de sortie.
 *
 * \~ \image html mergeNtiff_decoupe.png \~french
 *
 * Les masques sont gérés en toile de fond, en étant attachés à chacune des images manipulées.
 * \param[in] pImageOut image de sortie
 * \param[in] TabImageIn paquets d'images en entrée
 * \param[out] ppECIout paquet d'images superposable avec l'image de sortie
 * \param[in] nodata valeur de non-donnée
 * \return 0 en cas de succès, -1 en cas d'erreur
 */
int mergeTabImages ( LibtiffImage* pImageOut, // Sortie
                     std::vector<std::vector<Image*> >& TabImageIn, // Entrée
                     ExtendedCompoundImage** ppECIout, // Résultat du merge
                     int* nodata ) {
    
    ExtendedCompoundImageFactory ECIF ;
    std::vector<Image*> pOverlayedImages;

    pthread_mutex_lock ( & mutex_proj );

    for ( unsigned int i=0; i<TabImageIn.size(); i++ ) {
        LOGGER_DEBUG ( "Pack " << i << " : " << TabImageIn.at ( i ).size() << " image(s)" );
        // Mise en superposition du paquet d'images en 2 etapes

        // Etape 1 : Creation d'une image composite (avec potentiellement une seule image)

        ExtendedCompoundImage* pECI = ECIF.createExtendedCompoundImage ( TabImageIn.at ( i ),nodata,0 );
        if ( pECI==NULL ) {
            LOGGER_ERROR ( "Impossible d'assembler les images" );
            return -1;
        }
        pECI->setCRS(TabImageIn.at(i).at(0)->getCRS());

        ExtendedCompoundMask* pECMI = new ExtendedCompoundMask ( pECI );
        if ( ! pECI->setMask ( pECMI ) ) {
            LOGGER_ERROR ( "Cannot add mask to the Image's pack " << i );
            return -1;
        }

        pECMI->setCRS(TabImageIn.at(i).at(0)->getCRS());

        if ( pImageOut->isCompatibleWith ( pECI ) ) {
            LOGGER_DEBUG ( "\t is compatible" );
            /* les images sources et finale ont la meme resolution et la meme phase
             * on aura donc pas besoin de reechantillonnage.*/
            pOverlayedImages.push_back ( pECI );

        } else if (pECI->getCRS() == pImageOut->getCRS()) {

            LOGGER_DEBUG ( "\t need a resampling" );

            ResampledImage* pResampledImage = NULL;

            if ( ! resampleImages ( pImageOut, pECI, &pResampledImage ) ) {
                LOGGER_ERROR ( "Cannot resample images' pack" );
                return -1;
            }

            if ( pResampledImage == NULL ) {
                LOGGER_WARN ( "No resampled image to add" );
            } else {
                pOverlayedImages.push_back ( pResampledImage );
            }
                
        } else {
            
            LOGGER_DEBUG ( "\t need a reprojection" );

            ReprojectedImage* pReprojectedImage = NULL;

            if ( ! reprojectImages ( pImageOut, pECI, &pReprojectedImage ) ) {
                LOGGER_ERROR ( "Cannot reproject images' pack" );
                return -1;
            }
             
            if ( pReprojectedImage == NULL ) {
                LOGGER_WARN ( "No reprojected image to add" );
            } else {
                pOverlayedImages.push_back ( pReprojectedImage );
            }
        }

    }

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    *ppECIout = ECIF.createExtendedCompoundImage (
                    pImageOut->getWidth(), pImageOut->getHeight(), pImageOut->channels, pImageOut->getBbox(),
                    pOverlayedImages, nodata,0 );

    if ( *ppECIout == NULL ) {
        LOGGER_ERROR ( "Cannot create final compounded image." );
        return -1;
    }

    // Masque
    ExtendedCompoundMask* pECMIout = new ExtendedCompoundMask ( *ppECIout );

    if ( ! ( *ppECIout )->setMask ( pECMIout ) ) {
        LOGGER_ERROR ( "Cannot add mask to the main Extended Compound Image" );
        return -1;
    }

    return 0;
}

/**
 ** \~french
 * \brief Fonction principale de l'outil mergeNtiff
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 si réussi, -1 sinon
 ** \~english
 * \brief Main function for tool mergeNtiff
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return 0 if success, -1 otherwise
 */
int main ( int argc, char **argv ) {

    LibtiffImage* pImageOut ;
    LibtiffImage* pMaskOut = NULL;
    std::vector<LibtiffImage*> ImageIn;
    std::vector<std::vector<Image*> > TabImageIn;
    ExtendedCompoundImage* pECI;

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
    if ( parseCommandLine ( argc, argv ) < 0 ) {
        error ( "Echec lecture ligne de commande",-1 );
    }

    // Conversion string->int[] du paramètre nodata
    LOGGER_DEBUG ( "Nodata interpretation" );
    int nodata[samplesperpixel];

    char* charValue = strtok ( strnodata,"," );
    if ( charValue == NULL ) {
        error ( "Error with option -n : a value for nodata is missing",-1 );
    }
    nodata[0] = atoi ( charValue );
    for ( int i = 1; i < samplesperpixel; i++ ) {
        charValue = strtok ( NULL, "," );
        if ( charValue == NULL ) {
            error ( "Error with option -n : a value for nodata is missing",-1 );
        }
        nodata[i] = atoi ( charValue );
    }

    LOGGER_DEBUG ( "Load" );
    // Chargement des images
    if ( loadImages ( &pImageOut,&pMaskOut,&ImageIn ) < 0 ) {
        error ( "Echec chargement des images",-1 );
    }

    LOGGER_DEBUG ( "Check images" );
    // Controle des images
    if ( checkImages ( pImageOut,ImageIn ) < 0 ) {
        error ( "Echec controle des images",-1 );
    }

    LOGGER_DEBUG ( "Sort" );
    // Tri des images
    if ( sortImages ( ImageIn, &TabImageIn ) < 0 ) {
        error ( "Echec tri des images",-1 );
    }

    LOGGER_DEBUG ( "Merge" );
    // Fusion des paquets d images
    if ( mergeTabImages ( pImageOut, TabImageIn, &pECI, nodata ) < 0 ) {
        error ( "Echec fusion des paquets d images",-1 );
    }

    LOGGER_DEBUG ( "Save image" );
    // Enregistrement de l'image fusionnée
    if ( pImageOut->writeImage ( pECI ) < 0 ) {
        error ( "Echec enregistrement de l image finale",-1 );
    }

    if (pMaskOut != NULL) {
        LOGGER_DEBUG ( "Save mask" );
        // Enregistrement du masque fusionné, si demandé
        if (pMaskOut->writeImage ( pECI->Image::getMask() ) < 0 ) {
            error ( "Echec enregistrement du masque final",-1 );
        }
    }

    LOGGER_DEBUG ( "Clean" );
    // Nettoyage
    pj_clear_initcache();
    delete acc;
    delete pECI;
    delete pImageOut;
    delete pMaskOut;

    return 0;
}
