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
 * \~french \details les images doivent avoir la disposition suivante , mais les 4 ne sont pas forcément présentes :
 * \code
 *    image1 | image2
 *    -------+-------
 *    image3 | image4
 * \endcode
 * Les caractéristiques suivantes doivent être les mêmes pour les 4 images, le fond et seront celles de l'image de sortie :
 * \li largeur et hauteur en pixel
 * \li nombre de canaux
 * \li format des canaux
 * \li photomètrie
 *
 * Les masques doivent avoir la même taille mais sont à un canal entier non signé sur 8 bits
 * Exemple d'appel à la commande :
 * \~french \li sans masque, avec une image de fond \~english \li without mask, with background image
 * \~ \code
 * merge4tiff -g 1 -n 255,255,255 -c zip -b backgroundImage.tif -i1 image1.tif -i3 image3.tif imageOut.tif
 * \endcode
 * \~french \li avec masque, sans image de fond \~english \li with mask, without background image
 * \~ \code
 * merge4tiff -g 1 -n 255,255,255 -c zip -i1 image1.tif -m1 mask1.tif -i3 image3.tif -m3 mask3.tif -mo maskOut.tif -io imageOut.tif
 * \endcode
 */

#include "tiffio.h"
#include "Image.h"
#include "Format.h"
#include "Logger.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdint.h>
#include "../be4version.h"

/* Valeurs de nodata */
/** \~french Valeur de nodata sour forme de chaîne de caractère (passée en paramètre de la commande) */
char* strnodata;
/** \~french Valeur de nodata sous forme de tableau de flottants sur 32 bits*/
float* nodataFloat32;
/** \~french Valeur de nodata sous forme de tableau d'entier non-signés sur 8 bits*/
uint8_t* nodataUInt8;

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
/** \~french Bufferisation des images */
uint32_t rowsperstrip = 1;
/** \~french Compression de l'image de sortie */
uint16_t compression;
/** \~french Type du canal (entier, flottant, signé ou non...), dans les images en entrée et celle en sortie */
SampleType sampleType ( 0,0 );
/** \~french Nombre de canaux par pixel, dans les images en entrée et celle en sortie */
uint16_t samplesperpixel;
/** \~french Photométrie (rgb, gray), dans les images en entrée et celle en sortie */
uint16_t photometric;
/** \~french Agancement des canaux (ne gère que des canaux entremêlés : RGB RGB RGB...) */
uint16_t planarconfig;

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande merge4tiff
 * \details L'affichage se fait dans le niveau de logger INFO
 * \~ \code
 * merge4tiff version X.X.X
 *
 * Usage: merge4tiff [-g <VAL>] -n <VAL> -c <VAL> [-iX <FILE> [-mX<FILE>]] -io <FILE> [-mo <FILE>]
 *
 * Parameters:
 *      -g gamma float value, to dark (0 < g < 1) or brighten (1 < g) 8-bit integer images' subsampling
 *      -n nodata value, one interger per sample, seperated with comma. Examples
 *              -99999 for DTM
 *              255,255,255 for orthophotography
 *      -c output compression :
 *              raw     no compression
 *              none    no compression
 *              jpg     Jpeg encoding
 *              lzw     Lempel-Ziv & Welch encoding
 *              pkb     PackBits encoding
 *              zip     Deflate encoding
 *
 *      -io output image
 *      -mo output mask (optionnal)
 *
 *      -iX input images
 *              X = [1..4]      give input image position
 *                      image1 | image2
 *                      -------+-------
 *                      image3 | image4
 *
 *              X = b           background image
 *      -mX input associated masks (optionnal)
 *              X = [1..4] or X = b
 *
 * Examples
 *      - without mask, with background image
 *      merge4tiff -g 1 -n 255,255,255 -c zip -b backgroundImage.tif -i1 image1.tif -i3 image3.tif imageOut.tif
 *
 *      - with mask, without background image
 *      merge4tiff -g 1 -n 255,255,255 -c zip -i1 image1.tif -m1 mask1.tif -i3 image3.tif -m3 mask3.tif -mo maskOut.tif  -io imageOut.tif
 * \endcode
 */
