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
 * \brief Creation d une image georeference a partir de n images source
 * \author IGN
*
* Ce programme est destine a etre utilise dans la chaine de generation de cache be4.
* Il est appele pour calculer le niveau minimum d'une pyramide, pour chaque nouvelle image.
*
* Les images source ne sont pas necessairement entierement recouvrantes.
*
* Pas de fichier TIFF tuile ou LUT en entree
*
* Parametres d'entree :
* 1. Un fichier texte contenant les images source et l image finale avec leur georeferencement (resolution, emprise)
* 2. Un mode d'interpolation
* 3. Une couleur de NoData
* 4. Un type d image (Data/Metadata)
* 5. Le nombre de canaux des images
* 6. Nombre d'octets par canal
* 7. La colorimetrie
*
* En sortie, un fichier TIFF au format dit de travail brut non compressé entrelace
* Ou, erreurs (voir dans le main)
*
* Contrainte:
* Toutes les images sont dans le meme SRS (pas de reprojection)
* FIXME : meme type de pixels (nombre de canaux, poids, couleurs) en entree et en sortie
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
#include "Logger.h"
#include "LibtiffImage.h"
#include "ResampledImage.h"
#include "ExtendedCompoundImage.h"
#include "MirrorImage.h"
#include "math.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif


/**
* @fn void usage()
* Usage de la ligne de commande
*/

void usage() {
    LOGGER_INFO ( " Usage :  mergeNtiff -f [fichier liste des images source] -a [uint/float] -i [lanczos/nn/linear/bicubic] -n [couleur NoData] -t [img/mtd] -s [1/3/4] -b [8/32] -p[min_is_black/rgb/mask] " );
    LOGGER_INFO ( " Exemple : mergeNtiff -f configfile.txt -a float -i nn -n -99999 -t image -s 1 -b 32 -p gray " );
}

/**
*@fn int h2i(char s)
* Hexadecimal -> int
*/

int h2i ( char s ) {
    if ( '0' <= s && s <= '9' )
        return ( s - '0' );
    if ( 'a' <= s && s <= 'f' )
        return ( s - 'a' + 10 );
    if ( 'A' <= s && s <= 'F' )
        return ( 10 + s - 'A' );
    else
        return -1; /* invalid input! */
}

/**
* @fn parseCommandLine(int argc, char** argv, char* imageListFilename, Kernel::KernelType& interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& sampleformat,  uint16_t& photometric)
* Lecture des parametres de la ligne de commande
*/

int parseCommandLine ( int argc, char** argv, char* imageListFilename, Kernel::KernelType& interpolation, int& nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& sampleformat,  uint16_t& photometric ) {

    char strnodata[10];

    if ( argc != 17 ) {
        LOGGER_ERROR ( " Nombre de parametres incorrect" );
        usage();
        return -1;
    }

    for ( int i = 1; i < argc; i++ ) {
        if ( argv[i][0] == '-' ) {
            switch ( argv[i][1] ) {
            case 'f': // fichier de liste des images source
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -f" );
                    return -1;
                }
                strcpy ( imageListFilename,argv[i] );
                break;
            case 'i': // interpolation
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -i" );
                    return -1;
                }
                if ( strncmp ( argv[i], "lanczos",7 ) == 0 ) interpolation = Kernel::LANCZOS_3; // =4
                else if ( strncmp ( argv[i], "nn",3 ) == 0 ) interpolation = Kernel::NEAREST_NEIGHBOUR; // =0
                else if ( strncmp ( argv[i], "bicubic",9 ) == 0 ) interpolation = Kernel::CUBIC; // =2
                else if ( strncmp ( argv[i], "linear",6 ) == 0 ) interpolation = Kernel::LINEAR; // =2
                else {
                    LOGGER_ERROR ( "Erreur sur l'option -i " );
                    return -1;
                }
                break;
            case 'n': // nodata
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -n" );
                    return -1;
                }
                strcpy ( strnodata,argv[i] );
                break;
            case 't': // type
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -t" );
                    return -1;
                }
                if ( strncmp ( argv[i], "image",5 ) == 0 ) type = 1 ;
                else if ( strncmp ( argv[i], "mtd",3 ) == 0 ) type = 0 ;
                else {
                    LOGGER_ERROR ( "Erreur sur l'option -t" );
                    return -1;
                }
                break;
            case 's': // sampleperpixel
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -s" );
                    return -1;
                }
                if ( strncmp ( argv[i], "1",1 ) == 0 ) sampleperpixel = 1 ;
                else if ( strncmp ( argv[i], "3",1 ) == 0 ) sampleperpixel = 3 ;
                else if ( strncmp ( argv[i], "4",1 ) == 0 ) sampleperpixel = 4 ;
                else {
                    LOGGER_ERROR ( "Erreur sur l'option -s" );
                    return -1;
                }
                break;
            case 'b': // bitspersample
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -b" );
                    return -1;
                }
                if ( strncmp ( argv[i], "8",1 ) == 0 ) bitspersample = 8 ;
                else if ( strncmp ( argv[i], "32",2 ) == 0 ) bitspersample = 32 ;
                else {
                    LOGGER_ERROR ( "Erreur sur l'option -b" );
                    return -1;
                }
                break;
            case 'a': // sampleformat
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -a" );
                    return -1;
                }
                if ( strncmp ( argv[i],"uint",4 ) == 0 ) sampleformat = SAMPLEFORMAT_UINT ;
                else if ( strncmp ( argv[i],"float",5 ) == 0 ) sampleformat = SAMPLEFORMAT_IEEEFP ;
                else {
                    LOGGER_ERROR ( "Erreur sur l'option -a" );
                    return -1;
                }
                break;
            case 'p': // photometric
                if ( i++ >= argc ) {
                    LOGGER_ERROR ( "Erreur sur l'option -p" );
                    return -1;
                }
                if ( strncmp ( argv[i], "gray",4 ) == 0 ) photometric = PHOTOMETRIC_MINISBLACK;
                else if ( strncmp ( argv[i], "rgb",3 ) == 0 ) photometric = PHOTOMETRIC_RGB;
                else if ( strncmp ( argv[i], "mask",4 ) == 0 ) photometric = PHOTOMETRIC_MASK;
                else {
                    LOGGER_ERROR ( "Erreur sur l'option -p" );
                    return -1;
                }
                break;
            default:
                usage();
                return -1;
            }
        }
    }

    LOGGER_DEBUG ( "mergeNtiff -f " << imageListFilename );

