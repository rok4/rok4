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
 * \~french \brief Création d'une image TIFF géoréférencée à partir de n images TIFF sources géoréférencées
 * \~english \brief Create one georeferenced TIFF image from several georeferenced TIFF images
 * \~ \image html mergeNtiff.png
 * \~french \details Les images en entrée peuvent :
 * \li être de différentes résolutions
 * \li ne pas couvrir entièrement l'emprise de l'image de sortie
 * \li être recouvrantes entre elles
 * 
 * Les images en entrée et celle en sortie doivent avoir les même composantes suivantes :
 * \li le même système spatial de référence
 * \li le même nombre de canaux
 * \li le même format de canal
 * 
 * Les formats des canaux gérés sont :
 * \li entier non signé sur 8 bits
 * \li flottant sur 32 bits
 *
 * On doit réciser en paramètre de la commande :
 * \li Un fichier texte contenant les images sources et l'image finale avec leur georeferencement (resolution, emprise). On peut trouver également les masques associés aux images.
 * Format d'une ligne du fichier : \code<TYPE> <CHEMIN> <XMIN> <YMAX> <XMAX> <YMIN> <RESX> <RESY>\endcode
 * Exemple de configuration :
 * \~ \code{.txt}
 * IMG IMAGE.tif      -499       1501    1501    -499       2       2
 * MSK MASK.tif
 * IMG sources/imagefond.tif      -499       1501    1501    -499       4       4
 * MSK sources/maskfond.tif
 * IMG sources/image1.tif      0       1000    1000    0       1       1
 * MSK sources/mask1.tif
 * IMG sources/image2.tif      500       1500    1500    500       1       1
 * MSK sources/mask2.tif
 * \endcode
 * \~french
 * \li Le type de la donnée traitée (Data/Metadata)
 * \li La compression de l'image de sortie
 * \li Le mode d'interpolation
 * \li Le nombre de canaux par pixel
 * \li Le nombre de bits par canal
 * \li Le format du canal (entier ou flottant)
 * \li La valeur de non-donnée
 *
 * Pour réaliser la fusion des images en entrée, on traite différemment :
 * \li les images qui sont superposables à l'image de sortie (mêmes résolutions, mêmes phases) : on parle alors d'images compatibles, pas de réechantillonnage nécessaire.
 * \li les images non compatibles : un passage par le réechntillonnage (plus lourd en calcul) est indispensable.
 *
 * Exemple d'appel à la commande :
 * \~french \li pour des ortho-images \~english \li for orthoimage
 * \~ \code
 * mergeNtiff -f conf.txt -t image -c zip -i bicubic -s 3 -b 8 -p rgb -a uint -n 255,255,255
 * \endcode
 * \~french \li pour du MNT \~english \li for DTM
 * \~ \code
 * mergeNtiff -f conf.txt -t image -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999
 * \endcode
 * 
 * \~french
 * \todo Gérer correctement un canal alpha
 * \todo Permettre l'ajout ou la suppression à la volée d'un canal alpha
 */

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
#include "ExtendedCompoundImage.h"
#include "MirrorImage.h"
#include "Interpolation.h"
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
/** \~french Nombre de canaux par pixel, dans les images en entrée et celle en sortie */
uint16_t samplesperpixel;
/** \~french Nombre de bits par canal, dans les images en entrée et celle en sortie */
uint16_t bitspersample;
/** \~french Format du canal (entier, flottant), dans les images en entrée et celle en sortie */
uint16_t sampleformat;
/** \~french Photométrie (rgb, gray), dans les images en entrée et celle en sortie */
uint16_t photometric;
/** \~french Compression de l'image de sortie */
uint16_t compression;
/** \~french Type de donnée traitée : image (1) ou meta-donnée (0, non implémenté) */
int type=-1;
/** \~french Interpolation utilisée pour le réechantillonnage */
Interpolation::KernelType interpolation;

/**
 * \~french
 * \brief Affiche l'utilisation et les différentes options de la commande mergeNtiff
 * \details L'affichage se fait dans le niveau de logger INFO
 */
void usage() {
    LOGGER_INFO(
    "mergeNtiff version "<< BE4_VERSION << std::endl <<
    " Usage :  mergeNtiff -f [fichier liste des images source] -c [none/png/jpg/lzw/zip/pkb] -a [uint/float] -i [lanczos/nn/linear/bicubic] -n [couleur NoData] -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask] " << std::endl << 
    " Exemple : mergeNtiff -f configfile.txt -a float -i nn -n -99999 -t image -s 1 -b 32 -p gray ");
}

/**
 * \~french
 * \brief Affiche un message d'erreur, l'utilisation de la commande et sort en erreur (code de retour -1)
 * \param[in] message message d'erreur
 */
void error(std::string message) {
    LOGGER_ERROR(message);
    LOGGER_ERROR("Configuration file : " << imageListFilename);
    usage();
    sleep(1);
    exit(-1);
}

