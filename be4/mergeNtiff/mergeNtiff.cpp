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
* 2. Une compression pour le fichier de sortie
* 3. Un mode d'interpolation
* 4. Une couleur de NoData
* 5. Un type d image (Data/Metadata)
* 6. Le nombre de canaux des images
* 7. Nombre d'octets par canal
* 8. La colorimetrie
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
char imageListFilename[256];
char strnodata[256];
uint16_t samplesperpixel, bitspersample, sampleformat, photometric, compression;
int type=-1;
Interpolation::KernelType interpolation;

/**
* @fn void usage()
* Usage de la ligne de commande
*/

void usage() {
    LOGGER_INFO("mergeNtiff version "<< BE4_VERSION);
    LOGGER_INFO(" Usage :  mergeNtiff -f [fichier liste des images source] -c [none/png/jpg/lzw/zip/pkb] -a [uint/float] -i [lanczos/nn/linear/bicubic] -n [couleur NoData] -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask] ");
    LOGGER_INFO(" Exemple : mergeNtiff -f configfile.txt -a float -i nn -n -99999 -t image -s 1 -b 32 -p gray ");
}

/**
* @fn parseCommandLine(int argc, char** argv, char* imageListFilename, Interpolation::KernelType& interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& sampleformat,  uint16_t& photometric)
* Lecture des parametres de la ligne de commande
*/