//  Nodata interpretation
    if ( bitspersample == 32 && sampleformat == SAMPLEFORMAT_IEEEFP ) {
        nodata = atoi ( strnodata );
        if ( nodata == 0 && strcmp ( strnodata,"0" ) !=0 ) {
            LOGGER_ERROR ( "Invalid parameter in -n argument for float samples,a integer in decimal format is expected" );
            return -1;
        }
    } else if ( bitspersample == 8 && sampleformat == SAMPLEFORMAT_UINT ) {
        int a1 = h2i ( strnodata[0] );
        int a0 = h2i ( strnodata[1] );
        if ( a1 < 0 || a0 < 0 ) {
            LOGGER_ERROR ( "Invalid parameter in -n argument for integer samples" );
            return -1;
        }
        nodata = 16*a1+a0;
    } else {
        LOGGER_ERROR ( "sampleformat/bitspersample not supported" );
        return -1;
    }

    return 0;
}

/**
* @fn int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric)
* @brief Enregistrement d'une image TIFF
* @param Image : Image a enregistrer
* @param pName : nom du fichier TIFF
* @param sampleperpixel : nombre de canaux de l'image TIFF
* @param bitspersample : nombre de bits par canal de l'image TIFF
* @param sampleformat : format des données binaires (uint ou float)
* @param photometric : valeur du tag TIFFTAG_PHOTOMETRIC de l'image TIFF
* @param nodata : valeur du pixel representant la valeur NODATA (6 caractère hexadécimaux)
* TODO : gerer tous les types de couleur pour la valeur NODATA
* @return : 0 en cas de succes, -1 sinon
*/

