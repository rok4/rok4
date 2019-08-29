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
 * \file decimateNtiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Création d'une image TIFF géoréférencée à partir de n images sources géoréférencées
 * \~english \brief Create one georeferenced TIFF image from several georeferenced images
 * \~ \image html decimateNtiff.png \~french
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
 * Pour réaliser la fusion des images en entrée, on traite différemment :
 * \li les images qui sont superposables à l'image de sortie (même SRS, mêmes résolutions, mêmes phases) : on parle alors d'images compatibles, pas de réechantillonnage nécessaire.
 * \li les images non compatibles mais de même SRS : un passage par le réechantillonnage (plus lourd en calcul) est indispensable.
 * \li les images non compatibles et de SRS différents : un passage par la reprojection (encore plus lourd en calcul) est indispensable.
 *
 * Exemple d'appel à la commande :
 * \li pour des ortho-images \~english \li for orthoimage
 * \~ \code
 * decimateNtiff -f conf.txt -r /home/ign/results/ -c zip -i bicubic -s 3 -b 8 -p rgb -a uint -n 255,255,255
 * \endcode
 * \~french \li pour du MNT \~english \li for DTM
 * \~ \code
 * decimateNtiff -f conf.txt -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999
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
#include "Logger.h"

#include "FileImage.h"
#include "DecimatedImage.h"
#include "ExtendedCompoundImage.h"

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
/** \~french Valeur de nodata sour forme de chaîne de caractère (passée en paramètre de la commande) */
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
Compression::eCompression compression;

/** \~french Activation du niveau de log debug. Faux par défaut */
bool debugLogger=false;

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande decimateNtiff
 * \details L'affichage se fait dans le niveau de logger INFO
 * \~ \code
 * decimateNtiff version X.X.X
 *
 * Usage: decimateNtiff -f <FILE> -c <VAL> -n <VAL> 
 *
 * Parameters:
 *      -f configuration file : list of output and source images and masks
 *      -c output compression :
 *              raw     no compression
 *              none    no compression
 *              jpg     Jpeg encoding
 *              lzw     Lempel-Ziv & Welch encoding
 *              pkb     PackBits encoding
 *              zip     Deflate encoding
 *      -n nodata value, one interger per sample, seperated with comma. Examples
 *              -99999 for DTM
 *              255,255,255 for orthophotography
 *      -a sample format : (float or uint)
 *      -b bits per sample : (8 or 32)
 *      -s samples per pixel : (1, 2, 3 or 4)
 *      -d debug logger activation
 *
 * If bitspersample, sampleformat or samplesperpixel are not provided, those 3 informations are read from the image sources (all have to own the same). If 3 are provided, conversion may be done.
 *
 * \endcode
 */
void usage() {
    LOGGER_INFO ( "\ndecimateNtiff version " << ROK4_VERSION << "\n\n" <<

                  "Create one georeferenced TIFF image from several georeferenced TIFF images.\n\n" <<

                  "Usage: decimateNtiff -f <FILE> -c <VAL> -n <VAL> [-d] [-h]\n" <<

                  "Parameters:\n" <<
                  "    -f configuration file : list of output and source images and masks\n" <<
                  "    -c output compression :\n" <<
                  "            raw     no compression\n" <<
                  "            none    no compression\n" <<
                  "            jpg     Jpeg encoding\n" <<
                  "            lzw     Lempel-Ziv & Welch encoding\n" <<
                  "            pkb     PackBits encoding\n" <<
                  "            zip     Deflate encoding\n" <<
                  "    -n nodata value, one interger per sample, seperated with comma. Examples\n" <<
                  "            -99999 for DTM\n" <<
                  "            255,255,255 for orthophotography\n" <<
                  "    -a sample format : (float or uint)\n" <<
                  "    -b bits per sample : (8 or 32)\n" <<
                  "    -s samples per pixel : (1, 2, 3 or 4)\n" <<
                  "    -d debug logger activation\n\n" <<

                  "If bitspersample, sampleformat or samplesperpixel are not provided, those 3 informations are read from the image sources (all have to own the same). If 3 are provided, conversion may be done.\n\n");
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
                    LOGGER_ERROR ( "Error in option -f" );
                    return -1;
                }
                strcpy ( imageListFilename,argv[i] );
                break;
            case 'n': // nodata
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -n" );
                    return -1;
                }
                strcpy ( strnodata,argv[i] );
                break;
            case 'c': // compression
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -c" );
                    return -1;
                }
                if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = Compression::DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = Compression::PACKBITS;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = Compression::JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = Compression::LZW;
                else {
                    LOGGER_ERROR ( "Unknown value for option -c : " << argv[i] );
                    return -1;
                }
                break;

            /****************** OPTIONNEL, POUR FORCER DES CONVERSIONS **********************/
            case 's': // samplesperpixel
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error in option -s" );
                    return -1;
                }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) samplesperpixel = 1 ;
                else if ( strncmp ( argv[i], "2",1 ) == 0 ) samplesperpixel = 2 ;
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
                if ( strncmp ( argv[i],"uint",4 ) == 0 ) sampleformat = SampleFormat::UINT ;
                else if ( strncmp ( argv[i],"float",5 ) == 0 ) sampleformat = SampleFormat::FLOAT;
                else {
                    LOGGER_ERROR ( "Unknown value for option -a : " << argv[i] );
                    return -1;
                }
                break;
            /*******************************************************************************/

            default:
                LOGGER_ERROR ( "Unknown option : -" << argv[i][1] );
                return -1;
            }
        }
    }

    LOGGER_DEBUG ( "decimateNtiff -f " << imageListFilename );

    return 0;
}

