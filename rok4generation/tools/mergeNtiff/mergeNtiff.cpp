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
 * \file mergeNtiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Création d'une image TIFF géoréférencée à partir de n images sources géoréférencées
 * \~english \brief Create one georeferenced TIFF image from several georeferenced images
 * \~ \image html mergeNtiff.png \~french
 *
 * La légende utilisée dans tous les schémas de la documentation de ce fichier sera la suivante
 * \~ \image html mergeNtiff_legende.png \~french
 *
 * Pour réaliser la fusion des images en entrée, on traite différemment :
 * \li les images qui sont superposables à l'image de sortie (même SRS, mêmes résolutions, mêmes phases) : on parle alors d'images compatibles, pas de réechantillonnage nécessaire.
 * \li les images non compatibles mais de même SRS : un passage par le réechantillonnage (plus lourd en calcul) est indispensable.
 * \li les images non compatibles et de SRS différents : un passage par la reprojection (encore plus lourd en calcul) est indispensable.
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

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
namespace logging = boost::log;
namespace keywords = boost::log::keywords;

#include "FileImage.h"
#include "ResampledImage.h"
#include "ReprojectedImage.h"
#include "ExtendedCompoundImage.h"
#include "PixelConverter.h"

#include "CRS.h"
#include "Interpolation.h"
#include "Format.h"
#include "math.h"
#include "../../../rok4version.h"

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

/** \~french Valeur de nodata sous forme de chaîne de caractère (passée en paramètre de la commande) */
char strnodata[256];

/** \~french A-t-on précisé le format en sortie, c'est à dire les 3 informations samplesperpixel, bitspersample et sampleformat */
bool outputProvided = false;
/** \~french Nombre de canaux par pixel, pour l'image en sortie */
uint16_t samplesperpixel = 0;
/** \~french Nombre de bits occupé par un canal, pour l'image en sortie */
uint16_t bitspersample = 0;
/** \~french Format du canal (entier, flottant, signé ou non...), pour l'image en sortie */
SampleFormat::eSampleFormat sampleformat = SampleFormat::UNKNOWN;

/** \~french Photométrie (rgb, gray), déduit du nombre de canaux */
Photometric::ePhotometric photometric;

/** \~french Compression de l'image de sortie */
Compression::eCompression compression = Compression::NONE;
/** \~french Interpolation utilisée pour le réechantillonnage ou la reprojection */
Interpolation::KernelType interpolation = Interpolation::CUBIC;

/** \~french Activation du niveau de log debug. Faux par défaut */
bool debugLogger=false;

