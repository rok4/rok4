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

#ifndef EXTENTED_COMPOUND_IMAGE_H
#define EXTENTED_COMPOUND_IMAGE_H

/**
* @file ExtendedCompoundImage.h
* @brief Image composee a partir de n autres images
* @author IGN France - Geoportail
*/

#include "Image.h"
#include <vector>
#include <cstring>
#include "Logger.h"
#include "math.h"
#include "Utils.h"

/**
* @class ExtendedCompoundImage
* @brief Image compose de n autres images
* Ces images doivent etre superposables, c'est-a-dire remplir les conditions suivantes :
* Elles doivent toutes avoir la meme resolution en x
* Elles doivent toutes avoir la meme resolution en y
* Elles doivent toutes etres en phase en x et en y (ne pas avoir les pixels d'une image decales par rapport aux pixels d'une autre image)
*/
#include <iostream>
class ExtendedCompoundImage : public Image {

    friend class extendedCompoundImageFactory;

private:

    std::vector<Image*> images;
    std::vector<Image*> masks;

    // Certaines images peuvent etre des miroirs (MirrorImage)
    // Ces images ne doivent pas etre prises en compte dans la fonction getline d'une ExtendedCompoundMaskImage
    // Hypothese : ces images sont stockees en premier
    // A partir du nombre total de miroirs, on peut donc determiner si une image est un miroir ou non
    uint mirrors;

    int nodata;
    
    /* Il existe des données source qui contiennent du nodata, de valeur nodata_src. Lors de la superposition, on ne veut pas
     * le garder. Ce filtre augmantant le temps du mergeNtiff, on veut que ce soit en option :
     *      - true : on retire le blanc
     *      - false : comme avant
     */
    bool nowhite;
    
    uint16_t sampleformat;

    template<typename T>
    int _getline(T* buffer, int line);

    template<typename T>
    bool isNodata(T* pixel);

protected:

    /** Constructeur
      * Appelé via une fabrique de type extendedCompoundImageFactory
      * Les Image sont detruites ensuite en meme temps que l'objet
      * Il faut donc les creer au moyen de l operateur new et ne pas s'occuper de leur suppression
     */
    ExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, int nodata, uint16_t sampleformat, uint mirrors, bool nowhite) :
        Image(width, height, channels,bbox),
        images(images),
        nodata(nodata),
        sampleformat(sampleformat),
        mirrors(mirrors),
        nowhite(nowhite) {}

    ExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks, int nodata, uint16_t sampleformat, uint mirrors, bool nowhite) :
        Image(width, height, channels,bbox),
        images(images),
        masks(masks),
        nodata(nodata),
        sampleformat(sampleformat),
        mirrors(mirrors),
        nowhite(nowhite) {}

public:
    std::vector<Image*>* getimages() {return &images;}
    uint getmirrors() {return mirrors;}
    uint16_t getSampleformat() {return sampleformat;}

    /** Implementation de getline pour les uint8_t */
    int getline(uint8_t* buffer, int line);

    /** Implementation de getline pour les float */
    int getline(float* buffer, int line);

    /** Destructeur
      Suppression des images */
    virtual ~ExtendedCompoundImage() {
        for(uint i=0; i < images.size(); i++) {delete images[i];}
        for(uint i=0; i < masks.size(); i++) {delete masks[i];}
    }

};

/**
* @class ExtendedCompoundImageFactory
* @brief Fabrique de extendedCompoundImageFactory
* @return Un pointeur sur l'ExtendedCompoundImage creee, NULL en cas d'echec
* La creation par une fabrique permet de proceder a certaines verifications
*/

#define epsilon 0.001

class extendedCompoundImageFactory {
public:
    ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, int nodata, uint16_t sampleformat, uint mirrors, bool nowhite);

    ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks, int nodata, uint16_t sampleformat, uint mirrors, bool nowhite);
};

/**
* @class ExtendedCompoundMaskImage
* @brief Masque d'une ExtendedCompoundImage
* 255 : si une image occupe un pixel, 0 sinon
*/

class ExtendedCompoundMaskImage : public Image {

private:
        ExtendedCompoundImage* ECI;
        int _getline(uint8_t* buffer, int line);

public:
    /** Constructeur */
    ExtendedCompoundMaskImage(ExtendedCompoundImage*& ECI) :
        Image(ECI->width, ECI->height, 1,ECI->getbbox()),
        ECI(ECI) {}

    /** Implementation de getline pour les uint8_t */
    int getline(uint8_t* buffer, int line); 
    /** Implementation de getline pour les float */
    int getline(float* buffer, int line);

    /** Destructeur
    Suppression des images */
    virtual ~ExtendedCompoundMaskImage() {}

};

#endif