/**
 * \~french
 * \brief Lit l'ensemble de la configuration
 *
 * \param[in,out] masks Indicateurs de présence d'un masque
 * \param[in,out] paths Chemins des images
 * \param[in,out] bboxes Rectangles englobant des images
 * \param[in,out] resxs Résolution en x des images
 * \param[in,out] resys Résolution en y des images
 * \return true en cas de succès, false si échec
 */
bool loadConfiguration ( 
    std::vector<bool>* masks, 
    std::vector<char* >* paths, 
    std::vector<BoundingBox<double> >* bboxes,
    std::vector<double>* resxs,
    std::vector<double>* resys
) {

    std::ifstream file;

    file.open ( imageListFilename );
    if ( ! file.is_open() ) {
        LOGGER_ERROR ( "Impossible d'ouvrir le fichier " << imageListFilename );
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
        BoundingBox<double> bb(0.,0.,0.,0.);
        double resx, resy;
        bool isMask;

        file.getline(line, 2*IMAGE_MAX_FILENAME_LENGTH);
        LOGGER_DEBUG(line);  
        if ( strlen(line) == 0 ) {
            continue;
        }
        int nb = std::sscanf ( line,"%s %s %lf %lf %lf %lf %lf %lf", type, tmpPath, &bb.xmin, &bb.ymax, &bb.xmax, &bb.ymin, &resx, &resy );
        if ( nb == 8 && memcmp ( type,"IMG",3 ) == 0) {
            // On lit la ligne d'une image
            isMask = false;
        }
        else if ( nb == 2 && memcmp ( type,"MSK",3 ) == 0) {
            // On lit la ligne d'un masque
            isMask = true;

            if (masks->size() == 0 || masks->back()) {
                // La première ligne ne peut être un masque et on ne peut pas avoir deux masques à la suite
                LOGGER_ERROR ( "A MSK line have to follow an IMG line" );
                LOGGER_ERROR ( "\t line : " << line );   
                return false;             
            }
        }
        else {
            LOGGER_ERROR ( "We have to read 8 values for IMG or 2 for MSK" );
            LOGGER_ERROR ( "\t line : " << line );
            return false;
        }

        char* path = (char*) malloc(IMAGE_MAX_FILENAME_LENGTH);
        memset ( path, 0, IMAGE_MAX_FILENAME_LENGTH );
        strcpy ( path,tmpPath );

        // On ajoute tout ça dans les vecteurs
        masks->push_back(isMask);
        paths->push_back(path);
        bboxes->push_back(bb);
        resxs->push_back(resx);
        resys->push_back(resy);

    }

    if (file.eof()) {
        LOGGER_DEBUG("Fin du fichier de configuration atteinte");
        file.close();
        return true;
    } else {
        LOGGER_ERROR("Failure reading the configuration file " << imageListFilename);
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
 * \param[out] pImagesIn ensemble des images en entrée
 * \return code de retour, 0 si réussi, -1 sinon
 */
int loadImages ( FileImage** ppImageOut, FileImage** ppMaskOut, std::vector<FileImage*>* pImagesIn ) {
    
    std::vector<bool> masks;
    std::vector<char*> paths;
    std::vector<BoundingBox<double> > bboxes;
    std::vector<double> resxs;
    std::vector<double> resys;

    if (! loadConfiguration(&masks, &paths, &bboxes, &resxs, &resys) ) {
        LOGGER_ERROR ( "Cannot load configuration file " << imageListFilename );
        return -1;
    }

    // On doit avoir au moins deux lignes, trois si on a un masque de sortie
    if (masks.size() < 2 || (masks.size() == 2 && masks.back()) ) {
        LOGGER_ERROR ( "We have no input images in configuration file " << imageListFilename );
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
        LOGGER_DEBUG("image " << paths.at(i));

        nbImgsIn++;
        LOGGER_DEBUG("Input " << nbImgsIn);

        if ( resxs.at(i) == 0. || resys.at(i) == 0.) {
            LOGGER_ERROR ( "Source image " << nbImgsIn << " is not valid (resolutions)" );
            return -1;
        }

        FileImage* pImage=factory.createImageToRead ( paths.at(i), bboxes.at(i), resxs.at(i), resys.at(i) );
        if ( pImage == NULL ) {
            LOGGER_ERROR ( "Impossible de creer une image a partir de " << paths.at(i) );
            return -1;
        }

        delete paths.at(i);

        if ( i+1 < masks.size() && masks.at(i+1) ) {
            
            FileImage* pMask=factory.createImageToRead ( paths.at(i+1), bboxes.at(i), resxs.at(i), resys.at(i) );
            if ( pMask == NULL ) {
                LOGGER_ERROR ( "Impossible de creer un masque a partir de " << paths.at(i) );
                return -1;
            }

            if ( ! pImage->setMask ( pMask ) ) {
                LOGGER_ERROR ( "Cannot add mask to the input FileImage" );
                return -1;
            }
            i++;
            delete paths.at(i);
        }

        pImagesIn->push_back ( pImage );

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
                LOGGER_ERROR("We don't provided output format, so all inputs have to own the same" );
                LOGGER_ERROR("The first image and the " << nbImgsIn << " one don't have the same number of bits per sample" );
                LOGGER_ERROR(bitspersample << " != " << pImage->getBitsPerSample() );
            }
            if (samplesperpixel != pImage->getChannels()) {
                LOGGER_ERROR("We don't provided output format, so all inputs have to own the same" );
                LOGGER_ERROR("The first image and the " << nbImgsIn << " one don't have the same number of samples per pixel" );
                LOGGER_ERROR(samplesperpixel << " != " << pImage->getChannels() );
            }
            if (sampleformat != pImage->getSampleFormat()) {
                LOGGER_ERROR("We don't provided output format, so all inputs have to own the same" );
                LOGGER_ERROR("The first image and the " << nbImgsIn << " one don't have the same sample format" );
                LOGGER_ERROR(sampleformat << " != " << pImage->getSampleFormat() );
            }
        }
    }

    if ( pImagesIn->size() == 0 ) {
        LOGGER_ERROR ( "Erreur lecture du fichier de parametres '" << imageListFilename << "' : pas de données en entrée." );
        return -1;
    } else {
        LOGGER_DEBUG( nbImgsIn << " image(s) en entrée" );
    }

    /********************** LA SORTIE : CRÉATION *************************/

    if (samplesperpixel == 1) {
        photometric = Photometric::GRAY;
    } else if (samplesperpixel == 2) {
        photometric = Photometric::GRAY;
    } else {
        photometric = Photometric::RGB;
    }

    // Arrondi a la valeur entiere la plus proche
    int width = lround ( ( bboxes.at(0).xmax - bboxes.at(0).xmin ) / ( resxs.at(0) ) );
    int height = lround ( ( bboxes.at(0).ymax - bboxes.at(0).ymin ) / ( resys.at(0) ) );

    *ppImageOut = factory.createImageToWrite (
        paths.at(0), bboxes.at(0), resxs.at(0), resys.at(0), width, height,
        samplesperpixel, sampleformat, bitspersample, photometric, compression
    );

    if ( *ppImageOut == NULL ) {
        LOGGER_ERROR ( "Impossible de creer l'image " << paths.at(0) );
        return -1;
    }

    delete paths.at(0);

    if ( firstInput == 2 ) {

        *ppMaskOut = factory.createImageToWrite (
            paths.at(1), bboxes.at(0), resxs.at(0), resys.at(0), width, height,
            1, SampleFormat::UINT, 8, Photometric::MASK, Compression::DEFLATE
        );

        if ( *ppMaskOut == NULL ) {
            LOGGER_ERROR ( "Impossible de creer le masque " << paths.at(1) );
            return -1;
        }

        delete paths.at(1);
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
            LOGGER_ERROR("Cannot add converter for an input image");
            ( *itImg )->print();
            return -1;
        }

        if (debugLogger) ( *itImg )->print();
    }

    return 0;

}