/** \~french Message d'usage de la commande mergeNtiff */
std::string help = std::string("\nmergeNtiff version ") + std::string(ROK4_VERSION) + "\n\n"

    "Create one georeferenced TIFF image from several georeferenced TIFF images.\n\n"

    "Usage: mergeNtiff -f <FILE> [-r <DIR>] -c <VAL> -i <VAL> -n <VAL> [-a <VAL> -s <VAL> -b <VAL>]\n"

    "Parameters:\n"
    "    -f configuration file : list of output and source images and masks\n"
    "    -r output root : root directory for output files, have to end with a '/'\n"
    "    -c output compression :\n"
    "            raw     no compression\n"
    "            none    no compression\n"
    "            jpg     Jpeg encoding (rate 75)\n"
    "            jpg90   Jpeg encoding (rate 90)\n"
    "            lzw     Lempel-Ziv & Welch encoding\n"
    "            pkb     PackBits encoding\n"
    "            zip     Deflate encoding\n"
    "    -i interpolation : used for resampling :\n"
    "            nn nearest neighbor\n"
    "            linear\n"
    "            bicubic\n"
    "            lanczos lanczos 3\n"
    "    -n nodata value, one interger per sample, seperated with comma. Examples\n"
    "            -99999 for DTM\n"
    "            255,255,255 for orthophotography\n"
    "    -a sample format : (float or uint)\n"
    "    -b bits per sample : (8 or 32)\n"
    "    -s samples per pixel : (1, 2, 3 or 4)\n"
    "    -d debug logger activation\n\n"

    "If bitspersample, sampleformat or samplesperpixel are not provided, those 3 informations are read from the image sources (all have to own the same). If 3 are provided, conversion may be done.\n\n"

    "Examples\n"
    "    - for orthophotography\n"
    "    mergeNtiff -f conf.txt -c zip -i bicubic -n 255,255,255\n"
    "    - for DTM\n"
    "    mergeNtiff -f conf.txt -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999\n\n";

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande mergeNtiff #help
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

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
            case 'd': // debug logs
                debugLogger = true;
                break;
            case 'f': // fichier de liste des images source
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -f" ;
                    return -1;
                }
                strcpy ( imageListFilename,argv[i] );
                break;
            case 'r': // racine pour le fichier de configuration
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -r" ;
                    return -1;
                }
                strcpy ( outImagesRoot,argv[i] );
                break;
            case 'i': // interpolation
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -i" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "lanczos",7 ) == 0 ) interpolation = Interpolation::LANCZOS_3;
                else if ( strncmp ( argv[i], "nn",2 ) == 0 ) interpolation = Interpolation::NEAREST_NEIGHBOUR;
                else if ( strncmp ( argv[i], "bicubic",7 ) == 0 ) interpolation = Interpolation::CUBIC;
                else if ( strncmp ( argv[i], "linear",6 ) == 0 ) interpolation = Interpolation::LINEAR;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for option -i : " << argv[i] ;
                    return -1;
                }
                break;
            case 'n': // nodata
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -n" ;
                    return -1;
                }
                strcpy ( strnodata,argv[i] );
                break;
            case 'c': // compression
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -c" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = Compression::DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = Compression::PACKBITS;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = Compression::JPEG;
                else if ( strncmp ( argv[i], "jpg90",3 ) == 0 ) compression = Compression::JPEG90;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = Compression::LZW;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for option -c : " << argv[i] ;
                    return -1;
                }
                break;

            /****************** OPTIONNEL, POUR FORCER DES CONVERSIONS **********************/
            case 's': // samplesperpixel
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -s" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                else if ( strncmp ( argv[i], "2",1 ) == 0 ) samplesperpixel = 2 ;
                else if ( strncmp ( argv[i], "3",1 ) == 0 ) samplesperpixel = 3 ;
                else if ( strncmp ( argv[i], "4",1 ) == 0 ) samplesperpixel = 4 ;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for option -s : " << argv[i] ;
                    return -1;
                }
                break;
            case 'b': // bitspersample
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -b" ;
                    return -1;
                }
                if ( strncmp ( argv[i], "8",1 ) == 0 ) bitspersample = 8 ;
                else if ( strncmp ( argv[i], "32",2 ) == 0 ) bitspersample = 32 ;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for option -b : " << argv[i] ;
                    return -1;
                }
                break;
            case 'a': // sampleformat
                if ( i++ >= argc ) {
                    BOOST_LOG_TRIVIAL(error) <<  "Error in option -a" ;
                    return -1;
                }
                if ( strncmp ( argv[i],"uint",4 ) == 0 ) sampleformat = SampleFormat::UINT ;
                else if ( strncmp ( argv[i],"float",5 ) == 0 ) sampleformat = SampleFormat::FLOAT;
                else {
                    BOOST_LOG_TRIVIAL(error) <<  "Unknown value for option -a : " << argv[i] ;
                    return -1;
                }
                break;
            /*******************************************************************************/

            default:
                BOOST_LOG_TRIVIAL(error) <<  "Unknown option : -" << argv[i][1] ;
                return -1;
            }
        }
    }

    BOOST_LOG_TRIVIAL(debug) <<  "mergeNtiff -f " << imageListFilename ;

    return 0;
}

/**
 * \~french
 * \brief Lit l'ensemble de la configuration
 * \details On parse la ligne courante du fichier de configuration, en stockant les valeurs dans les variables fournies. On saute les lignes vides. On lit ensuite la ligne suivante :
 * \li si elle correspond à un masque, on complète les informations
 * \li si elle ne correspond pas à un masque, on recule le pointeur
 *
 * \param[in,out] masks Indicateurs de présence d'un masque
 * \param[in,out] paths Chemins des images
 * \param[in,out] srss Systèmes de coordonnées des images
 * \param[in,out] bboxes Rectangles englobant des images
 * \param[in,out] resxs Résolution en x des images
 * \param[in,out] resys Résolution en y des images
 * \return true en cas de succès, false si échec
 */