void usage() {
    LOGGER_INFO ( "\nmerge4tiff version " << BE4_VERSION << "\n\n" <<

                  "Four images subsampling, formed a square, might use a background and data masks\n\n" <<

                  "Usage: merge4tiff [-g <VAL>] -n <VAL> -c <VAL> [-iX <FILE> [-mX<FILE>]] -io <FILE> [-mo <FILE>]\n\n" <<

                  "Parameters:\n" <<
                  "     -g gamma float value, to dark (0 < g < 1) or brighten (1 < g) 8-bit integer images' subsampling\n" <<
                  "     -n nodata value, one interger per sample, seperated with comma. Examples\n" <<
                  "             -99999 for DTM\n" <<
                  "             255,255,255 for orthophotography\n" <<
                  "     -c output compression :\n" <<
                  "             raw     no compression\n" <<
                  "             none    no compression\n" <<
                  "             jpg     Jpeg encoding\n" <<
                  "             lzw     Lempel-Ziv & Welch encoding\n" <<
                  "             pkb     PackBits encoding\n" <<
                  "             zip     Deflate encoding\n\n" <<

                  "     -io output image\n" <<
                  "     -mo output mask (optionnal)\n\n" <<

                  "     -iX input images\n" <<
                  "             X = [1..4]      give input image position\n" <<
                  "                     image1 | image2\n" <<
                  "                     -------+-------\n" <<
                  "                     image3 | image4\n\n" <<

                  "             X = b           background image\n" <<
                  "     -mX input associated masks (optionnal)\n" <<
                  "             X = [1..4] or X = b\n\n" <<

                  "Examples\n" <<
                  "     - without mask, with background image\n" <<
                  "     merge4tiff -g 1 -n 255,255,255 -c zip -b backgroundImage.tif -i1 image1.tif -i3 image3.tif imageOut.tif\n\n" <<

                  "     - with mask, without background image\n" <<
                  "     merge4tiff -g 1 -n 255,255,255 -c zip -i1 image1.tif -m1 mask1.tif -i3 image3.tif -m3 mask3.tif -mo maskOut.tif  -io imageOut.tif\n" );
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
    compression = -1;
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
                if ( strncmp ( argv[i], "none",4 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "raw",3 ) == 0 ) compression = COMPRESSION_NONE;
                else if ( strncmp ( argv[i], "zip",3 ) == 0 ) compression = COMPRESSION_ADOBE_DEFLATE;
                else if ( strncmp ( argv[i], "pkb",3 ) == 0 ) compression = COMPRESSION_PACKBITS;
                else if ( strncmp ( argv[i], "jpg",3 ) == 0 ) compression = COMPRESSION_JPEG;
                else if ( strncmp ( argv[i], "lzw",3 ) == 0 ) compression = COMPRESSION_LZW;
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
            default:
                LOGGER_ERROR ( "Unknown option : -" << argv[i][1] );
                return -1;
            }
        }
    }

    /* Obligatoire :
     *  - la valeur de nodata
     *  - l'image de sortie
     *  - la compression
     */
    if ( strnodata == 0 ) {
        LOGGER_ERROR ( "Missing nodata value" );
        return -1;
    }
    if ( outputImage == 0 ) {
        LOGGER_ERROR ( "Missing output file" );
        return -1;
    }
    if ( compression == -1 ) {
        LOGGER_ERROR ( "Missing compression" );
        return -1;
    }

    return 0;
}

/**
 * \~french
 * \brief Contrôle les caractéristiques d'une image (format des canaux, tailles)
 * \param[in] image image à contrôler
 * \param[in] isMask précise si l'image contrôlée est un masque
 * \return code de retour, 0 si réussi, -1 sinon
 */