/**
 * \~french
 * \brief Trie les images sources
 * \details La première image en entrée peut être une image de fond : elle est alors compatible avec l'image en sortie
 * Les images suivantes sont les images à "décimer", dont on ne veut garder qu'un pixel sur N. Elles sont compatibles entre elles mais pas
 * avec l'image en sortie (et donc pas avec l'image de fond).
 * 
 * On doit donc avoir à la fin soit un paquet, soit deux avec une seule image dans le premier (le fond)
 *
 * \param[in] ImagesIn images en entrée
 * \param[out] pTabImageIn images en entrée, triées en paquets compatibles
 * \return code de retour, 0 si réussi, -1 sinon
 */
int sortImages ( std::vector<FileImage*> ImagesIn, std::vector<std::vector<Image*> >* pTabImageIn ) {
    std::vector<Image*> vTmpImg;
    std::vector<FileImage*>::iterator itiniImg = ImagesIn.begin();

    /* we create consistent images' vectors (X/Y resolution and X/Y phases)
     * Masks are moved in parallel with images
     */
    for ( std::vector<FileImage*>::iterator itImg = ImagesIn.begin(); itImg < ImagesIn.end()-1; itImg++ ) {

        if ( ! ( *itImg )->isCompatibleWith ( * ( itImg+1 ) ) ) {
            // two following images are not compatible, we split images' vector
            vTmpImg.assign ( itiniImg,itImg+1 );
            itiniImg = itImg+1;
            pTabImageIn->push_back ( vTmpImg );
        }
    }

    // we don't forget to store last images in pTabImageIn
    // images
    vTmpImg.assign ( itiniImg,ImagesIn.end() );
    pTabImageIn->push_back ( vTmpImg );
    
    if (! (pTabImageIn->size() == 1 || pTabImageIn->size() == 2) ) {
        LOGGER_ERROR("Input images have to constitute 1 or 2 (the background) consistent images' pack");
        return -1;
    }
    
    if (pTabImageIn->size() == 2) {
        if (pTabImageIn->at(0).size() != 1) {
            LOGGER_ERROR("If a background image is present, no another consistent image with it (one image pack)");
            return -1;
        }
    }

    return 0;
}


