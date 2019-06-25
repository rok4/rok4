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
 * \file composeNtiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Montage de N images TIFF aux mêmes dimensions et caractéristiques
 * \~english \brief Monte N TIFF images with same dimensions and attributes
 */

#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "tiffio.h"
#include "tiff.h"
#include "Logger.h"
#include "LibtiffImage.h"
#include "CompoundImage.h"
#include "Format.h"
#include "math.h"
#include "../../../rok4version.h"

/** \~french Nombre d'images dans le sens de la largeur */
int widthwiseImage = 0;
/** \~french Nombre d'images dans le sens de la hauteur */
int heightwiseImage = 0;

/** \~french Compression de l'image de sortie */
Compression::eCompression compression = Compression::NONE;

/** \~french Dossier des images sources */
char* inputDir = 0;

/** \~french Chemin de l'image en sortie */
char* outputImage = 0;

/** \~french Activation du niveau de log debug. Faux par défaut */
bool debugLogger=false;

/** \~french Message d'usage de la commande pbf2cache */
std::string help = std::string("\ncomposeNtiff version ") + std::string(ROK4_VERSION) + "\n\n"
    "Monte N TIFF image, forming a regular grid\n\n"

    "Usage: composeNtiff -s <DIRECTORY> -g <VAL> <VAL> -c <VAL> <OUTPUT FILE>\n\n"

    "Parameters:\n"
    "     -s source directory. All file into have to be images. If too much images are present, first are used.\n"
    "     -c output compression : default value : none\n"
    "             raw     no compression\n"
    "             none    no compression\n"
    "             jpg     Jpeg encoding\n"
    "             lzw     Lempel-Ziv & Welch encoding\n"
    "             pkb     PackBits encoding\n"
    "             zip     Deflate encoding\n"
    "     -g number of images, widthwise and heightwise, to compose the final image\n"
    "     -d debug logger activation\n\n"

    "Example\n"
    "     composeNtiff -s /home/ign/sources -g 10 10 -c zip output.tif\n\n";
    

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande composeNtiff #help
 * \details L'affichage se fait dans le niveau de logger INFO
 */
void usage() {
    LOGGER_INFO (help);
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur
 * \param[in] message message d'erreur
 * \param[in] errorCode code de retour
 */
void error ( std::string message, int errorCode ) {
    LOGGER_ERROR ( message );
    LOGGER_ERROR ( "Source directory : " << inputDir );
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
            case 's': // Input directory
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Error id -s option" );
                    return -1;
                }
                inputDir = argv[i];
                break;
            case 'c': // compression
                if ( ++i == argc ) {
                    LOGGER_ERROR("Error in -c option" );
                    return -1;
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
                    LOGGER_ERROR ( "Unknown compression : " + argv[i][1] );
                    return -1;
                }
                break;
            case 'g':
                if ( i+2 >= argc ) {
                    LOGGER_ERROR ( "Error in -g option" );
                    return -1;
                }
                widthwiseImage = atoi ( argv[++i] );
                heightwiseImage = atoi ( argv[++i] );
                break;
            default:
                LOGGER_ERROR ( "Unknown option : " << argv[i] );
                return -1;
            }
        } else {
            if ( outputImage == 0 ) outputImage = argv[i];
            else {
                LOGGER_ERROR( "Argument must specify just ONE output file" );
                return -1;
            }
        }
    }

    // Input directory control
    if ( inputDir == 0 ) {
        LOGGER_ERROR ( "We need to have a source images' directory (option -s)" );
        return -1;
    }

    // Output file control
    if ( outputImage == 0 ) {
        LOGGER_ERROR ( "We need to have an output file" );
        return -1;
    }

    // Geometry control
    if ( widthwiseImage == 0 || heightwiseImage == 0) {
        LOGGER_ERROR ( "We need to know composition geometry (option -g)" );
        return -1;
    }

    return 0;
}


/**
 * \~french
 * \brief Charge les images contenues dans le dossier en entrée et l'image de sortie
 * \details Toutes les images doivent avoir les mêmes caractéristiques, dimensions et type des canaux. Les images en entrée seront gérée par un objet de la classe #CompoundImage, et l'image en sortie sera une image TIFF.
 *
 * \param[out] ppImageOut image résultante de l'outil
 * \param[out] ppCompoundIn ensemble des images en entrée
 * \return code de retour, 0 si réussi, -1 sinon
 */
