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
 * \file merge4tiff.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Sous echantillonage de 4 images disposées en carré, avec utilisation possible de fond et de masques de données
 * \~english \brief Four images subsampling, formed a square, might use a background and data masks
 * \~ \image html merge4tiff.png
 */

#include "tiffio.h"
#include "Image.h"
#include "Format.h"
#include "FileImage.h"
#include "Logger.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdint.h>
#include "../../../rok4version.h"

/* Valeurs de nodata */
/** \~french Valeur de nodata sour forme de chaîne de caractère (passée en paramètre de la commande) */
char* strnodata;

/* Chemins des images en entrée et en sortie */
/** \~french Chemin de l'image de fond */
char* backgroundImage;
/** \~french Chemin du masque associé à l'image de fond */
char* backgroundMask;
/** \~french Chemins des images en entrée */
char* inputImages[4];
/** \~french Chemins des masques associés aux images en entrée */
char* inputMasks[4];
/** \~french Chemin de l'image en sortie */
char* outputImage;
/** \~french Chemin du masque associé à l'image en sortie */
char* outputMask;

/* Caractéristiques des images en entrée et en sortie */
/** \~french Valeur de gamma, pour foncer ou éclaircir des images en entier */
double gammaM4t;
/** \~french Largeur des images */
uint32_t width;
/** \~french Hauteur des images */
uint32_t height;
/** \~french Compression de l'image de sortie */
Compression::eCompression compression = Compression::NONE;

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

/** \~french Activation du niveau de log debug. Faux par défaut */
bool debugLogger=false;