bool loadConfiguration ( 
    std::vector<bool>* masks, 
    std::vector<char* >* paths, 
    std::vector<std::string>* srss, 
    std::vector<BoundingBox<double> >* bboxes,
    std::vector<double>* resxs,
    std::vector<double>* resys
) {

    std::ifstream file;
    int rootLength = strlen ( outImagesRoot );

    file.open ( imageListFilename );
    if ( ! file.is_open() ) {
        BOOST_LOG_TRIVIAL(error) <<  "Impossible d'ouvrir le fichier " << imageListFilename ;
        return false;
    }

    while ( file.good() ) {
        char tmpPath[IMAGE_MAX_FILENAME_LENGTH];
        char tmpCRS[20];
        char line[2*IMAGE_MAX_FILENAME_LENGTH];
        memset ( line, 0, 2*IMAGE_MAX_FILENAME_LENGTH );
        memset ( tmpPath, 0, IMAGE_MAX_FILENAME_LENGTH );
        memset ( tmpCRS, 0, 20 );

        char type[3];
        std::string crs;
        BoundingBox<double> bb(0.,0.,0.,0.);
        double resx, resy;
        bool isMask;

        file.getline(line, 2*IMAGE_MAX_FILENAME_LENGTH);
        if ( strlen(line) == 0 ) {
            continue;
        }
        int nb = std::sscanf ( line,"%s %s %s %lf %lf %lf %lf %lf %lf", type, tmpPath, tmpCRS, &bb.xmin, &bb.ymax, &bb.xmax, &bb.ymin, &resx, &resy );
        if ( nb == 9 && memcmp ( type,"IMG",3 ) == 0) {
            // On lit la ligne d'une image
            crs.assign ( tmpCRS );
            isMask = false;
        }
        else if ( nb == 2 && memcmp ( type,"MSK",3 ) == 0) {
            // On lit la ligne d'un masque
            isMask = true;

            if (masks->size() == 0 || masks->back()) {
                // La première ligne ne peut être un masque et on ne peut pas avoir deux masques à la suite
                BOOST_LOG_TRIVIAL(error) <<  "A MSK line have to follow an IMG line" ;
                BOOST_LOG_TRIVIAL(error) <<  "\t line : " << line ;   
                return false;             
            }
        }
        else {
            BOOST_LOG_TRIVIAL(error) <<  "We have to read 9 values for IMG or 2 for MSK" ;
            BOOST_LOG_TRIVIAL(error) <<  "\t line : " << line ;
            return false;
        }

        char* path = (char*) malloc(IMAGE_MAX_FILENAME_LENGTH);
        memset ( path, 0, IMAGE_MAX_FILENAME_LENGTH );

        if ( ! strncmp ( tmpPath,"?",1 ) ) {
            strcpy ( path, outImagesRoot );
            strcpy ( & ( path[rootLength] ),& ( tmpPath[1] ) );
        } else {
            strcpy ( path,tmpPath );
        }

        // On ajoute tout ça dans les vecteurs
        masks->push_back(isMask);
        paths->push_back(path);
        srss->push_back(crs);
        bboxes->push_back(bb);
        resxs->push_back(resx);
        resys->push_back(resy);

    }

    if (file.eof()) {
        BOOST_LOG_TRIVIAL(debug) << "Fin du fichier de configuration atteinte";
        file.close();
        return true;
    } else {
        BOOST_LOG_TRIVIAL(error) << "Failure reading the configuration file " << imageListFilename;
        file.close();
        return false;
    }
}

/**
 * \~french
 * \brief Charge les images en entrée et en sortie depuis le fichier de configuration
 * \details On va récupérer toutes les informations de toutes les images et masques présents dans le fichier de configuration et créer les objets FileImage correspondant. Toutes les images ici manipulées sont de vraies images (physiques) dans ce sens où elles sont des fichiers soit lus, soit qui seront écrits.
 *
 * Le chemin vers le fichier de configuration est stocké dans la variables globale imageListFilename et outImagesRoot va être concaténer au chemin vers les fichiers de sortie.
 * \param[out] ppImageOut image résultante de l'outil
 * \param[out] ppMaskOut masque résultat de l'outil, si demandé
 * \param[out] pImageIn ensemble des images en entrée
 * \return code de retour, 0 si réussi, -1 sinon
 */
