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

/*
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
	// Hypothese : ces images sont stockees en dernier
	// A partir du nombre total de miroirs, on peut donc determiner si une image est un miroir ou non
	uint mirrors;

	uint8_t nodata;
	uint16_t sampleformat;

	template<typename T>
	int _getline(T* buffer, int line);

protected:

	/** Constructeur
  	* Appelé via une fabrique de type extendedCompoundImageFactory
  	* Les Image sont detruites ensuite en meme temps que l'objet
  	* Il faut donc les creer au moyen de l operateur new et ne pas s'occuper de leur suppression
	 */
	ExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, uint8_t nodata, uint16_t sampleformat, uint mirrors) :
		Image(width, height, channels,bbox),
		images(images),
		nodata(nodata),
		sampleformat(sampleformat),
		mirrors(mirrors) {}

	ExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks, uint8_t nodata, uint16_t sampleformat, uint mirrors) :
                Image(width, height, channels,bbox),
		images(images),
                masks(masks),
		nodata(nodata),
		sampleformat(sampleformat),
		mirrors(mirrors) {}

public:
	std::vector<Image*>* getimages() {return &images;}
	uint getmirrors() {return mirrors;}

	/** Implementation de getline pour les uint8_t */
	int getline(uint8_t* buffer, int line);

	/** Implementation de getline pour les float */
	int getline(float* buffer, int line);

	/** Destructeur
      Suppression des images */
	virtual ~ExtendedCompoundImage() {
		for(uint i=0; i < images.size(); i++)
			delete images[i];
		for(uint i=0; i < masks.size(); i++)
                        delete masks[i];
	}

};

/*
* @class ExtendedCompoundImageFactory
* @brief Fabrique de extendedCompoundImageFactory
* @return Un pointeur sur l'ExtendedCompoundImage creee, NULL en cas d'echec
* La creation par une fabrique permet de proceder a certaines verifications
*/

#define epsilon 0.001

class extendedCompoundImageFactory {
public:
	ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, uint8_t nodata, uint16_t sampleformat, uint mirrors);

	ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks, uint8_t nodata, uint16_t sampleformat, uint mirrors);
};

/*
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