int saveImage ( Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric ) {
    // Ouverture du fichier
    LOGGER_DEBUG ( "Sauvegarde de l'image" << pName );
    TIFF* output=TIFFOpen ( pName,"w" );
    if ( output==NULL ) {
        LOGGER_ERROR ( "Impossible d'ouvrir le fichier " << pName << " en ecriture" );
        return -1;
    }

    // Ecriture de l'en-tete
    TIFFSetField ( output, TIFFTAG_IMAGEWIDTH, pImage->width );
    TIFFSetField ( output, TIFFTAG_IMAGELENGTH, pImage->height );
    TIFFSetField ( output, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel );
    TIFFSetField ( output, TIFFTAG_BITSPERSAMPLE, bitspersample );
    TIFFSetField ( output, TIFFTAG_SAMPLEFORMAT, sampleformat );
    TIFFSetField ( output, TIFFTAG_PHOTOMETRIC, photometric );
    TIFFSetField ( output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG );
    TIFFSetField ( output, TIFFTAG_COMPRESSION, COMPRESSION_NONE );
    TIFFSetField ( output, TIFFTAG_ROWSPERSTRIP, 1 );
    TIFFSetField ( output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE );

    // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if ( sampleformat==SAMPLEFORMAT_UINT ) {
        buf_u = ( unsigned char* ) _TIFFmalloc ( pImage->width * pImage->channels * bitspersample / 8 );
        for ( int line = 0; line < pImage->height; line++ ) {
            pImage->getline ( buf_u,line );
            TIFFWriteScanline ( output, buf_u, line, 0 );
        }
    } else if ( sampleformat==SAMPLEFORMAT_IEEEFP ) {
        buf_f = ( float* ) _TIFFmalloc ( pImage->width*pImage->channels*bitspersample/8 );
        for ( int line = 0; line < pImage->height; line++ ) {
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
* @fn int readFileLine(std::ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height)
* Lecture d une ligne du fichier de la liste d images source
*/

int readFileLine ( std::ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height ) {
    std::string str;
    std::getline ( file,str );
    double resx, resy;
    int nb;

    if ( ( nb=sscanf ( str.c_str(),"%s %lf %lf %lf %lf %lf %lf",filename, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, &resx, &resy ) ) ==7 ) {
        // Arrondi a la valeur entiere la plus proche
        *width = lround ( ( bbox->xmax - bbox->xmin ) /resx );
        *height = lround ( ( bbox->ymax - bbox->ymin ) /resy );
    }

    return nb;
}

/**
* @fn int loadImages(char* imageListFilename, LibtiffImage** ppImageOut, std::vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
* Chargement des images depuis le fichier texte donné en parametre
*/

int loadImages ( char* imageListFilename, LibtiffImage** ppImageOut, std::vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric ) {
    char filename[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    BoundingBox<double> bbox ( 0.,0.,0.,0. );
    int width, height;
    libtiffImageFactory factory;

    // Ouverture du fichier texte listant les images
    std::ifstream file;

    file.open ( imageListFilename );
    if ( !file ) {
        LOGGER_ERROR ( "Impossible d'ouvrir le fichier " << imageListFilename );
        return -1;
    }

    // Lecture et creation de l image de sortie
    if ( readFileLine ( file,filename,&bbox,&width,&height ) <0 ) {
        LOGGER_ERROR ( "Erreur lecture du fichier de parametres: " << imageListFilename << " a la ligne 0" );
        return -1;
    }

    *ppImageOut=factory.createLibtiffImage ( filename, bbox, width, height, sampleperpixel, bitspersample, photometric,COMPRESSION_NONE,16 );

    if ( *ppImageOut==NULL ) {
        LOGGER_ERROR ( "Impossible de creer " << filename );
        return -1;
    }

    // Lecture et creation des images source
    int nb=0,i;
    while ( ( nb=readFileLine ( file,filename,&bbox,&width,&height ) ) ==7 ) {
        LibtiffImage* pImage=factory.createLibtiffImage ( filename, bbox );
        if ( pImage==NULL ) {
            LOGGER_ERROR ( "Impossible de creer une image a partir de " << filename );
            return -1;
        }
        pImageIn->push_back ( pImage );
        i++;
    }
    if ( nb>=0 && nb!=7 ) {
        LOGGER_ERROR ( "Erreur lecture du fichier de parametres: " << imageListFilename << " a la ligne " << i );
        return -1;
    }

    // Fermeture du fichier
    file.close();

    return ( pImageIn->size() - 1 );
}


/**
* @fn int checkImages(LibtiffImage* pImageOut, std::vector<Image*>& ImageIn)
* @brief Controle des images
* TODO : ajouter des controles
*/

int checkImages ( LibtiffImage* pImageOut, std::vector<Image*>& ImageIn ) {
    for ( unsigned int i=0;i<ImageIn.size();i++ ) {
        if ( ImageIn.at ( i )->getresx() *ImageIn.at ( i )->getresy() ==0. ) {
            LOGGER_ERROR ( "Resolution de l image source " << i+1 << " sur " << ImageIn.size() << " egale a 0" );
            return -1;
        }
        if ( ImageIn.at ( i )->channels!=pImageOut->channels ) {
            LOGGER_ERROR ( "Nombre de canaux de l image source " << i+1 << " sur " << ImageIn.size() << " differente de l image de sortie" );
            return -1;
        }
    }
    if ( pImageOut->getresx() *pImageOut->getresy() ==0. ) {
        LOGGER_ERROR ( "Resolution de l image de sortie egale a 0 " << pImageOut->getfilename() );
        return -1;
    }
    if ( pImageOut->getbitspersample() !=8 && pImageOut->getbitspersample() !=32 ) {
        LOGGER_ERROR ( "Nombre de bits par sample de l image de sortie " << pImageOut->getfilename() << " non gere" );
        return -1;
    }

    return 0;
}

/**
* @fn double getPhasex(Image* pImage)
* @brief Calcul de la phase en X d'une image
*/

double getPhasex ( Image* pImage ) {
    double intpart;
    double phi=modf ( pImage->getxmin() /pImage->getresx(), &intpart );
    //NV le traitement suivant est vraiment louche. Pourquoi fait-on cela?
    if ( fabs ( 1-phi ) <pImage->getresx() /100. )
        phi=0.0000001; //NV cette valeur devrait etre calculee
    return phi;
}

/**
* @fn double getPhasey(Image* pImage)
* @brief Calcul de la phase en Y d'une image
*/

double getPhasey ( Image* pImage ) {
    double intpart;
    double phi=modf ( pImage->getymax() /pImage->getresy(), &intpart );
    //NV le traitement suivant est vraiment louche. Pourquoi fait-on cela?
    if ( fabs ( 1-phi ) <pImage->getresy() /100. )
        phi=0.0000001;//NV cette valeur devrait etre calculee
    return phi;
}

/* Teste si 2 images sont superposabbles */
bool areOverlayed ( Image* pImage1, Image* pImage2 ) {
    double epsilon_x=__min ( pImage1->getresx(), pImage2->getresx() ) /100.;
    double epsilon_y=__min ( pImage1->getresy(), pImage2->getresy() ) /100.;

    if ( fabs ( pImage1->getresx()-pImage2->getresx() ) > epsilon_x ) return false;
    if ( fabs ( pImage1->getresy()-pImage2->getresy() ) > epsilon_y ) return false;
    if ( fabs ( getPhasex ( pImage1 )-getPhasex ( pImage2 ) ) > epsilon_x ) return false;
    if ( fabs ( getPhasey ( pImage1 )-getPhasey ( pImage2 ) ) > epsilon_y ) return false;
    return true;
}

/* Fonctions d'ordre */
bool InfResx ( Image* pImage1, Image* pImage2 ) {
    return ( pImage1->getresx() <pImage2->getresx() - __min ( pImage1->getresx(), pImage2->getresx() ) /100. );
}
bool InfResy ( Image* pImage1, Image* pImage2 ) {
    return ( pImage1->getresy() <pImage2->getresy() - __min ( pImage1->getresy(), pImage2->getresy() ) /100. );
}
bool InfPhasex ( Image* pImage1, Image* pImage2 ) {
    return ( getPhasex ( pImage1 ) <getPhasex ( pImage2 ) - __min ( pImage1->getresx(), pImage2->getresx() ) /100. );
}
bool InfPhasey ( Image* pImage1, Image* pImage2 ) {
    return ( getPhasey ( pImage1 ) <getPhasey ( pImage2 ) - __min ( pImage1->getresy(), pImage2->getresy() ) /100. );
}

/**
* @brief Tri des images source en paquets d images superposables (memes phases et resolutions en x et y)
* @param ImageIn : vecteur contenant les images non triees
* @param pTabImageIn : tableau de vecteurs conteant chacun des images superposables
* @return 0 en cas de succes, -1 sinon
*/

int sortImages ( std::vector<Image*> ImageIn, std::vector<std::vector<Image*> >* pTabImageIn ) {
    std::vector<Image*> vTmp;

    // Initilisation du tableau de sortie
    pTabImageIn->push_back(ImageIn);

    // Creation de vecteurs contenant des images avec une resolution en x homogene
    // TODO : Attention, ils ne sont forcement en phase
    for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
    {
        std::stable_sort(it->begin(),it->end(),InfResx); 
        for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
        if ( fabs((*it2)->getresy()-(*(it2+1))->getresy()) > __min((*it2)->getresy(), (*(it2+1))->getresy())/100.)
        {
            vTmp.assign(it2+1,it->end());
            it->assign(it->begin(),it2+1);
            pTabImageIn->push_back(vTmp);
            return 0;
            it++;
        }
    }

//TODO : A refaire proprement
    /*
        // Creation de vecteurs contenant des images avec une resolution en x et en y homogenes
            for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
            {
                    std::sort(it->begin(),it->end(),InfResy);
                    for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                        if ((*it2)->getresy()!=(*(it2+1))->getresy() && it2+2!=it->end())
                        {
                                it->assign(it->begin(),it2);
                                vTmp.assign(it2+1,it->end());
                                pTabImageIn->push_back(vTmp);
                                it++;
                        }
            }

        // Creation de vecteurs contenant des images avec une resolution en x et en y, et une phase en x homogenes
            for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
            {
                    std::sort(it->begin(),it->end(),InfPhasex);
                    for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                        if (getPhasex(*it2)!=getPhasex(*(it2+1)) && it2+2!=it->end())
                        {
                                it->assign(it->begin(),it2);
                                vTmp.assign(it2+1,it->end());
                                pTabImageIn->push_back(vTmp);
                                it++;
                        }
            }

        // Creation de vecteurs contenant des images superposables
            for (std::vector<std::vector<Image*> >::iterator it=pTabImageIn->begin();it<pTabImageIn->end();it++)
            {
                    std::sort(it->begin(),it->end(),InfPhasey);
                    for (std::vector<Image*>::iterator it2 = it->begin();it2+1<it->end();it2++)
                        if (getPhasey(*it2)!=getPhasey(*(it2+1)) && it2+2!=it->end())
                        {
                                it->assign(it->begin(),it2);
                                vTmp.assign(it2+1,it->end());
                                pTabImageIn->push_back(vTmp);
                                it++;
                        }
            }
    */
    return 0;
}

/**
* @fn ExtendedCompoundImage* compoundImages(std::vector< Image*> & TabImageIn,char* nodata, uint16_t sampleformat, uint mirrors)
* @brief Assemblage d images superposables
* @param TabImageIn : vecteur d images a assembler
* @return Image composee de type ExtendedCompoundImage
*/

ExtendedCompoundImage* compoundImages ( std::vector< Image*> & TabImageIn,int nodata, uint16_t sampleformat, uint mirrors ) {
    if ( TabImageIn.empty() ) {
        LOGGER_ERROR ( "Assemblage d'un tableau d images de taille nulle" );
        return NULL;
    }

    // Rectangle englobant des images d entree
    double xmin=1E12, ymin=1E12, xmax=-1E12, ymax=-1E12 ;
    for ( unsigned int j=0;j<TabImageIn.size();j++ ) {
        if ( TabImageIn.at ( j )->getxmin() <xmin )  xmin=TabImageIn.at ( j )->getxmin();
        if ( TabImageIn.at ( j )->getymin() <ymin )  ymin=TabImageIn.at ( j )->getymin();
        if ( TabImageIn.at ( j )->getxmax() >xmax )  xmax=TabImageIn.at ( j )->getxmax();
        if ( TabImageIn.at ( j )->getymax() >ymax )  ymax=TabImageIn.at ( j )->getymax();
    }

    extendedCompoundImageFactory ECImgfactory ;
    int w= ( int ) ( ( xmax-xmin ) / ( *TabImageIn.begin() )->getresx() +0.5 ), h= ( int ) ( ( ymax-ymin ) / ( *TabImageIn.begin() )->getresy() +0.5 );
    ExtendedCompoundImage* pECI = ECImgfactory.createExtendedCompoundImage ( w,h, ( *TabImageIn.begin() )->channels, BoundingBox<double> ( xmin,ymin,xmax,ymax ), TabImageIn,nodata,sampleformat,mirrors );

    return pECI ;
}

/**
* @fn uint addMirrors(ExtendedCompoundImage* pECI)
* @brief Ajout de miroirs a une ExtendedCompoundImage
* L'image en entree doit etre composee d'un assemblage regulier d'images (de type CompoundImage)
* Objectif : mettre des miroirs la ou il n'y a pas d'images afin d'eviter des effets de bord en cas de reechantillonnage
* @param pECI : l'image à completer
* @return : le nombre de miroirs ajoutes
*/

uint addMirrors ( ExtendedCompoundImage* pECI ) {
    uint mirrors=0;

    int w=pECI->getimages()->at ( 0 )->width;
    int h=pECI->getimages()->at ( 0 )->height;
    double resx=pECI->getimages()->at ( 0 )->getresx();
    double resy=pECI->getimages()->at ( 0 )->getresy();
    double epsilon_x = resx / 100.;
    double epsilon_y = resx / 100.;

    int i,j;
    double intpart;
    for ( i=1;i<pECI->getimages()->size();i++ ) {
        if ( abs ( pECI->getimages()->at ( i )->getresx() - resx ) > epsilon_x
                || abs ( pECI->getimages()->at ( i )->getresy() - resy ) > epsilon_y
                || pECI->getimages()->at ( i )->width != w
                || pECI->getimages()->at ( i )->height != h
                || abs ( modf ( pECI->getimages()->at ( i )->getxmin() - pECI->getxmin() / ( w*resx ),&intpart ) ) > epsilon_x
                || abs ( modf ( pECI->getimages()->at ( i )->getymax() - pECI->getymax() / ( h*resy ),&intpart ) ) > epsilon_y ) {
            LOGGER_WARN ( "Image composite irreguliere : impossible d'ajouter des miroirs" );
            return 0;
        }
    }

    int nx= ( int ) floor ( ( pECI->getxmax()-pECI->getxmin() ) / ( w*resx ) + 0.5 ), // taille de l'ECI en nombre d'images en x
            ny= ( int ) floor ( ( pECI->getymax()-pECI->getymin() ) / ( h*resy ) + 0.5 ), // taille de l'ECI en nombre d'images en y
                n=pECI->getimages()->size();

    LOGGER_DEBUG ( "xmin:"<<pECI->getxmin() << " xmax:" << pECI->getxmax() << " w:" << w << " nx:" << nx );
    LOGGER_DEBUG ( "ymin:"<<pECI->getymin() << " ymax:" << pECI->getymax() << " h:" << h << " ny:" << ny );

    unsigned int k,l;
    Image*pI0,*pI1,*pI2,*pI3;
    double xmin,ymax;
    mirrorImageFactory MIFactory;

    for ( i=-1; i < nx+1; i++ )
        for ( j=-1; j < ny+1; j++ ) {
            LOGGER_DEBUG ( "I:"<<i<<" J:"<<j );

            if ( ( i==-1 && j==-1 ) || ( i==-1 && j==ny ) || ( i==nx && j==-1 ) || ( i==nx && j==ny ) )
                /* NV: On ne fait pas de miroirs dans les angles. Je me demande si ca ne pose pas un probleme au final */
                continue;

            for ( k=0;k<n;k++ ) {
                if ( ( fabs ( pECI->getimages()->at ( k )->getxmin() - ( pECI->getxmin() +i*w*resx ) ) < epsilon_x )
                        && ( fabs ( pECI->getimages()->at ( k )->getymax() - ( pECI->getymax()-j*h*resy ) ) < epsilon_y ) ) {
                    LOGGER_DEBUG ( "k:"<<k<<" xmin:"<<pECI->getimages()->at ( k )->getxmin() << " ymax:"<<pECI->getimages()->at ( k )->getymax() );
                    break;
                }
            }

            /* k==n implique qu'on a pas d'image dans le ECI à la position i,j*/
            if ( k==n ) {
                LOGGER_DEBUG ( "=> CALCUL DE MIROIR" );

                // Image 0
                pI0=NULL;
                xmin=pECI->getxmin() + ( i-1 ) *w*resx;
                ymax=pECI->getymax()-j*h*resy;
                for ( l=0;l<n;l++ )
                    if ( abs ( pECI->getimages()->at ( l )->getxmin() - xmin ) < epsilon_x && abs ( pECI->getimages()->at ( l )->getymax()-ymax ) < epsilon_y )
                        break;
                if ( l<n )
                    pI0=pECI->getimages()->at ( l );
                // Image 1
                pI1=NULL;
                xmin=pECI->getxmin() +i*w*resx;
                ymax=pECI->getymax()- ( j-1 ) *h*resy;
                for ( l=0;l<n;l++ )
                    if ( abs ( pECI->getimages()->at ( l )->getxmin() - xmin ) < epsilon_x && abs ( pECI->getimages()->at ( l )->getymax()-ymax ) <epsilon_y )
                        break;
                if ( l<n )
                    pI1=pECI->getimages()->at ( l );
                // Image 2
                pI2=NULL;
                xmin=pECI->getxmin() + ( i+1 ) *w*resx;
                ymax=pECI->getymax()-j*h*resy;
                for ( l=0;l<n;l++ )
                    if ( abs ( pECI->getimages()->at ( l )->getxmin() - xmin ) < epsilon_x && abs ( pECI->getimages()->at ( l )->getymax()-ymax ) <epsilon_y )
                        break;
                if ( l<n )
                    pI2=pECI->getimages()->at ( l );
                // Image 3
                pI3=NULL;
                xmin=pECI->getxmin() +i*w*resx;
                ymax=pECI->getymax()- ( j+1 ) *h*resy;
                for ( l=0;l<n;l++ )
                    if ( abs ( pECI->getimages()->at ( l )->getxmin() - xmin ) < epsilon_x && abs ( pECI->getimages()->at ( l )->getymax()-ymax ) <epsilon_y )
                        break;
                if ( l<n )
                    pI3=pECI->getimages()->at ( l );

                MirrorImage* mirror=MIFactory.createMirrorImage ( pI0,pI1,pI2,pI3 );

                if ( mirror!=NULL ) {
                    pECI->getimages()->push_back ( mirror );
                    //LOGGER_DEBUG("Ajout miroir "<<i<<"/"<<nx<<"     "<<j<<"/"<<ny);
                    //LOGGER_DEBUG(mirrors);
                    mirrors++;
                }
            }
        }
    //LOGGER_DEBUG(mirrors);
    return mirrors;
}


/**
* @fn ResampledImage* resampleImages(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Kernel::KernelType& interpolation, ExtendedCompoundMaskImage* mask, ResampledImage*& resampledMask)
* @brief Reechantillonnage d'une image de type ExtendedCompoundImage
* @brief Objectif : la rendre superposable a l'image finale
* @return Image reechantillonnee legerement plus petite
*/

ResampledImage* resampleImages ( LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Kernel::KernelType& interpolation, ExtendedCompoundMaskImage* mask, ResampledImage*& resampledMask ) {
    const Kernel& K = Kernel::getInstance ( interpolation );

    double xmin_src=pECI->getxmin(), ymin_src=pECI->getymin(), xmax_src=pECI->getxmax(), ymax_src=pECI->getymax();
    double resx_src=pECI->getresx(), resy_src=pECI->getresy(), resx_dst=pImageOut->getresx(), resy_dst=pImageOut->getresy();
    double ratio_x=resx_dst/resx_src, ratio_y=resy_dst/resy_src;

    // L'image reechantillonnee est limitee a l'image de sortie
    double xmin_dst=__max ( xmin_src+K.size ( ratio_x ) *resx_src,pImageOut->getxmin() ), xmax_dst=__min ( xmax_src-K.size ( ratio_x ) *resx_src,pImageOut->getxmax() ),
                    ymin_dst=__max ( ymin_src+K.size ( ratio_y ) *resy_src,pImageOut->getymin() ), ymax_dst=__min ( ymax_src-K.size ( ratio_y ) *resy_src,pImageOut->getymax() );

    // Exception : l'image d'entree n'intersecte pas l'image finale
    if ( xmax_src-K.size ( ratio_x ) *resx_src<pImageOut->getxmin() || xmin_src+K.size ( ratio_x ) *resx_src>pImageOut->getxmax() || ymax_src-K.size ( ratio_y ) *resy_src<pImageOut->getymin() || ymin_src+K.size ( ratio_y ) *resy_src>pImageOut->getymax() ) {
        LOGGER_WARN ( "Un paquet d'images (homogenes en résolutions et phase) est situe entierement a l'exterieur de l image finale" );
        return NULL;
    }

    // Coordonnees de l'image reechantillonnee en pixels
    xmin_dst/=resx_dst;
    xmin_dst=floor ( xmin_dst+0.1 );
    ymin_dst/=resy_dst;
    ymin_dst=floor ( ymin_dst+0.1 );
    xmax_dst/=resx_dst;
    xmax_dst=ceil ( xmax_dst-0.1 );
    ymax_dst/=resy_dst;
    ymax_dst=ceil ( ymax_dst-0.1 );
    // Dimension de l'image reechantillonnee
    int width_dst = int ( xmax_dst-xmin_dst+0.1 );
    int height_dst = int ( ymax_dst-ymin_dst+0.1 );
    xmin_dst*=resx_dst;
    xmax_dst*=resx_dst;
    ymin_dst*=resy_dst;
    ymax_dst*=resy_dst;

    double off_x= ( xmin_dst-xmin_src ) /resx_src,off_y= ( ymax_src-ymax_dst ) /resy_src;

    BoundingBox<double> bbox_dst ( xmin_dst, ymin_dst, xmax_dst, ymax_dst );
    // Reechantillonnage
    ResampledImage* pRImage = new ResampledImage ( pECI, width_dst, height_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst );

    //saveImage(pRImage,"test1.tif",3,8,1,PHOTOMETRIC_RGB);

    // Reechantillonage du masque
    resampledMask = new ResampledImage ( mask, width_dst, height_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst );
    return pRImage;
}

/**
* @fn int mergeTabImages(LibtiffImage* pImageOut, std::vector<std::vector<Image*> >& TabImageIn, ExtendedCompoundImage** ppECImage, Kernel::KernelType& interpolation, char* nodata, uint16_t sampleformat)
* @brief Fusion des images
* @param pImageOut : image de sortie
* @param TabImageIn : tableau de vecteur d images superposables
* @param ppECImage : image composite creee
* @param interpolation : type d'interpolation utilise
* @return 0 en cas de succes, -1 sinon
*/

int mergeTabImages ( LibtiffImage* pImageOut, std::vector<std::vector<Image*> >& TabImageIn, ExtendedCompoundImage** ppECImage, Kernel::KernelType& interpolation, int nodata, uint16_t sampleformat ) {
    extendedCompoundImageFactory ECImgfactory ;
    std::vector<Image*> pOverlayedImage;
    std::vector<Image*> pMask;

    for ( unsigned int i=0; i<TabImageIn.size(); i++ ) {
        // Mise en superposition du paquet d'images en 2 etapes

        // Etape 1 : Creation d'une image composite
        ExtendedCompoundImage* pECI = compoundImages ( TabImageIn.at ( i ),nodata,sampleformat,0 );
        ExtendedCompoundMaskImage* mask;// = new ExtendedCompoundMaskImage(pECI);

        if ( pECI==NULL ) {
            LOGGER_ERROR ( "Impossible d'assembler les images" );
            return -1;
        }

        //saveImage(pECI,"test0.tif",3,8,1,PHOTOMETRIC_RGB);

        if ( areOverlayed ( pImageOut,pECI ) ) {
            /* les images sources et finale ont la meme res et la meme phase
             * on aura donc pas besoin de reechantillonnage.*/
            pOverlayedImage.push_back ( pECI );
            //saveImage(pECI,"test0.tif",3,8,1,PHOTOMETRIC_RGB);
            mask = new ExtendedCompoundMaskImage ( pECI );
            pMask.push_back ( mask );
        } else {
            // Etape 2 : Reechantillonnage de l'image composite si necessaire

            uint mirrors=addMirrors ( pECI );

            ExtendedCompoundImage* pECI_withMirrors=compoundImages ( ( *pECI->getimages() ),nodata,sampleformat,mirrors );

            // LOGGER_DEBUG(mirrors<<" "<<pECI_withMirrors->getmirrors()<<" "<<pECI_withMirrors->getimages()->size());

            //saveImage(pECI,"test2.tif",3,8,1,PHOTOMETRIC_RGB);
            //saveImage(pECI_withMirrors,"test2bis.tif",3,8,1,PHOTOMETRIC_RGB);
            //return -1;

            mask = new ExtendedCompoundMaskImage ( pECI_withMirrors );

            ResampledImage* pResampledMask;
            ResampledImage* pRImage = resampleImages ( pImageOut, pECI_withMirrors, interpolation, mask, pResampledMask );

            if ( pRImage==NULL ) {
                LOGGER_ERROR ( "Impossible de reechantillonner les images" );
                return -1;
            }
            pOverlayedImage.push_back ( pRImage );
            //saveImage(pRImage,"test3.tif",3,8,1,PHOTOMETRIC_RGB);
            pMask.push_back ( pResampledMask );
            //saveImage(pRImage,"rimage.tif",3,8,1,PHOTOMETRIC_RGB);
            //saveImage(mask,"mask.tif",1,8,1,PHOTOMETRIC_MINISBLACK);
            //saveImage(pResampledMask,"rmask.tif",1,8,1,PHOTOMETRIC_MINISBLACK);
        }
    }

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    if ( ( *ppECImage = ECImgfactory.createExtendedCompoundImage ( pImageOut->width, pImageOut->height,
                        pImageOut->channels, pImageOut->getbbox(), pOverlayedImage,pMask,nodata,sampleformat,0 ) ) ==NULL ) {
        LOGGER_ERROR ( "Erreur lors de la fabrication de l image finale" );
        return -1;
    }

    return 0;
}

/**
* @fn int main(int argc, char **argv)
* @brief Fonction principale
*/

int main ( int argc, char **argv ) {
    char imageListFilename[256];
    int nodata;
    uint16_t sampleperpixel, bitspersample, sampleformat, photometric;
    int type=-1;
    Kernel::KernelType interpolation;

    LibtiffImage* pImageOut ;
    std::vector<Image*> ImageIn;
    std::vector<std::vector<Image*> > TabImageIn;
    ExtendedCompoundImage* pECImage;

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

    // Lecture des parametres de la ligne de commande
    if ( parseCommandLine ( argc, argv,imageListFilename,interpolation,nodata,type,sampleperpixel,bitspersample,sampleformat,photometric ) <0 ) {
        LOGGER_ERROR ( "Echec lecture ligne de commande" );
        sleep ( 1 );
        return -1;
    }

    // TODO : gérer le type mtd !!
    if ( type==0 ) {
        LOGGER_ERROR ( "Le type mtd n'est pas pris en compte" );
        sleep ( 1 );
        return -1;
    }

    LOGGER_DEBUG ( "Load" );
    // Chargement des images
    if ( loadImages ( imageListFilename,&pImageOut,&ImageIn,sampleperpixel,bitspersample,photometric ) <0 ) {
        LOGGER_ERROR ( "Echec chargement des images" );
        sleep ( 1 );
        return -1;
    }


    LOGGER_DEBUG ( "Check" );
    // Controle des images
    if ( checkImages ( pImageOut,ImageIn ) <0 ) {
        LOGGER_ERROR ( "Echec controle des images" );
        sleep ( 1 );
        return -1;
    }
    LOGGER_DEBUG ( "Sort" );
    // Tri des images
    if ( sortImages ( ImageIn, &TabImageIn ) <0 ) {
        LOGGER_ERROR ( "Echec tri des images" );
        sleep ( 1 );
        return -1;
    }
    
std::cout << "nombre de paquet d'image : " << TabImageIn.size() << std::endl; // #TEST#
    
    LOGGER_DEBUG ( "Merge" );
    // Fusion des paquets d images
    if ( mergeTabImages ( pImageOut, TabImageIn, &pECImage, interpolation,nodata,sampleformat ) < 0 ) {
        LOGGER_ERROR ( "Echec fusion des paquets d images" );
        sleep ( 1 );
        return -1;
    }
    LOGGER_DEBUG ( "Save" );
    // Enregistrement de l image fusionnee
    if ( saveImage ( pECImage,pImageOut->getfilename(),pImageOut->channels,bitspersample,sampleformat,photometric ) <0 ) {
        LOGGER_ERROR ( "Echec enregistrement de l image finale" );
        sleep ( 1 );
        return -1;
    }

    // Nettoyage
    delete pImageOut;
    delete pECImage;

    return 0;
}