int loadImages ( FileImage** ppImageOut, FileImage** ppMaskOut, std::vector<FileImage*>* pImageIn ) {


    std::vector<bool> masks;
    std::vector<char*> paths;
    std::vector<std::string> srss;
    std::vector<BoundingBox<double> > bboxes;
    std::vector<double> resxs;
    std::vector<double> resys;

    if (! loadConfiguration(&masks, &paths, &srss, &bboxes, &resxs, &resys) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot load configuration file " << imageListFilename ;
        return -1;
    }

    // On doit avoir au moins deux lignes, trois si on a un masque de sortie
    if (masks.size() < 2 || (masks.size() == 2 && masks.back()) ) {
        BOOST_LOG_TRIVIAL(error) <<  "We have no input images in configuration file " << imageListFilename ;
        return -1;
    }

    // On va charger les images en entrée en premier pour avoir certaines informations
    int firstInput = 1;
    if (masks.at(1)) {
        // La deuxième ligne est le masque de sortie
        firstInput = 2;
    }

    /****************** LES ENTRÉES : CRÉATION ******************/

    FileImageFactory factory;
    int nbImgsIn = 0;

    for ( int i = firstInput; i < masks.size(); i++ ) {

        nbImgsIn++;
        BOOST_LOG_TRIVIAL(debug) << "Input " << nbImgsIn;

        if ( resxs.at(i) == 0. || resys.at(i) == 0.) {
            BOOST_LOG_TRIVIAL(error) <<  "Source image " << nbImgsIn << " is not valid (resolutions)" ;
            return -1;
        }

        CRS crs;
        crs.setRequestCode ( srss.at(i) );

        if ( ! crs.validateBBox ( bboxes.at(i) ) ) {
            BOOST_LOG_TRIVIAL(debug) << "Warning : the input image's (" << paths.at(i) << ") bbox (" << bboxes.at(i).toString() << ") is not included in the srs (" << srss.at(i) << ") definition extent";
        }

        FileImage* pImage=factory.createImageToRead ( paths.at(i), bboxes.at(i), resxs.at(i), resys.at(i) );
        if ( pImage == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Impossible de creer une image a partir de " << paths.at(i) ;
            return -1;
        }
        pImage->setCRS ( crs );
        free(paths.at(i));

        if ( i+1 < masks.size() && masks.at(i+1) ) {
            
            FileImage* pMask=factory.createImageToRead ( paths.at(i+1), bboxes.at(i), resxs.at(i), resys.at(i) );
            if ( pMask == NULL ) {
                BOOST_LOG_TRIVIAL(error) <<  "Impossible de creer un masque a partir de " << paths.at(i) ;
                return -1;
            }
            pMask->setCRS ( crs );

            if ( ! pImage->setMask ( pMask ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask to the input FileImage" ;
                return -1;
            }
            i++;
            free(paths.at(i));
        }

        pImageIn->push_back ( pImage );

        /* On vérifie que le format des canaux est le même pour toutes les images en entrée :
         *     - sampleformat
         *     - bitspersample
         *     - samplesperpixel
         */

        if (! outputProvided && nbImgsIn == 1) {
            /* On n'a pas précisé de format en sortie, on va donc utiliser celui des entrées
             * On veut donc avoir le même format pour toutes les entrées
             * On lit la première image en entrée, qui sert de référence
             * L'image en sortie sera à ce format
             */
            bitspersample = pImage->getBitsPerSample();
            samplesperpixel = pImage->getChannels();
            sampleformat = pImage->getSampleFormat();
        } else if (! outputProvided) {
            // On doit avoir le même format pour tout le monde
            if (bitspersample != pImage->getBitsPerSample()) {
                BOOST_LOG_TRIVIAL(error) << "We don't provided output format, so all inputs have to own the same" ;
                BOOST_LOG_TRIVIAL(error) << "The first image and the " << nbImgsIn << " one don't have the same number of bits per sample" ;
                BOOST_LOG_TRIVIAL(error) << bitspersample << " != " << pImage->getBitsPerSample() ;
            }
            if (samplesperpixel != pImage->getChannels()) {
                BOOST_LOG_TRIVIAL(error) << "We don't provided output format, so all inputs have to own the same" ;
                BOOST_LOG_TRIVIAL(error) << "The first image and the " << nbImgsIn << " one don't have the same number of samples per pixel" ;
                BOOST_LOG_TRIVIAL(error) << samplesperpixel << " != " << pImage->getChannels() ;
            }
            if (sampleformat != pImage->getSampleFormat()) {
                BOOST_LOG_TRIVIAL(error) << "We don't provided output format, so all inputs have to own the same" ;
                BOOST_LOG_TRIVIAL(error) << "The first image and the " << nbImgsIn << " one don't have the same sample format" ;
                BOOST_LOG_TRIVIAL(error) << sampleformat << " != " << pImage->getSampleFormat() ;
            }
        }
    }

    if ( pImageIn->size() == 0 ) {
        BOOST_LOG_TRIVIAL(error) <<  "Erreur lecture du fichier de parametres '" << imageListFilename << "' : pas de données en entrée." ;
        return -1;
    } else {
        BOOST_LOG_TRIVIAL(debug) <<  nbImgsIn << " image(s) en entrée" ;
    }

    /********************** LA SORTIE : CRÉATION *************************/

    if (samplesperpixel == 1) {
        photometric = Photometric::GRAY;
    } else if (samplesperpixel == 2) {
        photometric = Photometric::GRAY;
    } else {
        photometric = Photometric::RGB;
    }

    CRS outCrs ( srss.at(0) );

    // Arrondi a la valeur entiere la plus proche
    int width = lround ( ( bboxes.at(0).xmax - bboxes.at(0).xmin ) / ( resxs.at(0) ) );
    int height = lround ( ( bboxes.at(0).ymax - bboxes.at(0).ymin ) / ( resys.at(0) ) );

    *ppImageOut = factory.createImageToWrite (
        paths.at(0), bboxes.at(0), resxs.at(0), resys.at(0), width, height,
        samplesperpixel, sampleformat, bitspersample, photometric, compression
    );

    if ( *ppImageOut == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "Impossible de creer l'image " << paths.at(0) ;
        return -1;
    }

    ( *ppImageOut )->setCRS ( outCrs );
    free(paths.at(0));

    if ( firstInput == 2 ) {

        *ppMaskOut = factory.createImageToWrite (
            paths.at(1), bboxes.at(0), resxs.at(0), resys.at(0), width, height,
            1, SampleFormat::UINT, 8, Photometric::MASK, Compression::DEFLATE
        );

        if ( *ppMaskOut == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Impossible de creer le masque " << paths.at(1) ;
            return -1;
        }

        ( *ppMaskOut )->setCRS ( outCrs );
        free(paths.at(1));
    }

    if (debugLogger) ( *ppImageOut )->print();

    return 0;
}

int addConverters(std::vector<FileImage*> ImageIn) {
    if (! outputProvided) {
        // On n'a pas précisé de format en sortie, donc toutes les images doivent avoir le même
        // Et la sortie a aussi ce format, donc pas besoin de convertisseur

        return 0;
    }

    for ( std::vector<FileImage*>::iterator itImg = ImageIn.begin(); itImg < ImageIn.end(); itImg++ ) {

        if ( ! ( *itImg )->addConverter ( sampleformat, bitspersample, samplesperpixel ) ) {
            BOOST_LOG_TRIVIAL(error) << "Cannot add converter for an input image";
            ( *itImg )->print();
            return -1;
        }

        if (debugLogger) ( *itImg )->print();
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
int sortImages ( std::vector<FileImage*> ImageIn, std::vector<std::vector<Image*> >* pTabImageIn ) {
    std::vector<Image*> vTmpImg;
    std::vector<FileImage*>::iterator itiniImg = ImageIn.begin();

    /* we create consistent images' vectors (X/Y resolution and X/Y phases)
     * Masks are moved in parallel with images
     */
    int toto = 0;
    for ( std::vector<FileImage*>::iterator itImg = ImageIn.begin(); itImg < ImageIn.end()-1; itImg++ ) {

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
 * \details Les 4 valeurs du rectangle englobant seront modifiée (au minimum) afin d'avoir les mêmes phases que celles de l'image fournie. Cependant, l'étendue finale sera incluse dans celle initiale (pas d'agrandissement de la bounding box).
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

    BOOST_LOG_TRIVIAL(debug) <<  "\t Bbox avant rephasage : " << bbox->toString() ;

    // Mise en phase de xmin (sans que celui ci puisse être plus petit)
    phi = modf ( bbox->xmin/resx_dst, &intpart );
    if ( phi < 0. ) {
        phi += 1.0;
    }

    if ( fabs ( phi-phaseX ) > 0.0001 && fabs ( phi-phaseX ) < 0.9999 ) {
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

    if ( fabs ( phi-phaseX ) > 0.0001 && fabs ( phi-phaseX ) < 0.9999 ) {
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

    if ( fabs ( phi-phaseY ) > 0.0001 && fabs ( phi-phaseY ) < 0.9999 ) {
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

    if ( fabs ( phi-phaseY ) > 0.0001 && fabs ( phi-phaseY ) < 0.9999 ) {
        phaseDiff = phaseY - phi;
        if ( phaseDiff > 0. ) {
            phaseDiff -= 1.0;
        }
        bbox->ymax += phaseDiff*resy_dst;
    }

    BOOST_LOG_TRIVIAL(debug) <<  "\t Bbox après rephasage : " << bbox->toString() ;
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
bool resampleImages ( FileImage* pImageOut, ExtendedCompoundImage* pECI, ResampledImage** ppRImage ) {

    double resx_dst = pImageOut->getResX(), resy_dst = pImageOut->getResY();

    const Kernel& K = Kernel::getInstance ( interpolation );

    // Ajout des miroirs
    // Valeurs utilisées pour déterminer la taille des miroirs en pixel (taille optimale en fonction du noyau utilisé)
    int mirrorSizeX = ceil ( K.size ( resx_dst / pECI->getResX() ) ) + 1;
    int mirrorSizeY = ceil ( K.size ( resy_dst / pECI->getResY() ) ) + 1;

    int mirrorSize = std::max ( mirrorSizeX, mirrorSizeY );

    BOOST_LOG_TRIVIAL(debug) << "\t Mirror's size : " << mirrorSize;


    // On mémorise la bbox d'origine, sans les miroirs
    BoundingBox<double> realBbox = pECI->getBbox();

    if ( ! pECI->addMirrors ( mirrorSize ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to add mirrors" ;
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
    makePhase ( pImageOut, &bbox_dst );

    // Dimension de l'image reechantillonnee
    int width_dst = int ( ( bbox_dst.xmax-bbox_dst.xmin ) / resx_dst + 0.5 );
    int height_dst = int ( ( bbox_dst.ymax-bbox_dst.ymin ) / resy_dst + 0.5 );

    if ( width_dst <= 0 || height_dst <= 0 ) {
        BOOST_LOG_TRIVIAL(warning) <<  "A ResampledImage's dimension would have been null" ;
        return true;
    }

    // On réechantillonne le masque : TOUJOURS EN PPV, sans utilisation de masque pour l'interpolation
    ResampledImage* pRMask = new ResampledImage ( pECI->Image::getMask(), width_dst, height_dst, resx_dst, resy_dst, bbox_dst,
            Interpolation::NEAREST_NEIGHBOUR, false );
    
    // Reechantillonnage
    *ppRImage = new ResampledImage ( pECI, width_dst, height_dst, resx_dst, resy_dst, bbox_dst,
                                     interpolation, pECI->useMasks() );
    

    if ( ! ( *ppRImage )->setMask ( pRMask ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask to the ResampledImage" ;
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
bool reprojectImages ( FileImage* pImageOut, ExtendedCompoundImage* pECI, ReprojectedImage** ppRImage ) {

    double resx_dst = pImageOut->getResX(), resy_dst = pImageOut->getResY();
    double resx_src = pECI->getResX(), resy_src = pECI->getResY();

    const Kernel& K = Kernel::getInstance ( interpolation );

    BoundingBox<double> tmpBbox = pECI->getBbox();

    /************ Initialisation des outils de conversion PROJ4 *********/

    std::string from_srs = pECI->getCRS().getProj4Code();
    std::string to_srs = pImageOut->getCRS().getProj4Code();

    /******** Conversion de la bbox source dans le srs de sortie ********/
    /******************* et calcul des ratios des résolutions ***********/

    /* On fait particulièrement attention à ne considérer que la partie valide de la bbox source
     * c'est à dire la partie incluse dans l'espace de définition du SRS
     * On va donc la "croper" */
    BoundingBox<double> cropSourceBbox = pECI->getCRS().cropBBox(tmpBbox);

    int cropWidth = ceil((cropSourceBbox.xmax - cropSourceBbox.xmin) / resx_src);
    int cropHeight = ceil((cropSourceBbox.ymax - cropSourceBbox.ymin) / resy_src);

    if ( cropSourceBbox.reproject(from_srs, to_srs) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Erreur reprojection bbox src -> dst" ;
        return false;
    }

    /* On valcule les résolutions de l'image source "équivalente" dans le SRS de destination, pour pouvoir calculer le ratio
     * des résolutions pour la taille des miroirs */
    double resx_calc = (cropSourceBbox.xmax - cropSourceBbox.xmin) / double(cropWidth);
    double resy_calc = (cropSourceBbox.ymax - cropSourceBbox.ymin) / double(cropHeight);

    /******************** Image reprojetée : dimensions *****************/

    /* On fait particulièrement attention à ne considérer que la partie valide de la bbox finale
     * c'est à dire la partie incluse dans l'espace de définition du SRS
     * On va donc la "croper" */
    BoundingBox<double> cropFinalBbox = pImageOut->getCRS().cropBBox(pImageOut->getBbox());

    double xmin_dst = __max ( cropSourceBbox.xmin,cropFinalBbox.xmin );
    double xmax_dst = __min ( cropSourceBbox.xmax,cropFinalBbox.xmax );
    double ymin_dst = __max ( cropSourceBbox.ymin,cropFinalBbox.ymin );
    double ymax_dst = __min ( cropSourceBbox.ymax,cropFinalBbox.ymax );

    BoundingBox<double> BBOX_dst ( xmin_dst,ymin_dst,xmax_dst,ymax_dst );

    BOOST_LOG_TRIVIAL(debug) <<  "        BBOX dst (srs destination) : " << BBOX_dst.toString() ;

    /* Nous avons maintenant les limites de l'image reprojetée. N'oublions pas que celle ci doit être compatible
     * avec l'image de sortie. Il faut donc modifier la bounding box afin qu'elle remplisse les conditions de compatibilité
     * (phases égales en x et en y).
     */
    makePhase ( pImageOut, &BBOX_dst );

    // Dimension de l'image reechantillonnee
    BOOST_LOG_TRIVIAL(debug) <<  "        Calculated destination width (float) : " << ( BBOX_dst.xmax - BBOX_dst.xmin ) / resx_dst ;
    BOOST_LOG_TRIVIAL(debug) <<  "        Calculated destination height (float) : " << ( BBOX_dst.ymax - BBOX_dst.ymin ) / resy_dst ;
    int width_dst = int ( ( BBOX_dst.xmax - BBOX_dst.xmin ) / resx_dst + 0.5 );
    int height_dst = int ( ( BBOX_dst.ymax - BBOX_dst.ymin ) / resy_dst + 0.5 );

    if ( width_dst <= 0 || height_dst <= 0 ) {
        BOOST_LOG_TRIVIAL(warning) <<  "A ReprojectedImage's dimension would have been null" ;
        return true;
    }

    tmpBbox = BBOX_dst;

    if ( tmpBbox.reproject ( to_srs, from_srs ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Erreur reprojection bbox dst en srs src" ;
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) <<  "        BBOX dst (srs source) : " << tmpBbox.toString() ;
    BOOST_LOG_TRIVIAL(debug) <<  "        BBOX source : " << pECI->getBbox().toString() ;
    

    /************************ Ajout des miroirs *************************/

    double ratioX = resx_dst / resx_calc;
    double ratioY = resy_dst / resy_calc;

    // Ajout des miroirs
    int mirrorSizeX = ceil ( K.size ( ratioX ) ) + 1;
    int mirrorSizeY = ceil ( K.size ( ratioY ) ) + 1;

    int mirrorSize = 2 * std::max ( mirrorSizeX, mirrorSizeY );

    BOOST_LOG_TRIVIAL(debug) <<  "        Mirror's size : " << mirrorSize ;

    if ( ! pECI->addMirrors ( mirrorSize ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to add mirrors" ;
        return false;
    }

    BOOST_LOG_TRIVIAL(debug) <<  "        BBOX source avec miroir : " << pECI->getBbox().toString() ;

    /********************** Image source agrandie ***********************/

    /* L'image à reprojeter n'est pas intégralement contenue dans l'image source. Cela va poser des problèmes lors de l'interpolation :
     * ReprojectedImage va vouloir accéder à des coordonnées pixel négatives -> segmentation fault.
     * Pour éviter cela, on va agrandir artificiellemnt l'étendue de l'image source (avec du nodata) */
    if ( ! pECI->extendBbox ( tmpBbox, mirrorSize ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to extend the source image extent for the reprojection" ;
        return false;
    }
    BOOST_LOG_TRIVIAL(debug) <<  "        BBOX source agrandie : " << pECI->getBbox().toString() ;

    /********************** Grille de reprojection **********************/

    Grid* grid = new Grid ( width_dst, height_dst, BBOX_dst );

    if ( ! ( grid->reproject ( to_srs, from_srs ) ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Bbox image invalide" ;
        return false;
    }

    grid->affine_transform ( 1./resx_src, -pECI->getBbox().xmin/resx_src - 0.5,
                             -1./resy_src, pECI->getBbox().ymax/resy_src - 0.5 );

    /*************************** Image reprojetée ***********************/

    // On  reprojete le masque : TOUJOURS EN PPV, sans utilisation de masque pour l'interpolation
    ReprojectedImage* pRMask = new ReprojectedImage ( pECI->Image::getMask(), BBOX_dst, resx_dst, resy_dst, grid,
            Interpolation::NEAREST_NEIGHBOUR, false );
    pRMask->setCRS(pImageOut->getCRS());

    // Reprojection de l'image    
    *ppRImage = new ReprojectedImage ( pECI, BBOX_dst, resx_dst, resy_dst, grid, interpolation, pECI->useMasks() );
    (*ppRImage)->setCRS(pImageOut->getCRS());

    if ( ! ( *ppRImage )->setMask ( pRMask ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask to the ReprojectedImage" ;
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
int mergeTabImages ( FileImage* pImageOut, // Sortie
                     std::vector<std::vector<Image*> >& TabImageIn, // Entrée
                     ExtendedCompoundImage** ppECIout, // Résultat du merge
                     int* nodata ) {

    ExtendedCompoundImageFactory ECIF ;
    std::vector<Image*> pOverlayedImages;

    for ( unsigned int i=0; i<TabImageIn.size(); i++ ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Pack " << i << " : " << TabImageIn.at ( i ).size() << " image(s)" ;
        // Mise en superposition du paquet d'images en 2 etapes

        // Etape 1 : Creation d'une image composite (avec potentiellement une seule image)

        ExtendedCompoundImage* pECI = ECIF.createExtendedCompoundImage ( TabImageIn.at ( i ),nodata,0 );
        if ( pECI == NULL ) {
            BOOST_LOG_TRIVIAL(error) <<  "Impossible d'assembler les images" ;
            return -1;
        }
        pECI->setCRS ( TabImageIn.at ( i ).at ( 0 )->getCRS() );

        ExtendedCompoundMask* pECMI = new ExtendedCompoundMask ( pECI );
        pECMI->setCRS ( TabImageIn.at ( i ).at ( 0 )->getCRS() );
        if ( ! pECI->setMask ( pECMI ) ) {
            BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask to the Image's pack " << i ;
            return -1;
        }

        if ( pImageOut->isCompatibleWith ( pECI ) ) {
            BOOST_LOG_TRIVIAL(debug) <<  "\t is compatible" ;
            /* les images sources et finale ont la meme resolution et la meme phase
             * on aura donc pas besoin de reechantillonnage.*/
            pOverlayedImages.push_back ( pECI );

        } else if ( pECI->getCRS() == pImageOut->getCRS() ) {

            BOOST_LOG_TRIVIAL(debug) <<  "\t need a resampling" ;

            ResampledImage* pResampledImage = NULL;

            if ( ! resampleImages ( pImageOut, pECI, &pResampledImage ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot resample images' pack" ;
                return -1;
            }

            if ( pResampledImage == NULL ) {
                BOOST_LOG_TRIVIAL(warning) <<  "No resampled image to add" ;
            } else {
                pOverlayedImages.push_back ( pResampledImage );
            }

        } else {

            BOOST_LOG_TRIVIAL(debug) <<  "\t need a reprojection" ;

            ReprojectedImage* pReprojectedImage = NULL;

            if ( ! reprojectImages ( pImageOut, pECI, &pReprojectedImage ) ) {
                BOOST_LOG_TRIVIAL(error) <<  "Cannot reproject images' pack" ;
                return -1;
            }

            if ( pReprojectedImage == NULL ) {
                BOOST_LOG_TRIVIAL(warning) <<  "No reprojected image to add" ;
            } else {
                pOverlayedImages.push_back ( pReprojectedImage );
            }
        }

    }

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    *ppECIout = ECIF.createExtendedCompoundImage (
                    pImageOut->getWidth(), pImageOut->getHeight(), pImageOut->getChannels(), pImageOut->getBbox(),
                    pOverlayedImages, nodata,0 );

    if ( *ppECIout == NULL ) {
        for (int i = 0; i < pOverlayedImages.size(); i++) delete pOverlayedImages.at(i);
        BOOST_LOG_TRIVIAL(error) <<  "Cannot create final compounded image." ;
        return -1;
    }

    // Masque
    ExtendedCompoundMask* pECMIout = new ExtendedCompoundMask ( *ppECIout );

    if ( ! ( *ppECIout )->setMask ( pECMIout ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Cannot add mask to the main Extended Compound Image" ;
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

    FileImage* pImageOut = NULL;
    FileImage* pMaskOut = NULL;
    std::vector<FileImage*> ImageIn;
    std::vector<std::vector<Image*> > TabImageIn;
    ExtendedCompoundImage* pECI = NULL;

    /* Initialisation des Loggers */
    boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::info );
    logging::add_common_attributes();
    boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
    logging::add_console_log (
        std::cout,
        keywords::format = "%Severity%\t%Message%"
    );

    // Lecture des parametres de la ligne de commande
    if ( parseCommandLine ( argc, argv ) < 0 ) {
        error ( "Echec lecture ligne de commande",-1 );
    }

    // On sait maintenant si on doit activer le niveau de log DEBUG
    if (debugLogger) {
        boost::log::core::get()->set_filter( boost::log::trivial::severity >= boost::log::trivial::debug );
    }

    // On regarde si on a tout précisé en sortie, pour voir si des conversions sont possibles
    if (sampleformat != SampleFormat::UNKNOWN && bitspersample != 0 && samplesperpixel !=0) {
      outputProvided = true;
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Load" ;
    // Chargement des images
    if ( loadImages ( &pImageOut,&pMaskOut,&ImageIn ) < 0 ) {
        if (pECI) delete pECI;
        if (pImageOut) delete pImageOut;
        if (pMaskOut) delete pMaskOut;
        error ( "Echec chargement des images",-1 );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Add converters" ;
    // Ajout des modules de conversion aux images en entrée
    if ( addConverters ( ImageIn ) < 0 ) {
        if (pECI) delete pECI;
        if (pImageOut) delete pImageOut;
        if (pMaskOut) delete pMaskOut;
        error ( "Echec ajout des convertisseurs", -1 );
    }

    // Maintenant que l'on a la valeur de samplesperpixel, on peut lire le nodata
    // Conversion string->int[] du paramètre nodata
    BOOST_LOG_TRIVIAL(debug) <<  "Nodata interpretation" ;
    int nodata[samplesperpixel];

    char* charValue = strtok ( strnodata,"," );
    if ( charValue == NULL ) {
        if (pECI) delete pECI;
        if (pImageOut) delete pImageOut;
        if (pMaskOut) delete pMaskOut;
        error ( "Error with option -n : a value for nodata is missing",-1 );
    }
    nodata[0] = atoi ( charValue );
    for ( int i = 1; i < samplesperpixel; i++ ) {
        charValue = strtok ( NULL, "," );
        if ( charValue == NULL ) {
            if (pECI) delete pECI;
            if (pImageOut) delete pImageOut;
            if (pMaskOut) delete pMaskOut;
            error ( "Error with option -n : one value per sample, separate with comma",-1 );
        }
        nodata[i] = atoi ( charValue );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Sort" ;
    // Tri des images
    if ( sortImages ( ImageIn, &TabImageIn ) < 0 ) {
        if (pECI) delete pECI;
        if (pImageOut) delete pImageOut;
        if (pMaskOut) delete pMaskOut;
        error ( "Echec tri des images",-1 );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Merge" ;
    // Fusion des paquets d images
    if ( mergeTabImages ( pImageOut, TabImageIn, &pECI, nodata ) < 0 ) {
        if (pECI) delete pECI;
        if (pImageOut) delete pImageOut;
        if (pMaskOut) delete pMaskOut;
        error ( "Echec fusion des paquets d images",-1 );
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Save image" ;
    // Enregistrement de l'image fusionnée
    if ( pImageOut->writeImage ( pECI ) < 0 ) {
        if (pECI) delete pECI;
        if (pImageOut) delete pImageOut;
        if (pMaskOut) delete pMaskOut;
        error ( "Echec enregistrement de l image finale",-1 );
    }

    if ( pMaskOut != NULL ) {
        BOOST_LOG_TRIVIAL(debug) <<  "Save mask" ;
        // Enregistrement du masque fusionné, si demandé
        if ( pMaskOut->writeImage ( pECI->Image::getMask() ) < 0 ) {
            if (pECI) delete pECI;
            if (pImageOut) delete pImageOut;
            if (pMaskOut) delete pMaskOut;
            error ( "Echec enregistrement du masque final",-1 );
        }
    }

    BOOST_LOG_TRIVIAL(debug) <<  "Clean" ;
    // Nettoyage
    pj_clear_initcache();
    delete pECI;
    delete pImageOut;
    delete pMaskOut;

    return 0;
}