int loadImages ( FileImage** ppImageOut, CompoundImage** ppCompoundIn ) {

    std::vector< std::string > imagesNames;
    
    std::vector< std::vector<Image*> > imagesIn;

    // Dimensionnement de imagesIn
    imagesIn.resize(heightwiseImage);
    for (int row = 0; row < heightwiseImage; row++)
        imagesIn.at(row).resize(widthwiseImage);

    int width, height;
    int samplesperpixel, bitspersample;
    SampleFormat::eSampleFormat sampleformat;
    Photometric::ePhotometric photometric;

    FileImageFactory FIF;


    /********* Parcours du dossier ************/

    // Ouverture et test du répertoire source
    DIR * rep = opendir(inputDir);

    if (rep == NULL) {
        LOGGER_ERROR("Cannot open input directory : " << inputDir);
        return -1;
    }
    
    struct dirent * ent;

     // On récupère tous les fichiers, puis on les trie
    while ((ent = readdir(rep)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        imagesNames.push_back(std::string(ent->d_name));
    }

    closedir(rep);
    
    LOGGER_DEBUG(imagesNames.size() << " files in the provided directory");
    if (imagesNames.size() > widthwiseImage*heightwiseImage) {
        LOGGER_WARN("We have too much images in the input directory (regarding to the provided geometry).");
        LOGGER_WARN("Only " << widthwiseImage*heightwiseImage << " first images will be used");
    }

    if (imagesNames.size() < widthwiseImage*heightwiseImage) {
        LOGGER_ERROR("Not enough images, we need " << widthwiseImage*heightwiseImage << ", and we find " << imagesNames.size());
        return -1;
    }

    std::sort(imagesNames.begin(), imagesNames.end());

    /********* Chargement des images ************/
    
    /* On doit connaître les dimensions des images en entrée pour pouvoir créer les images de sortie
     * Lecture et création des images sources */
    for (int k = 0; k < widthwiseImage*heightwiseImage; k++) {

        int i = k/widthwiseImage;
        int j = k%widthwiseImage;

        std::string str = inputDir + imagesNames.at(k);
        char filename[256];
        memset(filename, 0, 256);
        memcpy(filename, str.c_str(), str.length());

        FileImage* pImage = FIF.createImageToRead (filename, BoundingBox<double>(0,0,0,0), -1., -1. );
        if ( pImage == NULL ) {
            LOGGER_ERROR ( "Cannot create a FileImage from the file " << filename );
            return -1;
        }

        if ( i == 0 && j == 0 ) {
            // C'est notre première image en entrée, on mémorise les caractéristiques
            bitspersample = pImage->getBitsPerSample();
            sampleformat = pImage->getSampleFormat();
            photometric = pImage->getPhotometric();
            samplesperpixel = pImage->getChannels();
            width = pImage->getWidth();
            height = pImage->getHeight();
        } else {
            // Toutes les images en entrée doivent avoir certaines caractéristiques en commun
            if ( bitspersample != pImage->getBitsPerSample() ||
                 sampleformat != pImage->getSampleFormat() ||
                 photometric != pImage->getPhotometric() ||
                 samplesperpixel != pImage->getChannels() ||
                 width != pImage->getWidth() ||
                 height != pImage->getHeight() )
            {
                LOGGER_ERROR ( "All input images must have same dimensions and sample type : error for image " << filename );
                return -1;
            }
        }

        pImage->setBbox(BoundingBox<double>(j * width, (heightwiseImage - i - 1) * height, (j+1) * width, (heightwiseImage - i) * height));
        
        imagesIn[i][j] = pImage;
        
    }

    *ppCompoundIn = new CompoundImage(imagesIn);

    // Création de l'image de sortie
    *ppImageOut = FIF.createImageToWrite (
        outputImage, BoundingBox<double>(0., 0., 0., 0.), -1., -1., width*widthwiseImage, height*heightwiseImage, samplesperpixel,
        sampleformat, bitspersample, photometric,compression
    );

    if ( *ppImageOut == NULL ) {
        LOGGER_ERROR ( "Impossible de creer l'image de sortie " << outputImage );
        return -1;
    }

    return 0;
}

/**
 ** \~french
 * \brief Fonction principale de l'outil composeNtiff
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return code de retour, 0 si réussi, -1 sinon
 ** \~english
 * \brief Main function for tool composeNtiff
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return 0 if success, -1 otherwise
 */
int main ( int argc, char **argv ) {

    FileImage* pImageOut ;
    CompoundImage* pCompoundIn;

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
    if ( parseCommandLine ( argc,argv ) < 0 ) {
        error ( "Cannot parse command line",-1 );
    }

    // On sait maintenant si on doit activer le niveau de log DEBUG
    if (debugLogger) {
        Logger::setAccumulator(DEBUG, acc);
        std::ostream &logd = LOGGER ( DEBUG );
        logd.precision ( 16 );
        logd.setf ( std::ios::fixed,std::ios::floatfield );
    }

    LOGGER_DEBUG ( "Load" );
    // Chargement des images
    if ( loadImages ( &pImageOut, &pCompoundIn ) < 0 ) {
        error ( "Cannot load images from the input directory",-1 );
    }

    LOGGER_DEBUG ( "Save image" );
    // Enregistrement de l'image fusionnée
    if ( pImageOut->writeImage ( pCompoundIn ) < 0 ) {
        error ( "Cannot write the compound image",-1 );
    }

    Logger::stopLogger();
    acc->stop();
    acc->destroy();
    delete acc;
    delete pCompoundIn;
    delete pImageOut;

    return 0;
}