/**
 * \~french
 * \brief Récupère les valeurs passées en paramètres de la commande, et les stocke dans les variables globales
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 */
int parseCommandLine(int argc, char** argv) {
    
    if (argc != 19) {
        LOGGER_ERROR(" Nombre de parametres incorrect");
        return -1;
    }

    for(int i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            switch(argv[i][1]) {
                case 'f': // fichier de liste des images source
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -f"); return -1;}
                    strcpy(imageListFilename,argv[i]);
                    break;
                case 'i': // interpolation
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -i"); return -1;}
                    if(strncmp(argv[i], "lanczos",7) == 0) interpolation = Interpolation::LANCZOS_3; // =4
                    else if(strncmp(argv[i], "nn",2) == 0) interpolation = Interpolation::NEAREST_NEIGHBOUR; // =0
                    else if(strncmp(argv[i], "bicubic",7) == 0) interpolation = Interpolation::CUBIC; // =2
                    else if(strncmp(argv[i], "linear",6) == 0) interpolation = Interpolation::LINEAR; // =2
                    else {LOGGER_ERROR("Erreur sur l'option -i "); return -1;}
                    break;
                case 'n': // nodata
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -n"); return -1;}
                    strcpy(strnodata,argv[i]);
                    break;
                case 't': // type
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -t"); return -1;}
                    if(strncmp(argv[i], "image",5) == 0) type = 1 ;
                    else if(strncmp(argv[i], "mtd",3) == 0) {
                        LOGGER_ERROR("Le type mtd n'est pas pris en compte");
                        return -1;
                    }
                    else {LOGGER_ERROR("Erreur sur l'option -t"); return -1;}
                    break;
                case 's': // sampleperpixel
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                    if(strncmp(argv[i], "1",1) == 0) samplesperpixel = 1 ;
                    else if(strncmp(argv[i], "3",1) == 0) samplesperpixel = 3 ;
                    else if(strncmp(argv[i], "4",1) == 0) samplesperpixel = 4 ;
                    else {LOGGER_ERROR("Erreur sur l'option -s"); return -1;}
                    break;
                case 'b': // bitspersample
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -b"); return -1;}
                    if(strncmp(argv[i], "8",1) == 0) bitspersample = 8 ;
                    else if(strncmp(argv[i], "32",2) == 0) bitspersample = 32 ;
                    else {LOGGER_ERROR("Erreur sur l'option -b"); return -1;}
                    break;
                case 'a': // sampleformat
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -a"); return -1;}
                    if(strncmp(argv[i],"uint",4) == 0) sampleformat = SAMPLEFORMAT_UINT ;
                    else if(strncmp(argv[i],"float",5) == 0) sampleformat = SAMPLEFORMAT_IEEEFP ;
                    else {LOGGER_ERROR("Erreur sur l'option -a"); return -1;}
                    break;
                case 'p': // photometric
                    if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -p"); return -1;}
                    if(strncmp(argv[i], "gray",4) == 0) photometric = PHOTOMETRIC_MINISBLACK;
                    else if(strncmp(argv[i], "rgb",3) == 0) photometric = PHOTOMETRIC_RGB;
                    else if(strncmp(argv[i], "mask",4) == 0) photometric = PHOTOMETRIC_MASK;
                    else {LOGGER_ERROR("Erreur sur l'option -p"); return -1;}
                    break;
                case 'c': // compression
                   if(i++ >= argc) {LOGGER_ERROR("Erreur sur l'option -c"); return -1;}
                   if(strncmp(argv[i], "none",4) == 0) compression = COMPRESSION_NONE;
                   else if(strncmp(argv[i], "zip",3) == 0) compression = COMPRESSION_ADOBE_DEFLATE;
                   else if(strncmp(argv[i], "pkb",3) == 0) compression = COMPRESSION_PACKBITS;
                   else if(strncmp(argv[i], "jpg",3) == 0) compression = COMPRESSION_JPEG;
                   else if(strncmp(argv[i], "lzw",3) == 0) compression = COMPRESSION_LZW;
                   else compression = COMPRESSION_NONE;
                   break;
                default: return -1;
            }
        }
    }

    LOGGER_DEBUG("mergeNtiff -f " << imageListFilename);
    
    if (! ((bitspersample == 32 && sampleformat == SAMPLEFORMAT_IEEEFP) || 
            (bitspersample == 8 && sampleformat == SAMPLEFORMAT_UINT)) ){
        LOGGER_ERROR("sampleformat/bitspersample not supported");
        return -1;
    }
    
    return 0;
}

/**
 * \~french
 * \brief Enregistre une image TIFF, avec passage de ses composantes
 * \details Toutes les informations nécessaires à l'écriture d'une image n'étant pas stockées dans un objet Image, on se doit de les préciser en paramètre de la fonction.
 * \param[in] pImage image à enregistrer
 * \param[in] pName chemin de l'image à écrire
 * \param[in] bps nombre de bits par canal de l'image TIFF
 * \param[in] sf format des canaux (entier ou fottant)
 * \param[in] ph photométrie de l'image à écrire
 * \param[in] comp compression de l'image à écrire
 * \return 0 en cas de succes, -1 sinon
 */