int checkComponents ( TIFF* image, bool isMask ) {
    uint32_t _width,_height;
    uint16_t _bitspersample,_samplesperpixel,_sampleformat,_photometric,_planarconfig;

    if ( width == 0 ) { // read the parameters of the first input file
        if ( ! TIFFGetField ( image, TIFFTAG_IMAGEWIDTH, &width )                   ||
                ! TIFFGetField ( image, TIFFTAG_IMAGELENGTH, &height )                     ||
                ! TIFFGetField ( image, TIFFTAG_BITSPERSAMPLE, &_bitspersample )            ||
                ! TIFFGetFieldDefaulted ( image, TIFFTAG_PLANARCONFIG, &planarconfig )     ||
                ! TIFFGetField ( image, TIFFTAG_PHOTOMETRIC, &photometric )                ||
                ! TIFFGetFieldDefaulted ( image, TIFFTAG_SAMPLEFORMAT, &_sampleformat )     ||
                ! TIFFGetFieldDefaulted ( image, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel ) ) {
            LOGGER_ERROR ( std::string ( "Error reading input file: " ) + TIFFFileName ( image ) );
            return -1;
        }

        if ( planarconfig != 1 ) {
            LOGGER_ERROR ( "Sorry : only planarconfig = 1 is supported" );
            return -1;
        }
        if ( width%2 || height%2 ) {
            LOGGER_ERROR ( "Sorry : only even dimensions for input images are supported" );
            return -1;
        }

        sampleType = SampleType ( _bitspersample, _sampleformat );

        if ( ! sampleType.isSupported() ) {
            error ( "Supported sample format are :\n" + sampleType.getHandledFormat(),-1 );
        }

        return 0;
    }

    if ( ! TIFFGetField ( image, TIFFTAG_IMAGEWIDTH, &_width )                         ||
            ! TIFFGetField ( image, TIFFTAG_IMAGELENGTH, &_height )                       ||
            ! TIFFGetField ( image, TIFFTAG_BITSPERSAMPLE, &_bitspersample )              ||
            ! TIFFGetFieldDefaulted ( image, TIFFTAG_PLANARCONFIG, &_planarconfig )       ||
            ! TIFFGetFieldDefaulted ( image, TIFFTAG_SAMPLESPERPIXEL, &_samplesperpixel ) ||
            ! TIFFGetField ( image, TIFFTAG_PHOTOMETRIC, &_photometric )                  ||
            ! TIFFGetFieldDefaulted ( image, TIFFTAG_SAMPLEFORMAT, &_sampleformat ) ) {
        LOGGER_ERROR ( std::string ( "Error reading file " ) + TIFFFileName ( image ) );
        return -1;
    }

    if ( isMask ) {
        if ( ! ( _width == width && _height == height && _bitspersample == 8 && _planarconfig == planarconfig &&
                 _photometric == PHOTOMETRIC_MINISBLACK && _samplesperpixel == 1 ) ) {
            LOGGER_ERROR ( std::string ( "Error : all input masks must have the same parameters (width, height, etc...) : " )
                           + TIFFFileName ( image ) );
            return -1;
        }
    } else {
        if ( ! ( _width == width && _height == height && _bitspersample == sampleType.getBitsPerSample() &&
                 _planarconfig == planarconfig && _photometric == photometric && _samplesperpixel == samplesperpixel ) ) {

            LOGGER_ERROR ( std::string ( "Error : all input images must have the same parameters (width, height, etc...) : " )
                           + TIFFFileName ( image ) );
            return -1;
        }
    }

    return 0;
}

/**
 * \~french
 * \brief Contrôle l'ensemble des images et masques, en entrée et sortie
 * \details Crée les objets TIFF, contrôle la cohérence des caractéristiques des images en entrée, ouvre les flux de lecture et écriture
 * \param[in] INPUTI images en entrée
 * \param[in] INPUTM masques associé aux images en entrée
 * \param[in] BGI image de fond en entrée
 * \param[in] BGM masque associé à l'image de fond en entrée
 * \param[in] OUTPUTI image en sortie
 * \param[in] OUTPUTM masques associé à l'image en sortie
 * \return code de retour, 0 si réussi, -1 sinon
 */
