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
#include "Interpolation.h"
#include "math.h"
#include "../be4version.h"

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
    LOGGER_INFO("mergeNtiff version "<< BE4_VERSION);
    LOGGER_INFO(" Usage :  mergeNtiff -f [fichier liste des images source] -a [uint/float] -i [lanczos/nn/linear/bicubic] -n [couleur NoData] -t [img/mtd] -s [1/3] -b [8/32] -p[min_is_black/rgb/mask] ");
    LOGGER_INFO(" Exemple : mergeNtiff -f configfile.txt -a float -i nn -n -99999 -t image -s 1 -b 32 -p gray ");
}

/**
* @fn parseCommandLine(int argc, char** argv, char* imageListFilename, Interpolation::KernelType& interpolation, char* nodata, int& type, uint16_t& sampleperpixel, uint16_t& bitspersample, uint16_t& sampleformat,  uint16_t& photometric)
* Lecture des parametres de la ligne de commande
*/

int parseCommandLine(int argc, char** argv, char* imageListFilename, Interpolation::KernelType& interpolation, char* strnodata, bool& nowhite, int& type, uint16_t& samplesperpixel, uint16_t& bitspersample, uint16_t& sampleformat,  uint16_t& photometric) {
    
    if (argc != 17 && argc != 18) {
        LOGGER_ERROR(" Nombre de parametres incorrect");
        usage();
        return -1;
    }

    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i],"-nowhite")) {
            nowhite = true;
            continue;
        }
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
                    else if(strncmp(argv[i], "mtd",3) == 0) type = 0 ;
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
* @param nodata : valeur du pixel representant la valeur NODATA (6 caractère hexadécimaux)
* TODO : gerer tous les types de couleur pour la valeur NODATA
* @return : 0 en cas de succes, -1 sinon
*/