int saveImage(Image *pImage, char* pName, uint16_t bps, uint16_t sf, uint16_t ph, uint16_t comp) {
    // Ouverture du fichier
    /*TEST*//*
    LOGGER_INFO("Sauvegarde de l'image " << pName);
    pImage->print();*/
    TIFF* output=TIFFOpen(pName,"w");
    if (output==NULL) {
        LOGGER_ERROR("Impossible d'ouvrir le fichier " << pName << " en ecriture");
        return -1;
    }

    // Ecriture de l'en-tete
    TIFFSetField(output, TIFFTAG_IMAGEWIDTH, pImage->width);
    TIFFSetField(output, TIFFTAG_IMAGELENGTH, pImage->height);
    TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, pImage->channels);
    TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bps);
    TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, sf);
    TIFFSetField(output, TIFFTAG_PHOTOMETRIC, ph);
    TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(output, TIFFTAG_COMPRESSION, comp);
    TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

    // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if (sf==SAMPLEFORMAT_UINT){
        buf_u = (unsigned char*)_TIFFmalloc(pImage->width * pImage->channels * bps / 8);
        for( int line = 0; line < pImage->height; line++) {
            pImage->getline(buf_u,line);
            TIFFWriteScanline(output, buf_u, line, 0);
        }
    }
    else if(sf==SAMPLEFORMAT_IEEEFP){
        buf_f = (float*)_TIFFmalloc(pImage->width*pImage->channels*bps/8);
        for( int line = 0; line < pImage->height; line++) {
            pImage->getline(buf_f,line);
            TIFFWriteScanline(output, buf_f, line, 0);
        }
    }

        // Liberation
    if (buf_u) _TIFFfree(buf_u);
    if (buf_f) _TIFFfree(buf_f);
        TIFFClose(output);
        return 0;
}

/**
 * \~french
 * \brief Enregistre une image TIFF, sans passage de ses composantes
 * \details Toutes les informations nécessaires à l'écriture d'une image sont dans un objet LibtiffImage. Cependant, les données sources de l'image sont transmis via une seconde image.
 * \param[in] pOut image à enregistrer
 * \param[in] pIn source des donnée de l'image à écrire
 * \return 0 en cas de succes, -1 sinon
 */
int saveImage(LibtiffImage* pOut, Image* pIn) {
    // Ouverture du fichier
    TIFF* output=TIFFOpen(pOut->getfilename(),"w");
    if (output==NULL) {
        LOGGER_ERROR("Impossible d'ouvrir le fichier " << pOut->getfilename() << " en ecriture");
        return -1;
    }

    // Ecriture de l'en-tete
    TIFFSetField(output, TIFFTAG_IMAGEWIDTH, pOut->width);
    TIFFSetField(output, TIFFTAG_IMAGELENGTH, pOut->height);
    TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, pOut->channels);
    TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, pOut->getbitspersample());
    TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, pOut->getsampleformat());
    TIFFSetField(output, TIFFTAG_PHOTOMETRIC, pOut->getphotometric());
    TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(output, TIFFTAG_COMPRESSION, pOut->getcompression());
    TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
    TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

    // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if (pOut->getsampleformat() == SAMPLEFORMAT_UINT){
        buf_u = (unsigned char*)_TIFFmalloc(pOut->width * pOut->channels * pOut->getbitspersample() / 8);
        for( int line = 0; line < pOut->height; line++) {
            pIn->getline(buf_u,line);
            TIFFWriteScanline(output, buf_u, line, 0);
        }
    }
    else if(pOut->getsampleformat() == SAMPLEFORMAT_IEEEFP){
        buf_f = (float*)_TIFFmalloc(pOut->width*pOut->channels*pOut->getbitspersample()/8);
        for( int line = 0; line < pOut->height; line++) {
            pIn->getline(buf_f,line);
            TIFFWriteScanline(output, buf_f, line, 0);
        }
    }

    // Liberation
    if (buf_u) _TIFFfree(buf_u);
    if (buf_f) _TIFFfree(buf_f);
        TIFFClose(output);
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
 * \param[out] bbox rectangle englobant de l'image lue (et de son masque)
 * \param[out] width largeur en pixel de l'image lue (et de son masque)
 * \param[out] height hauteur en pixel de l'image lue (et de son masque)
 * \param[out] resx résolution en X de l'image lue (et de son masque)
 * \param[out] resy résolution en Y de l'image lue (et de son masque)
 * \return 0 en cas de succès, -1 si la fin du fichier est atteinte, 1 en cas d'erreur
 */
