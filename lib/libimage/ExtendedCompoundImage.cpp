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

#include "ExtendedCompoundImage.h"
#include "Logger.h"
#include "Utils.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

/**
@fn _getline(T* buffer, int line)
@brief Remplissage iteratif d'une ligne
Copie de la portion recouvrante de chaque ligne d'une image dans l'image finale
@param line le numero de la ligne
@return le nombre d'octets de la ligne
*/

template <typename T>
int ExtendedCompoundImage::_getline(T* buffer, int line) {
    int i;

    for (i=0;i<width*channels;i++) {buffer[i]=(T)nodata;}
    
    double y=l2y(line);
    
    for (i=0;i<(int)images.size();i++){
        // On ecarte les images qui ne se trouvent pas sur la ligne
        // On evite de comparer des coordonnees terrain (comparaison de flottants)
        // Les coordonnees image sont obtenues en arrondissant au pixel le plus proche

        if (y2l(images[i]->getymin()) <= line || y2l(images[i]->getymax()) > line) {continue;}
        if (images[i]->getxmin() >= getxmax() || images[i]->getxmax() <= getxmin()) {continue;}

        // c0 : indice de la 1ere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
        int c0=__max(0,x2c(images[i]->getxmin()));
        // c1-1 : indice de la derniere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
        int c1=__min(width,x2c(images[i]->getxmax()));

        // c2 : indice de de la 1ere colonne de l'ExtendedCompoundImage dans l'image courante
        int c2=-(__min(0,x2c(images[i]->getxmin())));

        T* buffer_t = new T[images[i]->width*images[i]->channels];
        LOGGER_DEBUG(i<<" "<<line<<" "<<images[i]->y2l(y));
        images[i]->getline(buffer_t,images[i]->y2l(y));
        if (masks.empty()) {
            memcpy(&buffer[c0*channels],&buffer_t[c2*channels],(c1-c0)*channels*sizeof(T));
        } else {
            
            int j;
            uint8_t* buffer_m = new uint8_t[masks[i]->width];

            masks[i]->getline(buffer_m,masks[i]->y2l(y));
            for (j=0;j<c1-c0;j++) {
                if (buffer_m[c2+j]>=127) {  // Seuillage subjectif du masque
                    memcpy(&buffer[(c0+j)*channels],&buffer_t[c2*channels+j*channels],sizeof(T)*channels);
                }
            }
            
            delete buffer_m;
        }
        delete [] buffer_t;
    }
    return width*channels*sizeof(T);
}

/** Implementation de getline pour les uint8_t */
int ExtendedCompoundImage::getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

/** Implementation de getline pour les float */
int ExtendedCompoundImage::getline(float* buffer, int line)
{
    if (sampleformat==1){     //uint8_t
        uint8_t* buffer_t = new uint8_t[width*channels];
            getline(buffer_t,line);
            convert(buffer,buffer_t,width*channels);
            delete [] buffer_t;
            return width*channels;
    }
    else            //float
        return _getline(buffer, line);
}

#define epsilon 0.001

ExtendedCompoundImage* extendedCompoundImageFactory::createExtendedCompoundImage(int width, 
                                                                                 int height, 
                                                                                 int channels, 
                                                                                 BoundingBox<double> bbox, 
                                                                                 std::vector<Image*>& images, 
                                                                                 int nodata, 
                                                                                 uint16_t sampleformat, 
                                                                                 uint mirrors)
{
    uint i;

    if (images.size()==0){
        LOGGER_ERROR("Creation d'une image composite sans image");
        return NULL;
    }

    for (i=0;i<images.size()-1;i++)
    {  
        if (! images[i]->isCompatibleWith(images[i+1])) {
            LOGGER_ERROR("Les images ne sont pas toutes compatibles : resX,resY phaseX,phaseY\n" <<
                "- image " << i << " : " << images[i]->getresx() << "," << images[i]->getresy() << " " << images[i]->getPhasex() << " " << images[i]->getPhasey() << "\n" <<
                "- image " << i+1 << " : " << images[i+1]->getresx() << "," << images[i+1]->getresy() << " " << images[i+1]->getPhasex() << " " << images[i+1]->getPhasey() << "\n"
                
            );
            return NULL;
        }
    }

    return new ExtendedCompoundImage(width,height,channels,bbox,images,nodata,sampleformat,mirrors);
}

ExtendedCompoundImage* extendedCompoundImageFactory::createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks, int nodata, uint16_t sampleformat, uint mirrors)
{
    // TODO : controler que les images et les masques sont superposables a l'image
    return new ExtendedCompoundImage(width,height,channels,bbox,images,masks,nodata,sampleformat,mirrors);
}

/**
@fn _getline(uint8_t* buffer, int line)
@brief Remplissage iteratif d'une ligne
@param le numero de la ligne
@return le nombre d'octets de la ligne
*/

int ExtendedCompoundMaskImage::_getline(uint8_t* buffer, int line) {
  memset(buffer,0,width*channels);
  // Rappel de l'hypothese : les miroirs sont ranges en dernier parmi les images de l ECI
  for (uint i=0; i < ECI->getimages()->size()-ECI->getmirrors(); i++){

    // On ecarte les images qui ne se trouvent pas sur la ligne
    // On evite de comparer des coordonnees terrain (comparaison de flottants)
    // Les coordonnees image sont obtenues en arrondissant au pixel le plus proche
    if (y2l(ECI->getimages()->at(i)->getymin()) <= line || y2l(ECI->getimages()->at(i)->getymax()) > line){
      continue;
    }
    if (ECI->getimages()->at(i)->getxmin() >= getxmax() || ECI->getimages()->at(i)->getxmax() <= getxmin()){
      continue;
    }

    // c0 : indice de la 1ere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
    int c0=__max(0,x2c(ECI->getimages()->at(i)->getxmin()));
    // c1-1 : indice de la derniere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
    int c1=__min(width,x2c(ECI->getimages()->at(i)->getxmax()));

    memset(&buffer[c0*channels],255,(c1-c0)*channels*sizeof(uint8_t));
  }
  return width*channels*sizeof(uint8_t);
}

/** Implementation de getline pour les uint8_t */
int ExtendedCompoundMaskImage::getline(uint8_t* buffer, int line) { return _getline(buffer, line); }
/** Implementation de getline pour les float */
int ExtendedCompoundMaskImage::getline(float* buffer, int line) {
    uint8_t* buffer_t = new uint8_t[width*channels];
    getline(buffer_t,line);
    convert(buffer,buffer_t,width*channels);
    delete [] buffer_t;
    return width*channels;
}       