/**
 * \~french \brief Traite chaque paquet d'images en entrée
 * \~english \brief Treat each input images pack
 * \~french \details 
 *
 * \param[in] pImageOut image de sortie
 * \param[in] TabImagesIn paquets d'images en entrée
 * \param[out] ppECIout paquet d'images superposable avec l'image de sortie
 * \param[in] nodata valeur de non-donnée
 * \return 0 en cas de succès, -1 en cas d'erreur
 */
int mergeTabImages ( FileImage* pImageOut, // Sortie
                     std::vector<std::vector<Image*> >& TabImagesIn, // Entrée
                     ExtendedCompoundImage** ppECIout, // Résultat du merge
                     int* nodata ) {

    ExtendedCompoundImageFactory ECIF ;
    DecimatedImageFactory DIF ;
    std::vector<Image*> pOverlayedImages;
    
    
    // ************* Le fond (éventuel)
    if ( TabImagesIn.size() == 2) {
        LOGGER_DEBUG("We have a background");
        if ( ! TabImagesIn.at(0).at(0)->isCompatibleWith ( pImageOut ) ) {
            LOGGER_ERROR ( "Background image have to be consistent with the output image" );
            TabImagesIn.at(0).at(0)->print();
            LOGGER_ERROR("not consistent with");
            pImageOut->print();
            return -1;
        }
        pOverlayedImages.push_back ( TabImagesIn.at(0).at(0) );
    }
    
    // ************* Les images à décimer

    // L'image
    ExtendedCompoundImage* pECI = ECIF.createExtendedCompoundImage ( TabImagesIn.at(TabImagesIn.size() - 1), nodata, 0 );
    if ( pECI == NULL ) {
        LOGGER_ERROR ( "Impossible d'assembler les images en entrée" );
        return -1;
    }
    
    DecimatedImage* pDII = DIF.createDecimatedImage(pECI, pImageOut->getBbox(), pImageOut->getResX(), pImageOut->getResY(), nodata);
    if ( pDII == NULL ) {
        LOGGER_ERROR ( "Impossible de créer la DecimatedImage (image)" );
        return -1;
    }

    // Le masque
    ExtendedCompoundMask* pECMI = new ExtendedCompoundMask ( pECI );
    if ( ! pECI->setMask ( pECMI ) ) {
        LOGGER_ERROR ( "Cannot add mask to the compound image " );
        return -1;
    }
    
    int nodata_mask[1] = {0};
    DecimatedImage* pDIM = DIF.createDecimatedImage(pECMI, pImageOut->getBbox(), pImageOut->getResX(), pImageOut->getResY(), nodata_mask);
    if ( pDIM == NULL ) {
        LOGGER_ERROR ( "Impossible de créer la DecimatedImage (mask)" );
        return -1;
    }
    
    if ( ! pDII->setMask ( pDIM ) ) {
        LOGGER_ERROR ( "Cannot add mask to the DecimatedImage" );
        return -1;
    }
    
    if (debugLogger) pECI->print();
    if (debugLogger) pDII->print();
    
    pOverlayedImages.push_back(pDII);

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    *ppECIout = ECIF.createExtendedCompoundImage (
        pImageOut->getWidth(), pImageOut->getHeight(), pImageOut->getChannels(), pImageOut->getBbox(),
        pOverlayedImages, nodata, 0
    );

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

    FileImage* pImageOut ;
    FileImage* pMaskOut = NULL;
    std::vector<FileImage*> ImagesIn;
    std::vector<std::vector<Image*> > TabImagesIn;
    ExtendedCompoundImage* pECI;

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

    // Lecture des parametres de la ligne de commande
    if ( parseCommandLine ( argc, argv ) < 0 ) {
        error ( "Echec lecture ligne de commande",-1 );
    }

    // On sait maintenant si on doit activer le niveau de log DEBUG
    if (debugLogger) {
        Logger::setAccumulator(DEBUG, acc);
        std::ostream &logd = LOGGER ( DEBUG );
        logd.precision ( 16 );
        logd.setf ( std::ios::fixed,std::ios::floatfield );
    }

    // On regarde si on a tout précisé en sortie, pour voir si des conversions sont possibles
    if (sampleformat != SampleFormat::UNKNOWN && bitspersample != 0 && samplesperpixel !=0) {
      outputProvided = true;
    }

    LOGGER_DEBUG ( "Load" );
    // Chargement des images
    if ( loadImages ( &pImageOut, &pMaskOut, &ImagesIn ) < 0 ) {
        error ( "Echec chargement des images",-1 );
    }

    LOGGER_DEBUG ( "Add converters" );
    // Ajout des modules de conversion aux images en entrée
    if ( addConverters ( ImagesIn ) < 0 ) {
        error ( "Echec ajout des convertisseurs", -1 );
    }
    
    // Conversion string->int[] du paramètre nodata
    LOGGER_DEBUG ( "Nodata interpretation" );
    int spp = ImagesIn.at(0)->getChannels();
    int nodata[spp];

    char* charValue = strtok ( strnodata,"," );
    if ( charValue == NULL ) {
        error ( "Error with option -n : a value for nodata is missing",-1 );
    }
    nodata[0] = atoi ( charValue );
    for ( int i = 1; i < spp; i++ ) {
        charValue = strtok ( NULL, "," );
        if ( charValue == NULL ) {
            error ( "Error with option -n : a value for nodata is missing",-1 );
        }
        nodata[i] = atoi ( charValue );
    }

    LOGGER_DEBUG ( "Sort" );
    // Tri des images
    if ( sortImages ( ImagesIn, &TabImagesIn ) < 0 ) {
        error ( "Echec tri des images",-1 );
    }

    LOGGER_DEBUG ( "Merge" );
    // Fusion des paquets d images
    if ( mergeTabImages ( pImageOut, TabImagesIn, &pECI, nodata ) < 0 ) {
        error ( "Echec fusion des paquets d images",-1 );
    }

    LOGGER_DEBUG ( "Save image" );
    // Enregistrement de l'image fusionnée
    if ( pImageOut->writeImage ( pECI ) < 0 ) {
        error ( "Echec enregistrement de l image finale",-1 );
    }

    if ( pMaskOut != NULL ) {
        LOGGER_DEBUG ( "Save mask" );
        // Enregistrement du masque fusionné, si demandé
        if ( pMaskOut->writeImage ( pECI->Image::getMask() ) < 0 ) {
            error ( "Echec enregistrement du masque final",-1 );
        }
    }

    LOGGER_DEBUG ( "Clean" );
    // Nettoyage
    acc->stop();
    acc->destroy();
    delete acc;
    delete pECI;
    delete pImageOut;
    delete pMaskOut;

    return 0;
}