int readFileLine(std::ifstream& file, char* imageFileName, bool* hasMask, char* maskFileName, BoundingBox<double>* bbox, int* width, int* height, double* resx, double* resy)
{
    std::string str;

    while (str.empty()) {
        if (file.eof()) {
            LOGGER_DEBUG("Fin du fichier de configuration atteinte");
            return -1;
        }
        std::getline(file,str);
    }
    
    int pos;
    int nb;

    char type[3];

    if ((nb = std::sscanf(str.c_str(),"%s %s %lf %lf %lf %lf %lf %lf",
        type, imageFileName, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, resx, resy)) == 8) {
        if (memcmp(type,"IMG",3)) {
            LOGGER_ERROR("We have to read an image information at first.");
            return 1;
        }
        // Arrondi a la valeur entiere la plus proche
        *width = lround((bbox->xmax - bbox->xmin)/(*resx));    
        *height = lround((bbox->ymax - bbox->ymin)/(*resy));

        pos = file.tellg();
    } else {
        LOGGER_ERROR("We have to read 8 values, we have " << nb);
        return 1;
    }

    str.clear();
    
    // Récupération d'un éventuel masque
    while (str.empty()) {
        if (file.eof()) {
            *hasMask = false;
            return -1;
        }
        std::getline(file,str);
    }
    
    if ((std::sscanf(str.c_str(),"%s %s", type, maskFileName)) != 2 || memcmp(type,"MSK",3)) {
        /* La ligne ne correspond pas au masque associé à l'image lue juste avant.
         * C'est en fait l'image suivante (ou une erreur). On doit donc remettre le
         * pointeur de manière à ce que cette image soit lue au prochain appel de
         * readFileLine.
         */
        *hasMask = false;
        file.seekg(pos);
    } else *hasMask = true;

    return 0;
}

/**
 * \~french
 * \brief Charge les images en entrée et en sortie depuis le fichier de configuration
 * \details On va récupérer toutes les informations de toutes les images et masques présents dans le fichier de configuration et créer les objets LibtiffImage correspondant. Toutes les images ici manipulées sont de vraies images (physiques) dans ce sens où elles sont des fichiers soit lus, soit qui seront écrits.
 * \param[in] imageListFilename chemin du fichier de configuration
 * \param[out] ppImageOut image résultante de l'outil
 * \param[out] ppMaskOut masque résultat de l'outil, si demandé
 * \param[out] pImageIn ensemble des images en entrée
 * \return 0 en cas de succès, -1 en cas d'erreur
 */