int saveImage(Image *pImage, char* pName, int sampleperpixel, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric) {
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
        TIFFSetField(output, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel);
        TIFFSetField(output, TIFFTAG_BITSPERSAMPLE, bitspersample);
        TIFFSetField(output, TIFFTAG_SAMPLEFORMAT, sampleformat);
        TIFFSetField(output, TIFFTAG_PHOTOMETRIC, photometric);
        TIFFSetField(output, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        TIFFSetField(output, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
        TIFFSetField(output, TIFFTAG_ROWSPERSTRIP, 1);
        TIFFSetField(output, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE);

        // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if (sampleformat==SAMPLEFORMAT_UINT){
        buf_u = (unsigned char*)_TIFFmalloc(pImage->width * pImage->channels * bitspersample / 8);
        for( int line = 0; line < pImage->height; line++) {
                        pImage->getline(buf_u,line);
                        TIFFWriteScanline(output, buf_u, line, 0);
        }
    }
    else if(sampleformat==SAMPLEFORMAT_IEEEFP){
        buf_f = (float*)_TIFFmalloc(pImage->width*pImage->channels*bitspersample/8);
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
* Lecture d une ligne du fichier de la liste d images source
*/

int readFileLine(std::ifstream& file, char* filename, BoundingBox<double>* bbox, int* width, int* height)
{
    std::string str;
    std::getline(file,str);
    double resx, resy;
    int nb;

    if ( (nb=sscanf(str.c_str(),"%s %lf %lf %lf %lf %lf %lf",filename, &bbox->xmin, &bbox->ymax, &bbox->xmax, &bbox->ymin, &resx, &resy)) ==7) {
        // Arrondi a la valeur entiere la plus proche
        *width = lround((bbox->xmax - bbox->xmin)/resx);    
        *height = lround((bbox->ymax - bbox->ymin)/resy);
    }

    return nb;
}

/**
* @fn int loadImages(char* imageListFilename, LibtiffImage** ppImageOut, std::vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
* Chargement des images depuis le fichier texte donné en parametre
*/

int loadImages(char* imageListFilename, LibtiffImage** ppImageOut, std::vector<Image*>* pImageIn, int sampleperpixel, uint16_t bitspersample, uint16_t photometric)
{
    char filename[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    BoundingBox<double> bbox(0.,0.,0.,0.);
    int width, height;
    libtiffImageFactory factory;

    // Ouverture du fichier texte listant les images
    std::ifstream file;

    file.open(imageListFilename);
    if (!file) {
        LOGGER_ERROR("Impossible d'ouvrir le fichier " << imageListFilename);
        return -1;
    }

    // Lecture et creation de l image de sortie
    if (readFileLine(file,filename,&bbox,&width,&height)<0){
        LOGGER_ERROR("Erreur lecture du fichier de parametres: " << imageListFilename << " a la ligne 0");
        return -1;
    }

    *ppImageOut=factory.createLibtiffImage(filename, bbox, width, height, sampleperpixel, bitspersample, photometric,COMPRESSION_NONE,16);

    if (*ppImageOut==NULL) {
        LOGGER_ERROR("Impossible de creer " << filename);
        return -1;
    }

    // Lecture et creation des images source
    int nb=0,i;
    while ((nb=readFileLine(file,filename,&bbox,&width,&height))==7){
        LibtiffImage* pImage=factory.createLibtiffImage(filename, bbox);
        if (pImage==NULL){
            LOGGER_ERROR("Impossible de creer une image a partir de " << filename);
            return -1;
        }
        pImageIn->push_back(pImage);
        i++;
    }
    if (nb>=0 && nb!=7){
        LOGGER_ERROR("Erreur lecture du fichier de parametres: " << imageListFilename << " a la ligne " << i);
        return -1;
    }

    // Fermeture du fichier
    file.close();

    return (pImageIn->size() - 1);
}


/**
* @fn int checkImages(LibtiffImage* pImageOut, std::vector<Image*>& ImageIn)
* @brief Controle des images
* TODO : ajouter des controles
*/

int checkImages(LibtiffImage* pImageOut, std::vector<Image*>& ImageIn)
{
    for (unsigned int i=0;i<ImageIn.size();i++) {
        if (ImageIn.at(i)->getresx()*ImageIn.at(i)->getresy()==0.) {    
            LOGGER_ERROR("Resolution de l image source " << i+1 << " sur " << ImageIn.size() << " egale a 0");
                    return -1;
        }
        if (ImageIn.at(i)->channels!=pImageOut->channels){
            LOGGER_ERROR("Nombre de canaux de l image source " << i+1 << " sur " << ImageIn.size() << " differente de l image de sortie");
                        return -1;
        }
    }
    if (pImageOut->getresx()*pImageOut->getresy()==0.){
        LOGGER_ERROR("Resolution de l image de sortie egale a 0 " << pImageOut->getfilename());
        return -1;
    }
    if (pImageOut->getbitspersample()!=8 && pImageOut->getbitspersample()!=32){
        LOGGER_ERROR("Nombre de bits par sample de l image de sortie " << pImageOut->getfilename() << " non gere");
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

int sortImages(std::vector<Image*> ImageIn, std::vector<std::vector<Image*> >* pTabImageIn)
{
    std::vector<Image*> vTmp;
    std::vector<Image*>::iterator itini = ImageIn.begin();

    // we create consistent images' vectors (X/Y resolution and X/Y phases)

    for (std::vector<Image*>::iterator it = ImageIn.begin(); it < ImageIn.end()-1;it++) {
        if (! (*it)->isCompatibleWith(*(it+1))) {
            // two following images are not compatible, we split images' vector
            vTmp.assign(itini,it+1);
            itini = it+1;
            pTabImageIn->push_back(vTmp);
        }
    }
    
    // we don't forget to store last images in pTabImageIn
    vTmp.assign(itini,ImageIn.end());
    pTabImageIn->push_back(vTmp);

    return 0;
}

/**
* @fn ExtendedCompoundImage* compoundImages(std::vector< Image*> & TabImageIn,char* nodata, uint16_t sampleformat, uint mirrors) 
* @brief Assemblage d images superposables
* @param TabImageIn : vecteur d images a assembler
* @return Image composee de type ExtendedCompoundImage
*/

ExtendedCompoundImage* compoundImages(std::vector< Image*> & TabImageIn,int* nodata, bool nowhite, uint16_t sampleformat, uint mirrors)
{
    if (TabImageIn.empty()) {
        LOGGER_ERROR("Assemblage d'un tableau d images de taille nulle");
        return NULL;
    }

    // Rectangle englobant des images d entree
    double xmin=1E12, ymin=1E12, xmax=-1E12, ymax=-1E12 ;
    for (unsigned int j=0;j<TabImageIn.size();j++) {
        if (TabImageIn.at(j)->getxmin()<xmin)  xmin=TabImageIn.at(j)->getxmin();
        if (TabImageIn.at(j)->getymin()<ymin)  ymin=TabImageIn.at(j)->getymin();
        if (TabImageIn.at(j)->getxmax()>xmax)  xmax=TabImageIn.at(j)->getxmax();
        if (TabImageIn.at(j)->getymax()>ymax)  ymax=TabImageIn.at(j)->getymax();
    }

    extendedCompoundImageFactory ECImgfactory ;
    int w=(int)((xmax-xmin)/(*TabImageIn.begin())->getresx()+0.5), h=(int)((ymax-ymin)/(*TabImageIn.begin())->getresy()+0.5);
    ExtendedCompoundImage* pECI = ECImgfactory.createExtendedCompoundImage(w,h,(*TabImageIn.begin())->channels, BoundingBox<double>(xmin,ymin,xmax,ymax), TabImageIn,nodata,sampleformat,mirrors,nowhite);

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

int addMirrors(ExtendedCompoundImage* pECI,int mirrorSize)
{
    uint mirrors=0;

    mirrorImageFactory MIFactory;
    
    int nbImagesSrc = pECI->getimages()->size();
    
    int i = 0;
    while (i<pECI->getimages()->size()) {
        for (int j=0; j<4; j++) {
            MirrorImage* mirror=MIFactory.createMirrorImage(pECI->getimages()->at(i),j,mirrorSize);
            if (mirror == NULL){
                LOGGER_ERROR("Unable to calculate mirrors");
                return -1;
            }
            //pECI->getimages()->push_back(mirror);
            pECI->getimages()->insert(pECI->getimages()->begin()+mirrors,mirror);
            mirrors++;
            i++;
        }
        i++;
    }

    return mirrors;
}


/**
* @fn ResampledImage* resampleImages(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Interpolation::KernelType& interpolation, ExtendedCompoundMaskImage* mask, ResampledImage*& resampledMask)
* @brief Reechantillonnage d'une image de type ExtendedCompoundImage
* @brief Objectif : la rendre superposable a l'image finale
* @return Image reechantillonnee legerement plus petite
*/

ResampledImage* resampleImages(LibtiffImage* pImageOut, ExtendedCompoundImage* pECI, Interpolation::KernelType& interpolation, ExtendedCompoundMaskImage* mask, ResampledImage*& resampledMask)
{
    const Kernel& K = Kernel::getInstance(interpolation);

    double xmin_src=pECI->getxmin(), ymin_src=pECI->getymin(), xmax_src=pECI->getxmax(), ymax_src=pECI->getymax();
    double resx_src=pECI->getresx(), resy_src=pECI->getresy(), resx_dst=pImageOut->getresx(), resy_dst=pImageOut->getresy();
    double ratio_x=resx_dst/resx_src, ratio_y=resy_dst/resy_src;

    // L'image reechantillonnee est limitee a l'image de sortie
    double xmin_dst=__max(xmin_src+K.size(ratio_x)*resx_src,pImageOut->getxmin());
    double xmax_dst=__min(xmax_src-K.size(ratio_x)*resx_src,pImageOut->getxmax());
    double ymin_dst=__max(ymin_src+K.size(ratio_y)*resy_src,pImageOut->getymin());
    double ymax_dst=__min(ymax_src-K.size(ratio_y)*resy_src,pImageOut->getymax());
    
    double ymaxdst_save = ymax_dst;
    
    // Coordonnees de l'image reechantillonnee en pixels
    xmin_dst/=resx_dst;
    //xmin_dst=floor(xmin_dst+0.1);
    
    ymin_dst/=resy_dst;
    //ymin_dst=floor(ymin_dst+0.1);
        
    xmax_dst/=resx_dst;
    //xmax_dst=ceil(xmax_dst-0.1);
        
    ymax_dst/=resy_dst;
    //ymax_dst=ceil(ymax_dst-0.1);
    
    // Dimension de l'image reechantillonnee
    int width_dst = int(xmax_dst-xmin_dst+0.1);
    int height_dst = int(ymax_dst-ymin_dst+0.1);
    xmin_dst*=resx_dst;
    xmax_dst*=resx_dst;
    ymin_dst*=resy_dst;
    ymax_dst*=resy_dst;
    
    /* Suite au arrondis, il se peut que l'image de destination finisse par dépasser les images source.
     * Cela va logiquement générer une erreur (impossible de trouver de la donnée source.
     * Pour éviter cela, et uniquement dans le cas où on déborde, on va rétrécir l'image de destination.
     * On ne peut pas arrondir systématiquement vers une réduction de l'image car cela pourrait engendrer
     * de lignes noires dans les images résultantes.
     * Cela dit, l'ajout des miroirs suffirait à éviter ce manque de données.
     */
    if (ymax_dst > ymax_src) {
        ymax_dst -= resy_dst;
        height_dst -= 1;
    }    
    if (ymin_dst < ymin_src) {
        ymin_dst += resy_dst;
        height_dst -= 1;
    }    
    if (xmax_dst > xmax_src) {
        xmax_dst -= resx_dst;
        width_dst -= 1;
    }    
    if (xmin_dst < xmin_src) {
        xmin_dst += resx_dst;
        width_dst -= 1;
    } 

    double off_x=(xmin_dst-xmin_src)/resx_src,off_y=(ymax_src-ymax_dst)/resy_src;

    BoundingBox<double> bbox_dst(xmin_dst, ymin_dst, xmax_dst, ymax_dst);
    // Reechantillonnage
    ResampledImage* pRImage = new ResampledImage(pECI, width_dst, height_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst);
    
    //saveImage(pRImage,"test1.tif",3,8,1,PHOTOMETRIC_RGB);

    // Reechantillonage du masque
    resampledMask = new ResampledImage( mask, width_dst, height_dst, off_x, off_y, ratio_x, ratio_y, interpolation, bbox_dst);
    
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

int mergeTabImages(LibtiffImage* pImageOut, std::vector<std::vector<Image*> >& TabImageIn, ExtendedCompoundImage** ppECImage, Interpolation::KernelType& interpolation, int* nodata, bool nowhite, uint16_t sampleformat)
{
    extendedCompoundImageFactory ECImgfactory ;
    std::vector<Image*> pOverlayedImage;
    std::vector<Image*> pMask;
    
    const Kernel& K = Kernel::getInstance(interpolation);

    double resx_dst=pImageOut->getresx();
    
    for (unsigned int i=0; i<TabImageIn.size(); i++) {
        // Mise en superposition du paquet d'images en 2 etapes

        // Etape 1 : Creation d'une image composite
        ExtendedCompoundImage* pECI = compoundImages(TabImageIn.at(i),nodata,nowhite,sampleformat,0);
        ExtendedCompoundMaskImage* mask;// = new ExtendedCompoundMaskImage(pECI);

        if (pECI==NULL) {
            LOGGER_ERROR("Impossible d'assembler les images");
            return -1;
        }

        //saveImage(pECI,"test0.tif",3,8,1,PHOTOMETRIC_RGB); /*TEST*/

        if (pImageOut->isCompatibleWith(pECI)){
            /* les images sources et finale ont la meme res et la meme phase
             * on aura donc pas besoin de reechantillonnage.*/
            pOverlayedImage.push_back(pECI);
            //saveImage(pECI,"pECI_compat.tif",3,8,1,PHOTOMETRIC_RGB); /*TEST*/
            mask = new ExtendedCompoundMaskImage(pECI);
            //saveImage(mask,"pECI_compat_mask.tif",1,8,1,PHOTOMETRIC_MASK); /*TEST*/
            pMask.push_back(mask);
        } else {
            // Etape 2 : Reechantillonnage de l'image composite si necessaire
            int mirrorSize = ceil(K.size(resx_dst/pECI->getresx())) + 1;
            int mirrors=addMirrors(pECI,mirrorSize);
            if (mirrors < 0){
                LOGGER_ERROR("Unable to add mirrors");
                return -1;
            }

            ExtendedCompoundImage* pECI_withMirrors=compoundImages((*pECI->getimages()),nodata,nowhite,sampleformat,mirrors);
            
            //saveImage(pECI,"pECI_non_compat.tif",3,8,1,PHOTOMETRIC_RGB); /*TEST*/
            //saveImage(pECI_withMirrors,"pECI_non_compat_withMirrors.tif",3,8,1,PHOTOMETRIC_RGB); /*TEST*/

            mask = new ExtendedCompoundMaskImage(pECI_withMirrors);

            ResampledImage* pResampledMask;
            
            ResampledImage* pRImage = resampleImages(pImageOut, pECI_withMirrors, interpolation, mask, pResampledMask);
            if (pRImage==NULL) {
                LOGGER_ERROR("Impossible de reechantillonner les images");
                return -1;
            }
            pOverlayedImage.push_back(pRImage);
            pMask.push_back(pResampledMask);
            //saveImage(pRImage,"pRImage.tif",3,8,1,PHOTOMETRIC_RGB); /*TEST*/
            //saveImage(pResampledMask,"pRImage_mask.tif",1,8,1,PHOTOMETRIC_MASK);
            //saveImage(mask,"mask.tif",1,8,1,PHOTOMETRIC_MINISBLACK);
            //saveImage(pResampledMask,"pResampledMask.tif",1,8,1,PHOTOMETRIC_MINISBLACK);
        }
    }

    // Assemblage des paquets et decoupage aux dimensions de l image de sortie
    if ( (*ppECImage = ECImgfactory.createExtendedCompoundImage(pImageOut->width, pImageOut->height,
        pImageOut->channels, pImageOut->getbbox(), pOverlayedImage,pMask,nodata,sampleformat,0,nowhite))==NULL) {
        LOGGER_ERROR("Erreur lors de la fabrication de l image finale");
        return -1;
    }

    return 0;
}

/**
* @fn int main(int argc, char **argv)
* @brief Fonction principale
*/

int main(int argc, char **argv) {
    char imageListFilename[256];
    char strnodata[256];
    bool nowhite = false;
    uint16_t samplesperpixel, bitspersample, sampleformat, photometric;
    int type=-1;
    Interpolation::KernelType interpolation;

    LibtiffImage* pImageOut ;
    std::vector<Image*> ImageIn;
    std::vector<std::vector<Image*> > TabImageIn;
    ExtendedCompoundImage* pECImage;

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
    if (parseCommandLine(argc,argv,imageListFilename,interpolation,strnodata,nowhite,type,
        samplesperpixel,bitspersample,sampleformat,photometric)<0 ){
        LOGGER_ERROR("Echec lecture ligne de commande");
        sleep(1);
        return -1;
    }
    
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

    // TODO : gérer le type mtd !!
    if (type==0) {
        LOGGER_ERROR("Le type mtd n'est pas pris en compte");
        sleep(1);
        return -1;
    }

    LOGGER_DEBUG("Load");
    // Chargement des images
    if (loadImages(imageListFilename,&pImageOut,&ImageIn,samplesperpixel,bitspersample,photometric)<0){
        LOGGER_ERROR("Echec chargement des images"); 
        sleep(1);
        return -1;
    }

    LOGGER_DEBUG("Check");
    // Controle des images
    if (checkImages(pImageOut,ImageIn)<0){
        LOGGER_ERROR("Echec controle des images");
        sleep(1);
        return -1;
     }
    LOGGER_DEBUG("Sort");
    // Tri des images
    if (sortImages(ImageIn, &TabImageIn)<0){
        LOGGER_ERROR("Echec tri des images");
        sleep(1);
        return -1;
    }
    LOGGER_DEBUG("Merge");
    // Fusion des paquets d images
    if (mergeTabImages(pImageOut, TabImageIn, &pECImage, interpolation,nodata,nowhite,sampleformat) < 0){
        LOGGER_ERROR("Echec fusion des paquets d images");
        sleep(1);
        return -1;
    }
    LOGGER_DEBUG("Save");
    // Enregistrement de l image fusionnee
    if (saveImage(pECImage,pImageOut->getfilename(),pImageOut->channels,bitspersample,sampleformat,photometric)<0){
        LOGGER_ERROR("Echec enregistrement de l image finale");
        sleep(1);
        return -1;
    }

    // Nettoyage
    delete pImageOut;
    delete pECImage;

    return 0;
}