int checkImages ( TIFF* INPUTI[2][2],TIFF* INPUTM[2][2],
                  TIFF*& BGI,TIFF*& BGM,
                  TIFF*& OUTPUTI,TIFF*& OUTPUTM ) {
    width=0;

    for ( int i = 0; i < 4; i++ ) {
        INPUTM[i/2][i%2] = NULL;
        if ( inputImages[i] == 0 ) {
            INPUTI[i/2][i%2] = NULL;
            continue;
        }

        TIFF *inputi = TIFFOpen ( inputImages[i], "r" );
        if ( inputi == NULL ) {
            LOGGER_ERROR ( "Unable to open input image: " + std::string ( inputImages[i] ) );
            return -1;
        }
        INPUTI[i/2][i%2] = inputi;

        if ( checkComponents ( inputi, false ) < 0 ) {
            LOGGER_ERROR ( "Unable to read components of the image " << std::string ( inputImages[i] ) );
            return -1;
        }

        if ( inputMasks[i] != 0 ) {
            TIFF *inputm = TIFFOpen ( inputMasks[i], "r" );
            if ( inputm == NULL ) {
                LOGGER_ERROR ( "Unable to open input mask: " << std::string ( inputMasks[i] ) );
                return -1;
            }
            INPUTM[i/2][i%2] = inputm;

            if ( checkComponents ( inputm, true ) < 0 ) {
                LOGGER_ERROR ( "Unable to read components of the mask " << std::string ( inputMasks[i] ) );
                return -1;
            }
        }

    }

    BGI = 0;
    BGM = 0;

    // Si on a quatre image et pas de masque (images considérées comme pleines), le fond est inutile
    if ( inputImages[0] && inputImages[1] && inputImages[2] && inputImages[3] &&
            ! inputMasks[0] && ! inputMasks[1] && ! inputMasks[2] && ! inputMasks[3] )
        backgroundImage=0;

    if ( backgroundImage ) {
        BGI=TIFFOpen ( backgroundImage, "r" );
        if ( BGI == NULL ) {
            LOGGER_ERROR ( "Unable to open background image: " + std::string ( backgroundImage ) );
            return -1;
        }

        if ( checkComponents ( BGI, false ) < 0 ) {
            LOGGER_ERROR ( "Unable to read components of the background image " << std::string ( backgroundImage ) );
            return -1;
        }

        if ( backgroundMask ) {
            BGM = TIFFOpen ( backgroundMask, "r" );
            if ( BGM == NULL ) {
                LOGGER_ERROR ( "Unable to open background mask: " + std::string ( backgroundMask ) );
                return -1;
            }

            if ( checkComponents ( BGM, true ) < 0 ) {
                LOGGER_ERROR ( "Unable to read components of the background mask " << std::string ( backgroundMask ) );
                return -1;
            }
        }
    }

    OUTPUTI = 0;
    OUTPUTM = 0;

    OUTPUTI = TIFFOpen ( outputImage, "w" );
    if ( OUTPUTI == NULL ) {
        LOGGER_ERROR ( "Unable to open output image: " + std::string ( outputImage ) );
        return -1;
    }

    if ( ! TIFFSetField ( OUTPUTI, TIFFTAG_IMAGEWIDTH, width ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_IMAGELENGTH, height ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_BITSPERSAMPLE, sampleType.getBitsPerSample() ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_PHOTOMETRIC, photometric ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_ROWSPERSTRIP, rowsperstrip ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_PLANARCONFIG, planarconfig ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_COMPRESSION, compression ) ||
            ! TIFFSetField ( OUTPUTI, TIFFTAG_SAMPLEFORMAT, sampleType.getSampleFormat() ) ) {
        LOGGER_ERROR ( "Error writting output image: " + std::string ( outputImage ) );
        return -1;
    }

    if ( outputMask ) {
        OUTPUTM = TIFFOpen ( outputMask, "w" );
        if ( OUTPUTM == NULL ) {
            LOGGER_ERROR ( "Unable to open output mask: " + std::string ( outputImage ) );
            return -1;
        }
        if ( ! TIFFSetField ( OUTPUTM, TIFFTAG_IMAGEWIDTH, width ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_IMAGELENGTH, height ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_BITSPERSAMPLE, 8 ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_SAMPLESPERPIXEL, 1 ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_ROWSPERSTRIP, rowsperstrip ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_PLANARCONFIG, planarconfig ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE ) ||
                ! TIFFSetField ( OUTPUTM, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT ) ) {
            LOGGER_ERROR ( "Error writting output mask: " + std::string ( outputMask ) );
            return -1;
        }
    }

    return 0;
}

/**
 * \~french
 * \brief Remplit un buffer à partir d'une ligne d'une image et d'un potentiel masque associé (cas entier)
 * \details les pixels qui ne contiennent pas de donnée sont remplis avec la valeur de nodata
 * \param[in] image ligne de l'image en sortie
 * \param[in] IMAGE image à lire
 * \param[in] mask ligne du masque en sortie
 * \param[in] MASK masque associé à l'image à lire (peut être nul)
 * \param[in] line indice de la ligne source dans l'image (et son masque)
 * \param[in] width largeur de la ligne à lire (et remplir)
 * \return code de retour, 0 si réussi, -1 sinon
 */