/** \~french Message d'usage de la commande merge4tiff */
std::string help = std::string("\ncache2work version ") + std::string(ROK4_VERSION) + "\n\n"

    "Four images subsampling, formed a square, might use a background and data masks\n\n"

    "Usage: merge4tiff [-g <VAL>] -n <VAL> [-c <VAL>] [-iX <FILE> [-mX<FILE>]] -io <FILE> [-mo <FILE>]\n\n"

    "Parameters:\n"
    "     -g gamma float value, to dark (0 < g < 1) or brighten (1 < g) 8-bit integer images' subsampling\n"
    "     -n nodata value, one interger per sample, seperated with comma. Examples\n"
    "             -99999 for DTM\n"
    "             255,255,255 for orthophotography\n"
    "     -c output compression :\n"
    "             raw     no compression\n"
    "             none    no compression\n"
    "             jpg     Jpeg encoding\n"
    "             lzw     Lempel-Ziv & Welch encoding\n"
    "             pkb     PackBits encoding\n"
    "             zip     Deflate encoding\n\n"

    "     -io output image\n"
    "     -mo output mask (optionnal)\n\n"

    "     -iX input images\n"
    "             X = [1..4]      give input image position\n"
    "                     image1 | image2\n"
    "                     -------+-------\n"
    "                     image3 | image4\n\n"

    "             X = b           background image\n"
    "     -mX input associated masks (optionnal)\n"
    "             X = [1..4] or X = b\n"
    "     -a sample format : (float or uint)\n"
    "     -b bits per sample : (8 or 32)\n"
    "     -s samples per pixel : (1, 2, 3 or 4)\n"
    "     -d debug logger activation\n\n"

    "If bitspersample, sampleformat or samplesperpixel are not provided, those 3 informations are read from the image sources (all have to own the same). If 3 are provided, conversion may be done.\n\n"

    "Examples\n"
    "     - without mask, with background image\n"
    "     merge4tiff -g 1 -n 255,255,255 -c zip -ib backgroundImage.tif -i1 image1.tif -i3 image3.tif -io imageOut.tif\n\n"

    "     - with mask, without background image\n"
    "     merge4tiff -g 1 -n 255,255,255 -c zip -i1 image1.tif -m1 mask1.tif -i3 image3.tif -m3 mask3.tif -mo maskOut.tif  -io imageOut.tif\n";

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande merge4tiff # help
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
int parseCommandLine ( int argc, char* argv[] ) {
    // Initialisation
    gammaM4t = 1.;
    strnodata = 0;
    compression = Compression::NONE;
    backgroundImage = 0;
    backgroundMask = 0;
    for ( int i=0; i<4; i++ ) {
        inputImages[i] = 0;
        inputMasks[i] = 0;
    }
    outputImage = 0;
    outputMask = 0;

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'h': // help
                usage();
                exit ( 0 );
            case 'd': // debug logs
                debugLogger = true;
                break;
            case 'g': // gamma
                if ( ++i == argc ) {
                    LOGGER_ERROR ( "Error in option -g" );
                    return -1;
                }
                gammaM4t = atof ( argv[i] );
                if ( gammaM4t <= 0. ) {
                    LOGGER_ERROR ( "Unvalid parameter in -g argument, have to be positive" );
                    return -1;
                }
                break;
            case 'n': // nodata
                if ( ++i == argc ) {
                    LOGGER_ERROR ( "Error in option -n" );
                    return -1;
                }
                strnodata = argv[i];
                break;
            case 'c': // compression
                if ( ++i == argc ) {
                    LOGGER_ERROR ( "Error in option -c" );
                    return -1;
                }
                if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = Compression::NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = Compression::DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = Compression::PACKBITS;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = Compression::JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = Compression::LZW;
                else {
                    LOGGER_ERROR ( "Unknown value for option -c : " << argv[i] );
                    return -1;
                }
                break;

            case 'i': // images
                if ( ++i == argc ) {
                    LOGGER_ERROR ( "Error in option -i" );
                    return -1;
                }
                switch ( argv[i-1][2] ) {
                case '1':
                    inputImages[0] = argv[i];
                    break;
                case '2':
                    inputImages[1] = argv[i];
                    break;
                case '3':
                    inputImages[2] = argv[i];
                    break;
                case '4':
                    inputImages[3] = argv[i];
                    break;
                case 'b':
                    backgroundImage = argv[i];
                    break;
                case 'o':
                    outputImage = argv[i];
                    break;
                default:
                    LOGGER_ERROR ( "Unknown image's indice : -m" << argv[i-1][2] );
                    return -1;
                }
                break;
            case 'm': // associated masks
                if ( ++i == argc ) {
                    LOGGER_ERROR ( "Error in option -m" );
                    return -1;
                }
                switch ( argv[i-1][2] ) {
                case '1':
                    inputMasks[0] = argv[i];
                    break;
                case '2':
                    inputMasks[1] = argv[i];
                    break;
                case '3':
                    inputMasks[2] = argv[i];
                    break;
                case '4':
                    inputMasks[3] = argv[i];
                    break;
                case 'b':
                    backgroundMask = argv[i];
                    break;
                case 'o':
                    outputMask = argv[i];
                    break;
                default:
                    LOGGER_ERROR ( "Unknown mask's indice : -m" << argv[i-1][2] );
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

    /* Obligatoire :
     *  - la valeur de nodata
     *  - l'image de sortie
     */
    if ( strnodata == 0 ) {
        LOGGER_ERROR ( "Missing nodata value" );
        return -1;
    }
    if ( outputImage == 0 ) {
        LOGGER_ERROR ( "Missing output file" );
        return -1;
    }

    return 0;
}

/**
 * \~french
 * \brief Contrôle les caractéristiques d'une image (format des canaux, tailles) et de son éventuel masque.
 * \details Si les composantes sont bonnes, le masque est attaché à l'image.
 * \param[in] image image à contrôler
 * \param[in] mask précise éventuellement un masque de donnée
 * \return code de retour, 0 si réussi, -1 sinon
 */
int checkComponents ( FileImage* image, FileImage* mask) {

    if ( width == 0 ) { // read the parameters of the first input file
        width = image->getWidth();
        height = image->getHeight();
        
        if ( width%2 || height%2 ) {
            LOGGER_ERROR ( "Sorry : only even dimensions for input images are supported" );
            return -1;
        }

        if (! outputProvided) {
            // On n'a pas précisé de format de sortie
            // Toutes les images en entrée doivent avoir le même format
            // La sortie aura ce format
            bitspersample = image->getBitsPerSample();
            photometric = image->getPhotometric();
            sampleformat = image->getSampleFormat();
            samplesperpixel = image->getChannels();
        } else {
            // La photométrie est déduite du nombre de canaux
            if (samplesperpixel == 1) {
                photometric = Photometric::GRAY;
            } else if (samplesperpixel == 2) {
                photometric = Photometric::GRAY;
            } else {
                photometric = Photometric::RGB;
            }
        }

        if ( ! (( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) || ( bitspersample == 8 && sampleformat == SampleFormat::UINT )) ) {
            LOGGER_ERROR ( "Unknown sample type (sample format + bits per sample)" );
            return -1;
        }
    } else {

        if ( image->getWidth() != width || image->getHeight() != height) {
            LOGGER_ERROR ( "Error : all input image must have the same dimensions (width, height) : " << image->getFilename());
            return -1;
        }

        if (! outputProvided) {
            if ( image->getBitsPerSample() != bitspersample || image->getSampleFormat() != sampleformat ||
                 image->getPhotometric() != photometric || image->getChannels() != samplesperpixel
            ) {
                LOGGER_ERROR (
                    "Error : output format is not provided, so all input image must have the same format (bits per sample, channels, etc...) : " << 
                    image->getFilename()
                );
                return -1;
            }
        }
    }

    if (mask != NULL) {
        if ( ! ( mask->getWidth() == width && mask->getHeight() == height && mask->getBitsPerSample() == 8 &&
                mask->getSampleFormat() == SampleFormat::UINT && mask->getPhotometric() == Photometric::GRAY && mask->getChannels() == 1 ) ) {

            LOGGER_ERROR ( "Error : all input masks must have the same parameters (width, height, etc...) : " << mask->getFilename());
            return -1;
        }

        if ( ! image->setMask(mask) ) {
            LOGGER_ERROR ( "Cannot add associated mask to the input FileImage " << image->getFilename() );
            return -1;
        }
        
    }

    if (outputProvided) {
        bool ok = image->addConverter ( sampleformat, bitspersample, samplesperpixel );
        if (! ok ) {
            LOGGER_ERROR ( "Cannot add converter to the input FileImage " << image->getFilename() );
            return -1;
        }
    }

    return 0;
}

/**
 * \~french
 * \brief Contrôle l'ensemble des images et masques, en entrée et sortie
 * \details Crée les objets TIFF, contrôle la cohérence des caractéristiques des images en entrée, ouvre les flux de lecture et écriture. Les éventuels masques associés sont ajoutés aux objets FileImage.
 * \param[in] INPUTI images en entrée
 * \param[in] BGI image de fond en entrée
 * \param[in] OUTPUTI image en sortie
 * \return code de retour, 0 si réussi, -1 sinon
 */
int checkImages ( FileImage* INPUTI[2][2], FileImage*& BGI, FileImage*& OUTPUTI, FileImage*& OUTPUTM) {
    width = 0;
    FileImageFactory FIF;

    for ( int i = 0; i < 4; i++ ) {
        LOGGER_DEBUG ( "Place " << i );
        // Initialisation
        if ( inputImages[i] == 0 ) {
            LOGGER_DEBUG ( "No image" );
            INPUTI[i/2][i%2] = NULL;
            continue;
        }

        // Image en entrée
        FileImage* inputi = FIF.createImageToRead(inputImages[i]);
        if ( inputi == NULL ) {
            LOGGER_ERROR ( "Unable to open input image: " + std::string ( inputImages[i] ) );
            return -1;
        }

        // Eventuelle masque associé
        FileImage* inputm = NULL;
        if ( inputMasks[i] != 0 ) {
            inputm = FIF.createImageToRead(inputMasks[i]);
            if ( inputm == NULL ) {
                LOGGER_ERROR ( "Unable to open input mask: " << std::string ( inputMasks[i] ) );
                return -1;
            }
        }

        // Controle des composantes des images/masques et association
        LOGGER_DEBUG ( "Check" );
        if ( checkComponents ( inputi, inputm ) < 0 ) {
            LOGGER_ERROR ( "Unvalid components for the image " << std::string ( inputImages[i] ) << " (or its mask)" );
            return -1;
        }
        
        INPUTI[i/2][i%2] = inputi;
    }

    BGI = NULL;

    // Si on a quatre image et pas de masque (images considérées comme pleines), le fond est inutile
    if ( inputImages[0] && inputImages[1] && inputImages[2] && inputImages[3] &&
            ! inputMasks[0] && ! inputMasks[1] && ! inputMasks[2] && ! inputMasks[3] )
        
        backgroundImage=0;

    if ( backgroundImage ) {
        BGI = FIF.createImageToRead(backgroundImage);
        if ( BGI == NULL ) {
            LOGGER_ERROR ( "Unable to open background image: " + std::string ( backgroundImage ) );
            return -1;
        }

        FileImage* BGM = NULL;

        if ( backgroundMask ) {
            BGM = FIF.createImageToRead(backgroundMask);
            if ( BGM == NULL ) {
                LOGGER_ERROR ( "Unable to open background mask: " + std::string ( backgroundMask ) );
                return -1;
            }
        }

        // Controle des composantes des images/masques
        if ( checkComponents ( BGI, BGM ) < 0 ) {
            LOGGER_ERROR ( "Unvalid components for the background image " << std::string ( backgroundImage ) << " (or its mask)" );
            return -1;
        }
    }

    /********************** EN SORTIE ***********************/

    OUTPUTI = NULL;
    OUTPUTM = NULL;

    OUTPUTI = FIF.createImageToWrite(outputImage, BoundingBox<double>(0,0,0,0), -1, -1, width, height,
                                     samplesperpixel, sampleformat, bitspersample, photometric, compression);
    if ( OUTPUTI == NULL ) {
        LOGGER_ERROR ( "Unable to open output image: " + std::string ( outputImage ) );
        return -1;
    }

    if ( outputMask ) {
        OUTPUTM = FIF.createImageToWrite(outputMask, BoundingBox<double>(0,0,0,0), -1, -1, width, height,
                                                   1, SampleFormat::UINT, 8, Photometric::MASK, Compression::DEFLATE);
        if ( OUTPUTM == NULL ) {
            LOGGER_ERROR ( "Unable to open output mask: " + std::string ( outputMask ) );
            return -1;
        }

        OUTPUTI->setMask(OUTPUTM);
    }

    return 0;
}


/**
 * \~french
 * \brief Remplit un buffer à partir d'une ligne d'une image et d'un potentiel masque associé
 * \details les pixels qui ne contiennent pas de donnée sont remplis avec la valeur de nodata
 * \param[in] BGI image de fond à lire
 * \param[out] image ligne de l'image en sortie
 * \param[out] mask ligne du masque en sortie
 * \param[in] line indice de la ligne source dans l'image (et son masque)
 * \param[in] nodata valeur de nodata
 * \return code de retour, 0 si réussi, -1 sinon
 */
template <typename T>
int fillBgLine ( FileImage* BGI, T* image, uint8_t* mask, int line, T* nodata ) {
    if ( BGI->getline( image, line ) == 0 ) return 1;

    if ( BGI->getMask() != NULL ) {
        if ( BGI->getMask()->getline( mask, line ) == 0 ) return 1;
        for ( int w = 0; w < width; w++ ) {
            if ( mask[w] == 0 ) {
                memcpy ( image + w*samplesperpixel, nodata,samplesperpixel*sizeof ( T ) );
            }
        }
    } else {
        memset ( mask,255,width );
    }

    return 0;
}

/**
 * \~french
 * \brief Fusionne les 4 images en entrée et le masque de fond dans l'image de sortie
 * \details Dans le cas entier, lors de la moyenne des 4 pixels, on utilise une valeur de gamma qui éclaircit (si supérieure à 1.0) ou fonce (si inférieure à 1.0) le résultat. Si gamma vaut 1, le résultat est une moyenne classique. Les masques sont déjà associé aux objets FileImage, sauf pour l'image de sortie.
 * \param[in] BGI image de fond en entrée
 * \param[in] INPUTI images en entrée
 * \param[in] OUTPUTI image en sortie
 * \param[in] OUTPUTI éventuel masque en sortie
 * \return code de retour, 0 si réussi, -1 sinon
 */
template <typename T>
int merge ( FileImage* BGI, FileImage* INPUTI[2][2], FileImage* OUTPUTI, FileImage* OUTPUTM, T* nodata ) {
    
    uint8 MERGE[1024];
    for ( int i = 0; i <= 1020; i++ ) MERGE[i] = 255 - ( uint8 ) round ( pow ( double ( 1020 - i ) /1020., gammaM4t ) * 255. );

    int nbsamples = width * samplesperpixel;
    int left,right;

    T line_bgI[nbsamples];
    uint8_t line_bgM[width];

    int nbData;
    float pix[samplesperpixel];

    T line_1I[2*nbsamples];
    uint8_t line_1M[2*width];

    T line_2I[2*nbsamples];
    uint8_t line_2M[2*width];

    T line_outI[nbsamples];
    uint8_t line_outM[width];

    // ----------- initialisation du fond -----------
    for ( int i = 0; i < nbsamples ; i++ )
        line_bgI[i] = nodata[i%samplesperpixel];

    memset ( line_bgM,0,width );

    for ( int y = 0; y < 2; y++ ) {
        if ( INPUTI[y][0] ) left = 0;
        else left = width;
        if ( INPUTI[y][1] ) right = 2*width;
        else right = width;

        for ( uint32 h = 0; h < height/2; h++ ) {

            int line = y*height/2 + h;

            // ------------------- le fond ------------------
            if ( BGI )
                if ( fillBgLine ( BGI, line_bgI, line_bgM, line, nodata ) ) {
                    LOGGER_ERROR ( "Unable to read background line" );
                    return -1;
                }

            if ( left == right ) {
                // On n'a pas d'image en entrée pour cette ligne, on stocke le fond et on passe à la suivante
                if ( OUTPUTI->writeLine( line_bgI, line ) == -1 ) {
                    LOGGER_ERROR ( "Unable to write image's line " << line );
                    return -1;
                }
                if ( OUTPUTM )
                    if ( OUTPUTM->writeLine( line_bgM, line ) == -1 ) {
                        LOGGER_ERROR ( "Unable to write mask's line " << line );
                        return -1;
                    }

                continue;
            }

            // -- initialisation de la sortie avec le fond --
            memcpy ( line_outI,line_bgI,nbsamples*sizeof ( T ) );
            memcpy ( line_outM,line_bgM,width );
            
            memset ( line_1M,255,2*width );
            memset ( line_2M,255,2*width );

            // ----------------- les images -----------------
            // ------ et les éventuels masques --------------
            if ( INPUTI[y][0] ) {
                if ( INPUTI[y][0]->getline( line_1I, 2*h ) == 0 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
                if ( INPUTI[y][0]->getline( line_2I, 2*h+1 ) == 0 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }

                if ( INPUTI[y][0]->getMask() ) {
                    if ( INPUTI[y][0]->getMask()->getline( line_1M, 2*h ) == 0 ) {
                        LOGGER_ERROR ( "Unable to read data line" );
                        return -1;
                    }
                    if ( INPUTI[y][0]->getMask()->getline( line_2M, 2*h+1 ) == 0 ) {
                        LOGGER_ERROR ( "Unable to read data line" );
                        return -1;
                    }
                }
            }


            if ( INPUTI[y][1] ) {
                if ( INPUTI[y][1]->getline( line_1I + nbsamples, 2*h ) == 0 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
                if ( INPUTI[y][1]->getline( line_2I + nbsamples, 2*h+1 ) == 0 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }

                if ( INPUTI[y][1]->getMask() ) {
                    if ( INPUTI[y][1]->getMask()->getline( line_1M + width, 2*h ) == 0 ) {
                        LOGGER_ERROR ( "Unable to read data line" );
                        return -1;
                    }
                    if ( INPUTI[y][1]->getMask()->getline( line_2M + width, 2*h+1 ) == 0 ) {
                        LOGGER_ERROR ( "Unable to read data line" );
                        return -1;
                    }
                }
            }

            // ----------------- la moyenne ----------------
            for ( int pixIn = left, sampleIn = left * samplesperpixel; pixIn < right;
                    pixIn += 2, sampleIn += 2*samplesperpixel ) {

                memset ( pix,0,samplesperpixel*sizeof ( float ) );
                nbData = 0;

                if ( line_1M[pixIn] ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_1I[sampleIn+c];
                }

                if ( line_1M[pixIn+1] ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_1I[sampleIn+samplesperpixel+c];
                }

                if ( line_2M[pixIn] ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_2I[sampleIn+c];
                }

                if ( line_2M[pixIn+1] ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_2I[sampleIn+samplesperpixel+c];
                }

                if ( nbData > 1 ) {
                    line_outM[pixIn/2] = 255;
                    if ( sizeof ( T ) == 1 ) {
                        // Cas entier : utilisation d'un gamma
                        for ( int c = 0; c < samplesperpixel; c++ ) line_outI[sampleIn/2+c] = MERGE[ ( int ) pix[c]*4/nbData];
                    } else if ( sizeof ( T ) == 4 ) {
                        for ( int c = 0; c < samplesperpixel; c++ ) line_outI[sampleIn/2+c] = pix[c]/ ( float ) nbData;
                    }
                }
            }

            if ( OUTPUTI->writeLine( line_outI, line ) == -1 ) {
                LOGGER_ERROR ( "Unable to write image" );
                return -1;
            }
            if ( OUTPUTM )
                if ( OUTPUTM->writeLine( line_outM, line ) == -1 ) {
                    LOGGER_ERROR ( "Unable to write mask" );
                    return -1;
                }
        }
    }

    return 0;
}

/**
 ** \~french
 * \brief Fonction principale de l'outil merge4tiff
 * \details Différencie le cas de canaux flottants sur 32 bits des canaux entier non signés sur 8 bits.
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 * \return 0 en cas de succès, -1 sinon
 ** \~english
 * \brief Main function for tool merge4tiff
 * \param[in] argc parameters number
 * \param[in] argv parameters array
 * \return 0 if success, -1 otherwise
 */
int main ( int argc, char* argv[] ) {
    FileImage* INPUTI[2][2];
    FileImage* BGI;
    FileImage* OUTPUTI;
    FileImage* OUTPUTM;

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

    LOGGER_DEBUG ( "Parse" );
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

    LOGGER_DEBUG ( "Check images" );
    // Controle des images
    if ( checkImages ( INPUTI, BGI, OUTPUTI, OUTPUTM ) < 0 ) {
        error ( "Echec controle des images",-1 );
    }

    LOGGER_DEBUG ( "Nodata interpretation" );
    // Conversion string->int[] du paramètre nodata
    int nodataInt[samplesperpixel];

    char* charValue = strtok ( strnodata,"," );
    if ( charValue == NULL ) {
        error ( "Error with option -n : a value for nodata is missing",-1 );
    }
    nodataInt[0] = atoi ( charValue );
    for ( int i = 1; i < samplesperpixel; i++ ) {
        charValue = strtok ( NULL, "," );
        if ( charValue == NULL ) {
            error ( "Error with option -n : a value for nodata is missing",-1 );
        }
        nodataInt[i] = atoi ( charValue );
    }

    // Cas MNT
    if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        LOGGER_DEBUG ( "Merge images (float)" );
        float nodata[samplesperpixel];
        for ( int i = 0; i < samplesperpixel; i++ ) nodata[i] = ( float ) nodataInt[i];
        if ( merge<float> ( BGI, INPUTI, OUTPUTI, OUTPUTM, nodata ) < 0 ) error ( "Unable to merge float images",-1 );
    }
    // Cas images
    else if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        LOGGER_DEBUG ( "Merge images (uint8_t)" );
        uint8_t nodata[samplesperpixel];
        for ( int i = 0; i < samplesperpixel; i++ ) nodata[i] = ( uint8_t ) nodataInt[i];
        if ( merge ( BGI, INPUTI, OUTPUTI, OUTPUTM, nodata ) < 0 ) error ( "Unable to merge integer images",-1 );
    } else {
        error ( "Unhandled sample's format",-1 );
    }


    LOGGER_DEBUG ( "Clean" );
    
    if ( BGI ) delete BGI;

    for ( int i = 0; i < 2; i++ ) for ( int j = 0; j < 2; j++ ) {
        if ( INPUTI[i][j] ) delete INPUTI[i][j] ;
    }

    delete OUTPUTI;

    Logger::stopLogger();
    acc->stop();
    acc->destroy();
    delete acc;
}