int loadImages(char* imageListFilename, LibtiffImage** ppImageOut, LibtiffImage** ppMaskOut,
               std::vector<LibtiffImage*>* pImageIn)
{
    char imageFileName[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    char maskFileName[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    BoundingBox<double> bbox(0.,0.,0.,0.);
    int width, height;
    bool hasMask;
    double resx, resy;
    libtiffImageFactory factory;

    // Ouverture du fichier texte listant les images
    std::ifstream file;

    file.open(imageListFilename);
    if (!file) {
        LOGGER_ERROR("Impossible d'ouvrir le fichier " << imageListFilename);
        return -1;
    }

    // Lecture et creation de l image de sortie
    if (readFileLine(file,imageFileName,&hasMask,maskFileName,&bbox,&width,&height,&resx,&resy)) {
        LOGGER_ERROR("Erreur lecture des premières lignes du fichier de parametres: " << imageListFilename);
        return -1;
    }

    *ppImageOut = factory.createLibtiffImage(imageFileName, bbox, width, height,resx, resy, samplesperpixel,
                                             bitspersample, sampleformat, photometric,COMPRESSION_NONE,16);

    if (*ppImageOut == NULL) {
        LOGGER_ERROR("Impossible de creer l'image " << imageFileName);
        return -1;
    }

    if (hasMask) {
        *ppMaskOut = factory.createLibtiffImage(maskFileName, bbox, width, height,resx, resy, 1, 8, 1,
                                                PHOTOMETRIC_MINISBLACK,COMPRESSION_PACKBITS,16);

        if (*ppMaskOut == NULL) {
            LOGGER_ERROR("Impossible de creer le masque " << maskFileName);
            return -1;
        }
    }

    int lig=3;

    // Lecture et creation des images source
    int nb=0;
    while ((nb = readFileLine(file,imageFileName,&hasMask,maskFileName,&bbox,&width,&height,&resx,&resy)) == 0) {
        
        LibtiffImage* pImage=factory.createLibtiffImage(imageFileName, bbox, width, height, resx, resy);
        if (pImage == NULL){
            LOGGER_ERROR("Impossible de creer une image a partir de " << imageFileName);
            return -1;
        }
        lig++;

        if (hasMask) {
            lig++;
            LibtiffImage* pMask=factory.createLibtiffImage(maskFileName, bbox, width, height, resx, resy);
            if (pMask == NULL) {
                LOGGER_ERROR("Impossible de creer un masque a partir de " << maskFileName);
                return -1;
            }
            
            if (! pImage->setMask(pMask)) {
                LOGGER_ERROR("Cannot add mask to the input LibTiffImage");
                return -1;
            }
        }
        
        pImageIn->push_back(pImage);
    }
    
    if (nb != -1) {
        LOGGER_ERROR("Erreur lecture du fichier de parametres: " << imageListFilename << " a la ligne " << lig);
        return -1;
    }

    // Fermeture du fichier
    file.close();

    return (pImageIn->size() - 1);
}

/**
 * \~french
 * \brief Contrôle la cohérence des images en entrée et celle en sortie
 * \details On vérifie que les résolutions fournies ne sont pas nulles et que le format des canaux est le même pour les images en entrée et pour celle en sortie
 * \param[in] pImageOut image résultante de l'outil
 * \param[in] ImageIn images en entrée
 * \return 0 en cas de succès, -1 en cas d'erreur
 * \todo Contrôler les éventuels masques
 */
int checkImages(LibtiffImage* pImageOut, std::vector<LibtiffImage*>& ImageIn)
{
    for (unsigned int i=0; i < ImageIn.size(); i++) {
        if (ImageIn.at(i)->getresx()*ImageIn.at(i)->getresy()==0.) {    
            LOGGER_ERROR("Source image " << i+1 << " is not valid (resolutions)");
            ImageIn.at(i)->print();
            return -1;
        }
        if (ImageIn.at(i)->channels != pImageOut->channels){
            LOGGER_ERROR(
                "Source image " << i+1 << " is not valid (samples per pixel have to be " << pImageOut->channels << ")");
            ImageIn.at(i)->print();
            return -1;
        }
    }
    if (pImageOut->getresx()*pImageOut->getresy()==0.){
        LOGGER_ERROR("Output image (" << pImageOut->getfilename() << ") is not valid (resolutions)");
        pImageOut->print();
        return -1;
    }
    if (pImageOut->getbitspersample()!=8 && pImageOut->getbitspersample()!=32){
        LOGGER_ERROR("Unvalid samples per pixels for the output image " << pImageOut->getfilename());
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
 * \param[in] ImageIn images en entrée
 * \param[out] pTabImageIn images en entrée, triées en paquets compatibles
 * \return 0 en cas de succès, -1 en cas d'erreur
 */
int sortImages(std::vector<LibtiffImage*> ImageIn, std::vector<std::vector<Image*> >* pTabImageIn)
{    
    std::vector<Image*> vTmpImg;
    std::vector<LibtiffImage*>::iterator itiniImg = ImageIn.begin();

    /* we create consistent images' vectors (X/Y resolution and X/Y phases)
     * Masks are moved in parallel with images
     */
    for (std::vector<LibtiffImage*>::iterator itImg = ImageIn.begin(); itImg < ImageIn.end()-1; itImg++) {
        
        if (! (*itImg)->isCompatibleWith(*(itImg+1))) {
            // two following images are not compatible, we split images' vector
            // images
            vTmpImg.assign(itiniImg,itImg+1);
            itiniImg = itImg+1;
            pTabImageIn->push_back(vTmpImg);
        }
    }
    
    // we don't forget to store last images in pTabImageIn
    // images
    vTmpImg.assign(itiniImg,ImageIn.end());
    pTabImageIn->push_back(vTmpImg);

    return 0;
}

/**
 * \~french
 * \brief Réechantillonne un paquet d'images compatibles
 * \details On crée l'objet ResampledImage correspondant au réechantillonnage du paquet d'images, afin de le rendre compatible avec l'image de sortie. On veut que l'emprise de l'image réechantillonnée ne dépasse ni de l'image de sortie, ni des images en entrée (sans prendre en compte les miroirs, données virtuelles).
 * \param[in] pImageOut image résultante de l'outil
 * \param[in] pECI paquet d'images compatibles, à réechantillonner
 * \param[in] realBbox réel rectangle englobant du paquet d'images, sans prendre en compte les miroirs ajoutés.
 * \return image réechantillonnée
 */
ResampledImage* resampleImages(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, BoundingBox<double> realBbox)
{
    double resx_src=pECI->getresx(), resy_src=pECI->getresy();
    // Extrêmes en comptant les miroirs
    double xmin_src=pECI->getxmin(), ymax_src=pECI->getymax();
    
    double resx_dst=pImageOut->getresx(), resy_dst=pImageOut->getresy();
    
    double ratio_x=resx_dst/resx_src, ratio_y=resy_dst/resy_src;

    /* L'image reechantillonnee est limitee a l'intersection entre l'image de sortie et les images sources
     * (sans compter les miroirs)
     */
    double xmin_dst=__max(realBbox.xmin,pImageOut->getxmin());
    double xmax_dst=__min(realBbox.xmax,pImageOut->getxmax());
    double ymin_dst=__max(realBbox.ymin,pImageOut->getymin());
    double ymax_dst=__min(realBbox.ymax,pImageOut->getymax());

    LOGGER_DEBUG("Image rééchantillonnée : bbox avant rephasage : "
        <<xmin_dst << "," << ymin_dst << "," << xmax_dst << "," << ymax_dst);

    /* Nous avons maintenant les limites de l'image réechantillonée. N'oublions pas que celle ci doit être compatible
     * avec l'image de sortie (c'est la seule raison d'être du réechantillonnage). Il faut donc modifier la bounding box
     * afin qu'elle remplisse les conditions de compatibilité (phases égales en x et en y).
     */

    double intpart;
    double phi = 0;
    double phaseDiff = 0;

    double phaseX = pImageOut->getPhasex();
    double phaseY = pImageOut->getPhasey();

    // Mise en phase de xmin (sans que celui ci puisse être plus petit)
    phi = modf(xmin_dst/resx_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}
    
    if (fabs(phi-phaseX) > 0.01 && fabs(phi-phaseX) < 0.99) {
        phaseDiff = phaseX - phi;
        if (phaseDiff < 0.) {phaseDiff += 1.0;}
        xmin_dst += phaseDiff*resx_dst;
    }

    // Mise en phase de xmax (sans que celui ci puisse être plus grand)
    phi = modf(xmax_dst/resx_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}

    if (fabs(phi-phaseX) > 0.01 && fabs(phi-phaseX) < 0.99) {
        phaseDiff = phaseX - phi;
        if (phaseDiff > 0.) {phaseDiff -= 1.0;}
        xmax_dst += phaseDiff*resx_dst;
    }

    // Mise en phase de ymin (sans que celui ci puisse être plus petit)
    phi = modf(ymin_dst/resy_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}
    
    if (fabs(phi-phaseY) > 0.01 && fabs(phi-phaseX) < 0.99) {
        phaseDiff = phaseY - phi;
        if (phaseDiff < 0.) {phaseDiff += 1.0;}
        ymin_dst += phaseDiff*resy_dst;
    }

    // Mise en phase de ymax (sans que celui ci puisse être plus grand)
    phi = modf(ymax_dst/resy_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}
    
    if (fabs(phi-phaseY) > 0.01 && fabs(phi-phaseX) < 0.99) {
        phaseDiff = phaseY - phi;
        if (phaseDiff > 0.) {phaseDiff -= 1.0;}
        ymax_dst += phaseDiff*resy_dst;
    }

    LOGGER_DEBUG("Image rééchantillonnée : bbox après rephasage : "
        << xmin_dst << "," << ymin_dst << "," << xmax_dst << "," << ymax_dst);
    
    // Dimension de l'image reechantillonnee
    int width_dst = int((xmax_dst-xmin_dst)/resx_dst+0.5);
    int height_dst = int((ymax_dst-ymin_dst)/resy_dst+0.5);

    double off_x=(xmin_dst-xmin_src)/resx_src,off_y=(ymax_src-ymax_dst)/resy_src;

    BoundingBox<double> bbox_dst(xmin_dst, ymin_dst, xmax_dst, ymax_dst);

    // On commence par réechantillonner le masque : TOUJOURS EN PPV, sans utilisation de masque pour l'interpolation
    ResampledImage* pRMask = new ResampledImage(pECI->Image::getMask(), width_dst, height_dst,resx_dst,resy_dst,
                                                off_x, off_y, ratio_x, ratio_y, false,
                                                Interpolation::NEAREST_NEIGHBOUR, bbox_dst);

    // Reechantillonnage
    ResampledImage* pRImage = new ResampledImage(pECI, width_dst, height_dst,resx_dst,resy_dst, off_x, off_y,
                                                 ratio_x, ratio_y, pECI->useMasks(), interpolation, bbox_dst);

    if (! pRImage->setMask(pRMask)) {
        LOGGER_ERROR("Cannot add mask to the Resampled Image");
        return NULL;
    }
    
    return pRImage;
}

/**
 * \~french \brief Ajoute des miroirs à un paquet d'images compatibles
 * \~english \brief Add mirrors to compatible images pack
 * \~ \image html miroirs.png
 * \~french \details On va vouloir réechantillonner ce paquet d'images, donc utiliser une interpolation. Une interpolation se fait sur nombre plus ou moins grand de pixels sources, selon le type. On veut que l'interpolation soit possible même sur les pixels du bord, et ce sans effet de bord. On ajoute donc des pixels virtuels, qui ne sont que le reflet des pixels de l'image. On crée ainsi 4 images miroirs (objets de la classe MirrorImage) par image du paquet (une à chaque bord). On sait distinguer les vraies images de celles virtuelles. On va également optimiser la taille des miroirs, et leur donner la taille juste suffisante pour l'interpolation.
 * \param[in] pECI paquet d'images compatibles, auquel on veut ajouter les miroirs
 * \param[in] mirrorSize taille en pixel des miroirs, dépendant du mode d'interpolation et du ratio des résolutions
 * \return paquet d'images contenant les miroirs
 */
ExtendedCompoundImage* addMirrors(ExtendedCompoundImage* pECI,int mirrorSize)
{
    mirrorImageFactory MIF;
    ExtendedCompoundImageFactory ECIF ;
    std::vector< Image*>  mirrorImages;

    uint i = 0;
    while (i<pECI->getImages()->size()) {
        for (int j=0; j<4; j++) {
            MirrorImage* mirrorImage = MIF.createMirrorImage(pECI->getImage(i), pECI->getSampleformat(),
                                                                   j, mirrorSize);
            if (mirrorImage == NULL){
                LOGGER_ERROR("Unable to calculate image's mirror");
                return NULL;
            }

            if (pECI->getMask(i)) {
                MirrorImage* mirrorMask = MIF.createMirrorImage(pECI->getMask(i), SAMPLEFORMAT_UINT, j, mirrorSize);
                if (mirrorMask == NULL){
                    LOGGER_ERROR("Unable to calculate mask's mirror");
                    return NULL;
                }
                if (! mirrorImage->setMask(mirrorMask)) {
                    LOGGER_ERROR("Unable to add mask to mirror");
                    return NULL;
                }
            }

            mirrorImages.push_back(mirrorImage);
        }
        i++;
    }

    mirrorImages.insert(mirrorImages.end(),pECI->getImages()->begin(),pECI->getImages()->end());

    ExtendedCompoundImage* pECI_mirrors = ECIF.createExtendedCompoundImage(mirrorImages,pECI->getNodata(),
                                                                           pECI->getSampleformat(), i*4);

    return pECI_mirrors;
}

/**
 * \~french \brief Traite chaque paquet d'images en entrée
 * \~english \brief Treat each input images pack
 * \~french \details On a préalablement trié les images par compatibilité. Pour chaque paquet, on va créer un objet de la classe ExtendedCompoundImage. Ce dernier est à considérer comme une image simple.
 * Cette image peut être :
 * \li superposable avec l'image de sortie. Elle est directement ajoutée à une liste d'image.
 * \li non superposable avec l'image de sortie. On va alors la réechantillonner, en utilisant la classe ResampledImage. C'est l'image réechantillonnée que l'on ajoute à la liste d'image.
 *
 * On obtient donc une liste d'images superposables avec celle de sortie, que l'on va réunir sous un objet de la classe ExtendedCompoundImage, qui sera la source unique utilisée pour écrire l'image de sortie.
 *
 * Les masques sont gérés en toile de fond, en étant attachés à chacune des images manipulées.
 * \param[in] pImageOut image de sortie
 * \param[in] TabImageIn paquets d'images en entrée
 * \param[out] ppECIout paquet d'images superposable avec l'image de sortie
 * \param[in] nodata valeur de non-donnée
 * \return 0 en cas de succès, -1 en cas d'erreur
 */
int mergeTabImages(LibtiffImage* pImageOut, // Sortie
                   std::vector<std::vector<Image*> >& TabImageIn, // Entrée
                   ExtendedCompoundImage** ppECIout, // Résultat du merge
                   int* nodata)
{
    ExtendedCompoundImageFactory ECIF ;
    std::vector<Image*> pOverlayedImages;

    // Valeurs utilisées pour déterminer la taille des miroirs en pixel (taille optimale en fonction du noyau utilisé)
    const Kernel& K = Kernel::getInstance(interpolation);
    double resx_dst = pImageOut->getresx();
    
    for (unsigned int i=0; i<TabImageIn.size(); i++) {
        // Mise en superposition du paquet d'images en 2 etapes
        
        // Etape 1 : Creation d'une image composite (avec potentiellement une seule image)

        ExtendedCompoundImage* pECI = ECIF.createExtendedCompoundImage(TabImageIn.at(i),nodata,sampleformat,0);
        if (pECI==NULL) {
            LOGGER_ERROR("Impossible d'assembler les images");
            return -1;
        }

        ExtendedCompoundMaskImage* pECMI = new ExtendedCompoundMaskImage(pECI);
        if (! pECI->setMask(pECMI)) {
            LOGGER_ERROR("Cannot add mask to the Image's pack " << i);
            return -1;
        }
        
        if (pImageOut->isCompatibleWith(pECI)){
            /* les images sources et finale ont la meme resolution et la meme phase
             * on aura donc pas besoin de reechantillonnage.*/
            pOverlayedImages.push_back(pECI);
            
        } else {
            // Etape 2 : Reechantillonnage de l'image composite necessaire

            // Ajout des miroirs
            int mirrorSize = ceil(K.size(resx_dst/pECI->getresx())) + 1;
            ExtendedCompoundImage* pECI_M = addMirrors(pECI,mirrorSize);
            if (pECI_M == NULL) {
                LOGGER_ERROR("Unable to add mirrors");
                return -1;
            }
            
            ExtendedCompoundMaskImage* pECMI_M = new ExtendedCompoundMaskImage(pECI_M);
            if (! pECI_M->setMask(pECMI_M)) {
                LOGGER_ERROR("Cannot add mask to the Image's pack with mirrors !");
                return -1;
            }

            ResampledImage* pResampledImage = resampleImages(pImageOut, pECI_M, pECI->getbbox());
            if (pResampledImage == NULL) {
                LOGGER_ERROR("Cannot resample Image's pack");
                return -1;
            }
            
            pOverlayedImages.push_back(pResampledImage);
            
        }

    }

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    *ppECIout = ECIF.createExtendedCompoundImage(
        pImageOut->width, pImageOut->height, pImageOut->channels, pImageOut->getbbox(),
        pOverlayedImages, nodata, sampleformat,0);
    
    if ( *ppECIout == NULL) {
        LOGGER_ERROR("Erreur lors de la fabrication de l image finale");
        return -1;
    }
    
    // Masque
    ExtendedCompoundMaskImage* pECMIout = new ExtendedCompoundMaskImage(*ppECIout);

    if (! (*ppECIout)->setMask(pECMIout)) {
        LOGGER_ERROR("Cannot add mask to the main Extended Compound Image");
        return -1;
    }

    return 0;
}

/*TEST : sortie des images (masques) temporaires
saveImage(pECI,"pECI.tif",3,8,1,PHOTOMETRIC_RGB,COMPRESSION_NONE);
saveImage(pECMI,"pECMI.tif",1,8,1,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
saveImage(pECI,"pECI_compat.tif",3,8,1,PHOTOMETRIC_RGB,COMPRESSION_NONE);
saveImage(mask,"pECI_compat_mask.tif",1,8,1,PHOTOMETRIC_MASK,COMPRESSION_NONE);
saveImage(pECI,"pECI_non_compat_withMirrors.tif",1,32,SAMPLEFORMAT_IEEEFP,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
saveImage(pECI,"pECI_non_compat_withMirrors.tif",3,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_RGB,COMPRESSION_NONE);
saveImage(pRImage,"pRImage.tif",3,8,1,PHOTOMETRIC_RGB,COMPRESSION_NONE);
saveImage(pResampledMask,"pRImage_mask.tif",1,8,1,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
saveImage(mask,"mask.tif",1,8,1,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
LOGGER_ERROR("On enregistre le paquet d'images " << i);
saveImage(pECI,"pECI.tif",3,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_RGB,COMPRESSION_NONE);
LOGGER_ERROR("On enregistre le masque du paquet d'images " << i);
saveImage(pECMI,"pECMI.tif",1,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
LOGGER_ERROR("On enregistre le paquet d'images avec miroirs");
saveImage(pECI_mirrors,"pECI_mirrors.tif",3,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_RGB,COMPRESSION_NONE);
LOGGER_ERROR("On enregistre le masque du paquet d'images avec miroirs");
saveImage(pECMI,"pECMI_mirrors.tif",1,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
LOGGER_ERROR("On enregistre le masque rééchantillonné");
saveImage(resampledMask,"pResampledMask.tif",1,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
*/

/**
 * \~french
 * \brief Fonction principale de l'outil mergeNtiff
 * \param[in] argc nombre de paramètres
 * \param[in] argv tableau des paramètres
 */
int main(int argc, char **argv) {

    LibtiffImage* pImageOut ;
    LibtiffImage* pMaskOut = NULL;
    std::vector<LibtiffImage*> ImageIn;
    std::vector<std::vector<Image*> > TabImageIn;
    ExtendedCompoundImage* pECI;

    /* Initialisation des Loggers */
    Logger::setOutput(STANDARD_OUTPUT_STREAM_FOR_ERRORS);

    Accumulator* acc = new StreamAccumulator();
    //Logger::setAccumulator(DEBUG, acc);
    Logger::setAccumulator(INFO , acc);
    Logger::setAccumulator(WARN , acc);
    Logger::setAccumulator(ERROR, acc);
    Logger::setAccumulator(FATAL, acc);

    std::ostream &logd = LOGGER(DEBUG);
    logd.precision(16);
    logd.setf(std::ios::fixed,std::ios::floatfield);

    std::ostream &logw = LOGGER(WARN);
    logw.precision(16);
    logw.setf(std::ios::fixed,std::ios::floatfield);
    
    LOGGER_DEBUG("Parse");
    // Lecture des parametres de la ligne de commande
    if (parseCommandLine(argc, argv) < 0){
        error("Echec lecture ligne de commande");
    }

    // Conversion string->int[] du paramètre nodata
    LOGGER_DEBUG("Nodata interpretation");
    int nodata[samplesperpixel];
    
    char* charValue = strtok(strnodata,",");
    if(charValue == NULL) {
        error("Error with option -n : a value for nodata is missing");
    }
    nodata[0] = atoi(charValue);
    for(int i = 1; i < samplesperpixel; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            error("Error with option -n : a value for nodata is missing");
        }
        nodata[i] = atoi(charValue);
    }

    LOGGER_DEBUG("Load");
    // Chargement des images
    if (loadImages(imageListFilename,&pImageOut,&pMaskOut,&ImageIn) < 0) {
        error("Echec chargement des images");
    }

    LOGGER_DEBUG("Check images");
    // Controle des images
    if (checkImages(pImageOut,ImageIn) < 0) {
        error("Echec controle des images");
    }
    
    LOGGER_DEBUG("Sort");
    // Tri des images
    if (sortImages(ImageIn, &TabImageIn) < 0) {
        error("Echec tri des images");
    }
    
    LOGGER_DEBUG("Merge");
    // Fusion des paquets d images
    if (mergeTabImages(pImageOut, TabImageIn, &pECI, nodata) < 0) {
        error("Echec fusion des paquets d images");
    }
    
    LOGGER_DEBUG("Save image");
    // Enregistrement de l'image fusionnée
    if (saveImage(pImageOut,pECI) < 0) {
        error("Echec enregistrement de l image finale");
    }

    LOGGER_DEBUG("Save mask");
    // Enregistrement du masque fusionné, si demandé
    if (pMaskOut != NULL && saveImage(pMaskOut,pECI->Image::getMask()) < 0) {
        error("Echec enregistrement du masque final");
    }
    
    LOGGER_DEBUG("Clean");
    // Nettoyage
    delete acc;
    delete pECI;
    delete pImageOut;
    delete pMaskOut;

    return 0;
}