int fillLine ( uint8_t* image, TIFF* IMAGE, uint8_t* mask, TIFF* MASK, int line, int width ) {
    if ( TIFFReadScanline ( IMAGE, image,line ) == -1 ) return 1;

    if ( MASK ) {
        if ( TIFFReadScanline ( MASK, mask,line ) == -1 ) return 1;
        for ( int w = 0; w < width; w++ ) {
            if ( mask[w] < 127 ) {
                memcpy ( image + w*samplesperpixel,nodataUInt8,samplesperpixel );
            }
        }
    } else {
        memset ( mask,255,width );
    }

    return 0;
}

/**
 * \~french
 * \brief Remplit un buffer à partir d'une ligne d'une image et d'un potentiel masque associé (cas flottant)
 * \details les pixels qui ne contiennent pas de donnée sont remplis avec la valeur de nodata
 * \param[in] image ligne de l'image en sortie
 * \param[in] IMAGE image à lire
 * \param[in] mask ligne du masque en sortie
 * \param[in] MASK masque associé à l'image à lire (peut être nul)
 * \param[in] line indice de la ligne source dans l'image (et son masque)
 * \param[in] width largeur de la ligne à lire (et remplir)
 * \return code de retour, 0 si réussi, -1 sinon
 */
int fillLine ( float* image, TIFF* IMAGE, uint8_t* mask, TIFF* MASK, int line, int width ) {
    if ( TIFFReadScanline ( IMAGE, image,line ) == -1 ) return 1;

    if ( MASK ) {
        if ( TIFFReadScanline ( MASK, mask,line ) == -1 ) return 1;
        for ( int w = 0; w < width; w++ ) {
            if ( mask[w] < 127 ) {
                memcpy ( image + w*samplesperpixel,nodataFloat32,samplesperpixel*sizeof ( float ) );
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
 * \details Dans le cas entier ,lors de la moyenne des 4 pixels, on utilise une valeur de gamma qui éclaircit (si supérieure à 1.0) ou fonce (si inférieure à 1.0) le résultat. Si gamma vaut 1, le résultat est une moyenne classique.
 * \param[in] BGI image de fond en entrée
 * \param[in] BGM masque associé à l'image de fond en entrée
 * \param[in] INPUTI images en entrée
 * \param[in] INPUTM masques associé aux images en entrée
 * \param[in] OUTPUTI image en sortie
 * \param[in] OUTPUTM masques associé à l'image en sortie
 * \return code de retour, 0 si réussi, -1 sinon
 */
template <typename T>
int merge ( TIFF* BGI, TIFF* BGM, TIFF* INPUTI[2][2], TIFF* INPUTM[2][2], TIFF* OUTPUTI, TIFF* OUTPUTM ) {
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
    if ( sizeof ( T ) == 4 )
        for ( int i = 0; i < nbsamples ; i++ )
            line_bgI[i] = nodataFloat32[i%samplesperpixel];

    if ( sizeof ( T ) == 1 )
        for ( int i = 0; i < nbsamples ; i++ )
            line_bgI[i] = nodataUInt8[i%samplesperpixel];

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
                if ( fillLine ( line_bgI,BGI,line_bgM,BGM,line,width ) ) {
                    LOGGER_ERROR ( "Unable to read background line" );
                    return -1;
                }

            if ( left == right ) {
                // On n'a pas d'image en entrée pour cette ligne, on stocke le fond et on passe à la suivante
                if ( TIFFWriteScanline ( OUTPUTI, line_bgI, line ) == -1 ) {
                    LOGGER_ERROR ( "Unable to write image" );
                    return -1;
                }
                if ( OUTPUTM ) if ( TIFFWriteScanline ( OUTPUTM, line_bgM, line ) == -1 ) {
                        LOGGER_ERROR ( "Unable to write mask" );
                        return -1;
                    }

                continue;
            }

            // -- initialisation de la sortie avec le fond --
            memcpy ( line_outI,line_bgI,nbsamples*sizeof ( T ) );
            memcpy ( line_outM,line_bgM,width );

            // ----------------- les images -----------------
            if ( INPUTI[y][0] ) {
                if ( TIFFReadScanline ( INPUTI[y][0], line_1I, 2*h ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
                if ( TIFFReadScanline ( INPUTI[y][0], line_2I, 2*h+1 ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
            }


            if ( INPUTI[y][1] ) {
                if ( TIFFReadScanline ( INPUTI[y][1], line_1I + nbsamples, 2*h ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
                if ( TIFFReadScanline ( INPUTI[y][1], line_2I + nbsamples, 2*h+1 ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
            }

            // ----------------- les masques ----------------
            memset ( line_1M,255,2*width );
            memset ( line_2M,255,2*width );

            if ( INPUTM[y][0] ) {
                if ( TIFFReadScanline ( INPUTM[y][0], line_1M, 2*h ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
                if ( TIFFReadScanline ( INPUTM[y][0], line_2M, 2*h+1 ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
            }

            if ( INPUTM[y][1] ) {
                if ( TIFFReadScanline ( INPUTM[y][1], line_1M + width, 2*h ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
                if ( TIFFReadScanline ( INPUTM[y][1], line_2M + width, 2*h+1 ) == -1 ) {
                    LOGGER_ERROR ( "Unable to read data line" );
                    return -1;
                }
            }

            // ----------------- la moyenne ----------------
            for ( int pixIn = left, sampleIn = left * samplesperpixel; pixIn < right;
                    pixIn += 2, sampleIn += 2*samplesperpixel ) {

                memset ( pix,0,samplesperpixel*sizeof ( float ) );
                nbData = 0;

                if ( line_1M[pixIn] >= 127 ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_1I[sampleIn+c];
                }

                if ( line_1M[pixIn+1] >= 127 ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_1I[sampleIn+samplesperpixel+c];
                }

                if ( line_2M[pixIn] >= 127 ) {
                    nbData++;
                    for ( int c = 0; c < samplesperpixel; c++ ) pix[c] += line_2I[sampleIn+c];
                }

                if ( line_2M[pixIn+1] >= 127 ) {
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

            if ( TIFFWriteScanline ( OUTPUTI, line_outI, line ) == -1 ) {
                LOGGER_ERROR ( "Unable to write image" );
                return -1;
            }
            if ( OUTPUTM ) if ( TIFFWriteScanline ( OUTPUTM, line_outM, line ) == -1 ) {
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
    TIFF* INPUTI[2][2];
    TIFF* INPUTM[2][2];
    TIFF* BGI;
    TIFF* BGM;
    TIFF* OUTPUTI;
    TIFF* OUTPUTM;

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

    LOGGER_DEBUG ( "Parse" );
    // Lecture des parametres de la ligne de commande
    if ( parseCommandLine ( argc, argv ) < 0 ) {
        error ( "Echec lecture ligne de commande",-1 );
    }

    LOGGER_DEBUG ( "Check images" );
    // Controle des images
    if ( checkImages ( INPUTI,INPUTM,BGI,BGM,OUTPUTI,OUTPUTM ) < 0 ) {
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
    if ( sampleType.isFloat() ) {
        LOGGER_DEBUG ( "Merge images (float)" );
        nodataFloat32 = new float[samplesperpixel];
        for ( int i = 0; i < samplesperpixel; i++ ) nodataFloat32[i] = ( float ) nodataInt[i];

        if ( merge<float> ( BGI,BGM,INPUTI,INPUTM,OUTPUTI,OUTPUTM ) < 0 ) error ( "Unable to merge float images",-1 );
        delete [] nodataFloat32;
    }
    // Cas images
    else if ( sampleType.isUInt8() ) {
        LOGGER_DEBUG ( "Merge images (uint8_t)" );
        nodataUInt8 = new uint8_t[samplesperpixel];
        for ( int i = 0; i < samplesperpixel; i++ ) nodataUInt8[i] = ( uint8_t ) nodataInt[i];

        if ( merge<uint8_t> ( BGI,BGM,INPUTI,INPUTM,OUTPUTI,OUTPUTM ) < 0 ) error ( "Unable to merge integer images",-1 );
        delete [] nodataUInt8;
    }


    LOGGER_DEBUG ( "Clean" );
    if ( BGI ) TIFFClose ( BGI );
    if ( BGM ) TIFFClose ( BGM );

    for ( int i = 0; i < 2; i++ ) for ( int j = 0; j < 2; j++ ) {
        if ( INPUTI[i][j] ) TIFFClose ( INPUTI[i][j] );
        if ( INPUTM[i][j] ) TIFFClose ( INPUTM[i][j] );
    }

    TIFFClose ( OUTPUTI );
    if ( OUTPUTM ) TIFFClose ( OUTPUTM );
    delete acc;
}