int parseCommandLine(int argc, char** argv) {
    
    if (argc != 19) {
        LOGGER_ERROR(" Nombre de parametres incorrect");
        usage();
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
                    else if(strncmp(argv[i], "nn",3) == 0) interpolation = Interpolation::NEAREST_NEIGHBOUR; // =0
                    else if(strncmp(argv[i], "bicubic",9) == 0) interpolation = Interpolation::CUBIC; // =2
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
                default: usage(); return -1;
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
* @fn int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric)
* @brief Enregistrement d'une image TIFF
* @param Image : Image a enregistrer
* @param pName : nom du fichier TIFF
* @param sampleperpixel : nombre de canaux de l'image TIFF
* @param bitspersample : nombre de bits par canal de l'image TIFF
* @param sampleformat : format des données binaires (uint ou float)
* @param photometric : valeur du tag TIFFTAG_PHOTOMETRIC de l'image TIFF
* @param compression : valeur de compression de l'image a enregistrer
* @param nodata : valeur du pixel representant la valeur NODATA (6 caractère hexadécimaux)
* TODO : gerer tous les types de couleur pour la valeur NODATA
* @return : 0 en cas de succes, -1 sinon
*/

int saveImage(Image *pImage, char* pName, int spp, uint16_t bps, uint16_t sf, uint16_t ph, uint16_t comp) {
    // Ouverture du fichier
    LOGGER_DEBUG("Sauvegarde de l'image" << pName);
    TIFF* output=TIFFOpen(pName,"w");
    if (output==NULL) {
        LOGGER_ERROR("Impossible d'ouvrir le fichier " << pName << " en ecriture");
        return -1;
    }

    // Ecriture de l'en-tete
    TIFFSetField(output, TIFFTAG_IMAGEWIDTH, pImage->width);
    TIFFSetField(output, TIFFTAG_IMAGELENGTH, pImage->height);
    TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, spp);
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
* @fn int readFileLine(std::ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height)
* Lecture d une ligne du fichier de la liste d images source et de la suivante si celle si contient le tag du masque
* Retourne -1 si la fin du fichier est atteinte
* Retourne 8 si une image et ses informations (dont masque) ont été bien lue
* Retourne un autre entier positif si une ligne est mal formattée.
*/

int readFileLine(std::ifstream& file, char* imageFileName, char* maskFileName, BoundingBox<double>* bbox, int* width, int* height, double* resx, double* resy)
{
    std::string str;

    if (file.eof()) {
        LOGGER_DEBUG("Fin du fichier de configuration atteinte");
        return -1;
    }
    
    std::getline(file,str);
    int nb;
    int pos;

    char type[3];

    if ( (nb=std::sscanf(str.c_str(),"%s %s %lf %lf %lf %lf %lf %lf",
        type, imageFileName, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, resx, resy)) == 8) {
        if (memcmp(type,"IMG",3)) {
            LOGGER_ERROR("We have to read an image information at first.");
            return 0;
        }
        // Arrondi a la valeur entiere la plus proche
        *width = lround((bbox->xmax - bbox->xmin)/(*resx));    
        *height = lround((bbox->ymax - bbox->ymin)/(*resy));

        pos = file.tellg();
    
        // Récupération d'un éventuel masque
        std::getline(file,str);
        if ((std::sscanf(str.c_str(),"%s %s", type, maskFileName)) != 2 || memcmp(type,"MSK",3)) {
            /* La ligne ne correspond pas au masque associé à l'image lue juste avant.
             * C'est en fait l'image suivante (ou une erreur. On doit donc remettre le
             * pointeur de manière à ce que cette image soit lue au prochain appel de
             * readFileLine
             */
            file.seekg(pos);
        }
        
    }

    return nb;
}

/**
* @fn int loadImages(char* imageListFilename, LibtiffImage** ppImageOut, std::vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
* Chargement des images depuis le fichier texte donné en parametre
*/

int loadImages(char* imageListFilename, LibtiffImage** ppImageOut, LibtiffImage** ppMaskOut,
               std::vector<LibtiffImage*>* pImageIn)
{
    char imageFileName[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    char maskFileName[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    BoundingBox<double> bbox(0.,0.,0.,0.);
    int width, height;
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
    if (readFileLine(file,imageFileName,maskFileName,&bbox,&width,&height,&resx,&resy) != 8) {
        LOGGER_ERROR("Erreur lecture des premières lignes du fichier de parametres: " << imageListFilename);
        return -1;
    }

    *ppImageOut = factory.createLibtiffImage(imageFileName, bbox, width, height,resx, resy, samplesperpixel, bitspersample, photometric,COMPRESSION_NONE,16);

    if (*ppImageOut == NULL) {
        LOGGER_ERROR("Impossible de creer l'image " << imageFileName);
        return -1;
    }

    *ppMaskOut = factory.createLibtiffImage(maskFileName, bbox, width, height,resx, resy, 1, 8, PHOTOMETRIC_MASK,COMPRESSION_NONE,16);

    if (*ppMaskOut==NULL) {
        LOGGER_ERROR("Impossible de creer le masque " << maskFileName);
        return -1;
    }

    int lig=3;

    // Lecture et creation des images source
    int nb=0;
    while ((nb=readFileLine(file,imageFileName,maskFileName,&bbox,&width,&height,&resx,&resy)) == 8) {
        LibtiffImage* pImage=factory.createLibtiffImage(imageFileName, bbox, resx, resy);
        if (pImage==NULL){
            LOGGER_ERROR("Impossible de creer une image a partir de " << imageFileName);
            return -1;
        }
        lig++;

        if (maskFileName != NULL) {
            lig++;
            LibtiffImage* pMask=factory.createLibtiffImage(maskFileName, bbox, resx, resy);
            if (pMask==NULL) {
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
* @fn int checkImages(LibtiffImage* pImageOut, std::vector<LibtiffImage*>& ImageIn)
* @brief Controle des images
* TODO : ajouter des controles
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
* @brief Tri des images source en paquets d images superposables (memes phases et resolutions en x et y)
* @param ImageIn : vecteur contenant les images non triees
* @param pTabImageIn : tableau de vecteurs conteant chacun des images superposables
* @return 0 en cas de succes, -1 sinon
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
* @fn ExtendedCompoundImage* compoundImages(std::vector< Image*> & TabImageIn,char* nodata, uint16_t sampleformat, uint mirrors) 
* @brief Assemblage d images superposables
* @param TabImageIn : vecteur d images a assembler
* @return Image composee de type ExtendedCompoundImage
*/

ExtendedCompoundImage* compoundImages(std::vector< Image*> & TabImageIn, int* nodata)
{
    if (TabImageIn.empty()) {
        LOGGER_ERROR("Assemblage d'un tableau d'images de taille nulle");
        return NULL;
    }
    
    ExtendedCompoundImageFactory ECIF ;
    ExtendedCompoundImage* pECI = ECIF.createExtendedCompoundImage(TabImageIn,nodata,sampleformat);

    return pECI ;
}

/** 
* @fn int addMirrors(ExtendedCompoundImage* pECI,int mirrorSize)
* @brief Ajout de miroirs a une ExtendedCompoundImage
* On ajoute à chaque image de l'ECI 4 images au bord (un buffer miroir d'une largeur égale à celle nécessaire
* à l'interpolation
* Objectif : mettre des miroirs la ou il n'y a pas d'images afin d'eviter des effets de bord en cas de reechantillonnage
* @param pECI : l'image à completer
* @return : le nombre de miroirs ajoutes
*/
/*
ExtendedCompoundImage* addMirrors(ExtendedCompoundImage* pECI,int mirrorSize)
{
    mirrorImageFactory MIFactory;
    std::vector< Image*>  mirrorImages;

    uint i = 0;
    while (i<pECI->getimages()->size()) {
        for (int j=0; j<4; j++) {
            MirrorImage* mirrorImage = MIFactory.createMirrorImage(pECI->getImage(i),
                                                                   pECI->getSampleformat(),j,mirrorSize);
            if (mirrorImage == NULL){
                LOGGER_ERROR("Unable to calculate mirrors' image");
                return NULL;
            }

            MirrorImage* mirrorMask = MIFactory.createMirrorImage(pECI->getMask(i),
                                                                  SAMPLEFORMAT_UINT,j,mirrorSize);
            if (mirrorMask == NULL){
                LOGGER_ERROR("Unable to calculate mirrors' mask");
                return NULL;
            }

            mirrorImage->setMask(mirrorMask);
            
            mirrorImages.push_back(mirrorImage);
        }
        i++;
    }

    mirrorImages.insert(mirrorImages.end(),pECI->getimages()->begin(),pECI->getimages()->end());

    ExtendedCompoundImage* pECI_mirrors = compoundImages(mirrorImages,pECI->getNodata(), i*4);

    return pECI_mirrors;
}
*/

/**
* @fn ResampledImage* resampleImages(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, BoundingBox<double> bbox_src, Interpolation::KernelType& interpolation, ExtendedCompoundMaskImage* mask, ResampledImage*& resampledMask)
* @brief Reechantillonnage d'une image de type ExtendedCompoundImage
* @brief Objectif : la rendre superposable a l'image finale
* @return Image reechantillonnee legerement plus petite
*/

ResampledImage* resampleImages(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI)
{
    double resx_src=pECI->getresx(), resy_src=pECI->getresy();
    // Extrêmes en comptant les miroirs
    double xmin_src=pECI->getxmin(), ymax_src=pECI->getymax();
    
    double resx_dst=pImageOut->getresx(), resy_dst=pImageOut->getresy();
    
    double ratio_x=resx_dst/resx_src, ratio_y=resy_dst/resy_src;

    /* L'image reechantillonnee est limitee a l'intersection entre l'image de sortie et les images sources
     * (sans compter les miroirs)
     */
    double xmin_dst=__max(pECI->getbbox().xmin,pImageOut->getxmin());
    double xmax_dst=__min(pECI->getbbox().xmax,pImageOut->getxmax());
    double ymin_dst=__max(pECI->getbbox().ymin,pImageOut->getymin());
    double ymax_dst=__min(pECI->getbbox().ymax,pImageOut->getymax());

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
    phaseDiff += phaseX - phi;
    if (phaseDiff < 0.) {phaseDiff += 1.0;}
    xmin_dst += phaseDiff;

    // Mise en phase de xmax (sans que celui ci puisse être plus grand)
    phi = modf(xmax_dst/resx_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}
    phaseDiff += phaseX - phi;
    if (phaseDiff > 0.) {phaseDiff -= 1.0;}
    xmax_dst += phaseDiff;

    // Mise en phase de ymin (sans que celui ci puisse être plus petit)
    phi = modf(ymin_dst/resy_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}
    phaseDiff += phaseY - phi;
    if (phaseDiff < 0.) {phaseDiff += 1.0;}
    ymin_dst += phaseDiff;

    // Mise en phase de ymax (sans que celui ci puisse être plus grand)
    phi = modf(xmax_dst/resy_dst, &intpart);
    if (phi < 0.) {phi += 1.0;}
    phaseDiff += phaseY - phi;
    if (phaseDiff > 0.) {phaseDiff -= 1.0;}
    xmax_dst += phaseDiff;
    
    // Dimension de l'image reechantillonnee
    int width_dst = int((xmax_dst-xmin_dst)/resx_dst+0.5);
    int height_dst = int((ymax_dst-ymin_dst)/resy_dst+0.5);

    double off_x=(xmin_dst-xmin_src)/resx_src,off_y=(ymax_src-ymax_dst)/resy_src;

    BoundingBox<double> bbox_dst(xmin_dst, ymin_dst, xmax_dst, ymax_dst);

    // On commence par réechantillonner le masque : TOUJOURS EN PPV
    ResampledImage* resampledMask = new ResampledImage(pECI->Image::getMask(), width_dst, height_dst,resx_dst,resy_dst, off_x, off_y, ratio_x, ratio_y, Interpolation::NEAREST_NEIGHBOUR, bbox_dst);

    LOGGER_ERROR("On enregistre le masque rééchantillonné"); /*TEST*/
    resampledMask->print(); /*TEST*/
    saveImage(resampledMask,"resampledMask.tif",1,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE);
    
    // Reechantillonnage
    ResampledImage* pRImage = new ResampledImage(pECI, width_dst, height_dst,resx_dst,resy_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst);

    if (! pRImage->setMask(resampledMask)) {
        LOGGER_ERROR("Cannot add mask to the Resampled Image");
        return NULL;
    }

    LOGGER_ERROR("On enregistre l'image rééchantillonnée"); /*TEST*/
    pRImage->print(); /*TEST*/
    saveImage(pRImage,"pResampledImage.tif",3,8,SAMPLEFORMAT_UINT,PHOTOMETRIC_RGB,COMPRESSION_NONE);

    return pRImage;
}

/**
* @fn int mergeTabImages(LibtiffImage* pImageOut, std::vector<std::vector<Image*> >& TabImageIn, ExtendedCompoundImage** ppECImage, Interpolation::KernelType& interpolation, char* nodata, uint16_t sampleformat)
* @brief Fusion des images
* @param pImageOut : image de sortie
* @param TabImageIn : tableau de vecteur d images superposables
* @param ppECImage : image composite creee
* @param interpolation : type d'interpolation utilise
* @return 0 en cas de succes, -1 sinon
*/

int mergeTabImages(LibtiffImage* pImageOut, // Sortie
                   std::vector<std::vector<Image*> >& TabImageIn, // Entrée
                   ExtendedCompoundImage** ppECIout, // Résultat du merge
                   int* nodata)
{
    ExtendedCompoundImageFactory ECIF ;
    std::vector<Image*> pOverlayedImages;

    // Valeurs utilisées pour déterminer la taille des miroirs en pixel (taille optimale en fonction du noyau utilisé)
    /*const Kernel& K = Kernel::getInstance(interpolation);
    double resx_dst = pImageOut->getresx();*/
    
    for (unsigned int i=0; i<TabImageIn.size(); i++) {
        // Mise en superposition du paquet d'images en 2 etapes

        // Etape 1 : Creation d'une image composite (avec potentiellement une seule image)
        ExtendedCompoundImage* pECI = compoundImages(TabImageIn.at(i),nodata);
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
            
            /*// Ajout des miroirs
            int mirrorSize = ceil(K.size(resx_dst/pECI->getresx())) + 1;
            ExtendedCompoundImage* pECI_mirrors = addMirrors(pECI,mirrorSize);
            if (pECI_mirrors == NULL) {
                LOGGER_ERROR("Unable to add mirrors");
                return -1;
            }*/

            ResampledImage* pResampledImage = resampleImages(pImageOut, pECI);
            if (pResampledImage==NULL) {
                LOGGER_ERROR("Impossible de reechantillonner les images");
                return -1;
            }
            
            pOverlayedImages.push_back(pResampledImage);
            
        }

    }

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    *ppECIout = ECIF.createExtendedCompoundImage(
        pImageOut->width, pImageOut->height, pImageOut->channels, pImageOut->getbbox(),
        pOverlayedImages, nodata, sampleformat);
    
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
* @fn int main(int argc, char **argv)
* @brief Fonction principale
*/

int main(int argc, char **argv) {

    LibtiffImage* pImageOut ;
    LibtiffImage* pMaskOut ;
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
        LOGGER_ERROR("Echec lecture ligne de commande");
        sleep(1);
        return -1;
    }

    // Conversion string->int[] du paramètre nodata
    LOGGER_DEBUG("Nodata interpretation");
    int nodata[samplesperpixel];
    
    char* charValue = strtok(strnodata,",");
    if(charValue == NULL) {
        LOGGER_ERROR("Error with option -n : a value for nodata is missing");
        return -1;
    }
    nodata[0] = atoi(charValue);
    for(int i = 1; i < samplesperpixel; i++) {
        charValue = strtok (NULL, ",");
        if(charValue == NULL) {
            LOGGER_ERROR("Error with option -n : a value for nodata is missing");
            return -1;
        }
        nodata[i] = atoi(charValue);
    }

    LOGGER_DEBUG("Load");
    // Chargement des images
    if (loadImages(imageListFilename,&pImageOut,&pMaskOut,&ImageIn) < 0) {
        LOGGER_ERROR("Echec chargement des images"); 
        sleep(1);
        return -1;
    }

    LOGGER_DEBUG("Check images");
    // Controle des images
    if (checkImages(pImageOut,ImageIn) < 0) {
        LOGGER_ERROR("Echec controle des images");
        sleep(1);
        return -1;
    }
    
    LOGGER_DEBUG("Sort");
    // Tri des images
    if (sortImages(ImageIn, &TabImageIn) < 0) {
        LOGGER_ERROR("Echec tri des images");
        sleep(1);
        return -1;
    }
    
    LOGGER_DEBUG("Merge");
    // Fusion des paquets d images
    if (mergeTabImages(pImageOut, TabImageIn, &pECI, nodata) < 0) {
        LOGGER_ERROR("Echec fusion des paquets d images");
        sleep(1);
        return -1;
    }
    
    LOGGER_DEBUG("Save image");
    // Enregistrement de l'image fusionnée
    if (saveImage(pECI,pImageOut->getfilename(),pImageOut->channels,
        bitspersample, sampleformat, photometric, compression) < 0) {
        LOGGER_ERROR("Echec enregistrement de l image finale");
        sleep(1);
        return -1;
    }

    LOGGER_DEBUG("Save mask");
    // Enregistrement du masque fusionné
    if (saveImage(pECI->Image::getMask(),pMaskOut->getfilename(),1,
        8, SAMPLEFORMAT_UINT,PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE) < 0) {
        LOGGER_ERROR("Echec enregistrement du masque final");
        sleep(1);
        return -1;
    }
    
    LOGGER_DEBUG("Clean");
    // Nettoyage
    delete acc;
    delete pECI;
    delete pImageOut;
    delete pMaskOut;

    return 0;
}
