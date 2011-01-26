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

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

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
	uint8_t nodata;

	/**
	@fn _getline(T* buffer, int line)
	@brief Remplissage iteratif d'une ligne
	Copie de la portion recouvrante de chaque ligne d'une image dans l'image finale
	@param le numero de la ligne
	@return le nombre d'octets de la ligne
	*/

	template<typename T>
	int _getline(T* buffer, int line) {
		unsigned int i;
		for (i=0;i<width*channels;i++)
			buffer[i]=(T)nodata;
		double y=l2y(line);
		for (i=0;i<images.size();i++){
                        // On ecarte les images qui ne se trouvent pas sur la ligne
                        if (images[i]->getymin()>=y||images[i]->getymax()<y)
                                continue;
                        if (images[i]->getxmin()>=getxmax()||images[i]->getxmax()<=getxmin())
                                continue;

                 	// c0 : indice de la 1ere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
                        int c0=__max(0,x2c(images[i]->getxmin()));
                        // c1-1 : indice de la derniere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
                        int c1=__min(width,x2c(images[i]->getxmax()));

			// c2 : indicde de la 1ere colonne de l'ExtendedCompoundImage dans l'image courante
			int c2=-(__min(0,x2c(images[i]->getxmin())));

                        T* buffer_t = new T[images[i]->width*images[i]->channels];
                        images[i]->getline(buffer_t,images[i]->y2l(y));
		
			if (masks.empty())
	                        memcpy(&buffer[c0*channels],&buffer_t[c2*channels],(c1-c0)*channels*sizeof(T));
			else{
				unsigned int j;
				uint8_t* buffer_m = new uint8_t[masks[i]->width];
				masks[i]->getline(buffer_m,masks[i]->y2l(y));
				for (j=0;j<c1-c0;j++)
					if (buffer_m[c2+j]==255)
                                           memcpy(&buffer[(c0+j)*channels],&buffer_t[c2*channels+j*channels],sizeof(T)*channels);
				delete buffer_m;
			}

                        delete [] buffer_t;
		}
		return width*channels*sizeof(T);
	}

protected:

	/** Constructeur
  	* Appelé via une fabrique de type extendedCompoundImageFactory
  	* Les Image sont detruites ensuite en meme temps que l'objet
  	* Il faut donc les creer au moyen de l operateur new et ne pas s'occuper de leur suppression
	 */
	ExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, uint8_t nodata) :
		Image(width, height, channels,bbox),
		images(images),
		nodata(nodata) {}

	ExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks, uint8_t nodata) :
                Image(width, height, channels,bbox),
		images(images),
                masks(masks),
		nodata(nodata) {}

public:
	std::vector<Image*>* getimages() {return &images;}

	/** Implementation de getline pour les uint8_t */
	int getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

	/** Implementation de getline pour les float */
	int getline(float* buffer, int line) { return _getline(buffer, line); }

	/** Destructeur
      Suppression des images */
	virtual ~ExtendedCompoundImage() {
		for(uint i = 0; i < images.size(); i++)
			delete images[i];
	}

};

/*
* @class ExtendedCompoundImageFactory
* @brief Fabrique de extendedCompoundImageFactory
* @return Un pointeur sur l'ExtendedCOmpoundImage creee, NULL en cas d'echec
* La creation par une fabrique permet de proceder a certaines verifications
*/

#define epsilon 0.001

class extendedCompoundImageFactory {
public:
	ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, uint8_t nodata)
	{
		uint i;
		double intpart, phasex0, phasey0, phasex1, phasey1;
		for (i=0;i<images.size()-1;i++)
		{
			if ( fabs(images[i]->getresx()-images[i+1]->getresx())>epsilon || fabs(images[i]->getresy()-images[i+1]->getresy())>epsilon )
			{
				LOGGER_DEBUG("Les images ne sont pas toutes a la meme resolution "<<images[i]->getresx()<<" "<<images[i+1]->getresx()<<" "<<images[i]->getresy()<<" "<<images[i+1]->getresy());
				return NULL;				
			}
			phasex0 = modf(images[i]->getxmin()/images[i]->getresx(),&intpart);
			phasex1 = modf(images[i+1]->getxmin()/images[i+1]->getresx(),&intpart);
			phasey0 = modf(images[i]->getymax()/images[i]->getresy(),&intpart);
			phasey0 = modf(images[i+1]->getymax()/images[i+1]->getresy(),&intpart);
			if ( (fabs(phasex1-phasex0)>epsilon && ( (fabs(phasex0)>epsilon && fabs(1-phasex0)>epsilon) || (fabs(phasex1)>epsilon && fabs(1-phasex1)>epsilon)))
			|| (fabs(phasey1-phasey0)>epsilon && ( (fabs(phasey0)>epsilon && fabs(1-phasey0)>epsilon) || (fabs(phasey1)>epsilon && fabs(1-phasey1)>epsilon))) )
			{
				LOGGER_DEBUG("Les images ne sont pas toutes en phase "<<phasex0<<" "<<phasex1<<" "<<phasey0<<" "<<phasey1);
				return NULL;
			}
		}

		return new ExtendedCompoundImage(width,height,channels,bbox,images,nodata);
	}

	ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images, std::vector<Image*>& masks,uint8_t nodata)
	{
		// TODO : controler que les images et les masques sont superposables a l'image
		return new ExtendedCompoundImage(width,height,channels,bbox,images,masks,nodata);
	}
};

/*
* @class ExtendedCompoundMaskImage
* @brief Masque d'une ExtendedCompoundImage
* 1 : si une image occupe un pixel, 0 sinon
*/

class ExtendedCompoundMaskImage : public Image {

private:
        ExtendedCompoundImage* ECI;

        /**
        @fn _getline(uint8_t* buffer, int line)
        @brief Remplissage iteratif d'une ligne
        @param le numero de la ligne
        @return le nombre d'octets de la ligne
        */

        int _getline(uint8_t* buffer, int line) {
                memset(buffer,0,width*channels);
                double y=l2y(line);
                for (uint i=0;i<ECI->getimages()->size();i++){
                        // On ecarte les images qui ne se trouvent pas sur la ligne
                        if (ECI->getimages()->at(i)->getymin()>=y||ECI->getimages()->at(i)->getymax()<y)
                                continue;
                        if (ECI->getimages()->at(i)->getxmin()>=getxmax()||ECI->getimages()->at(i)->getxmax()<=getxmin())
                                continue;

                        // c0 : indice de la 1ere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
                        int c0=__max(0,x2c(ECI->getimages()->at(i)->getxmin()));
                        // c1-1 : indice de la derniere colonne dans l'ExtendedCompoundImage de son intersection avec l'image courante
                        int c1=__min(width,x2c(ECI->getimages()->at(i)->getxmax()));

                        memset(&buffer[c0*channels],255,(c1-c0)*channels*sizeof(uint8_t));
                }
                return width*channels*sizeof(uint8_t);
        }

public:
	/** Constructeur */
        ExtendedCompoundMaskImage(ExtendedCompoundImage*& ECI) :
                Image(ECI->width, ECI->height, 1,ECI->getbbox()),
                ECI(ECI) {}

        /** Implementation de getline pour les uint8_t */
        int getline(uint8_t* buffer, int line) { return _getline(buffer, line); }
	/** Implementation de getline pour les float */
	int getline(float* buffer, int line) {
        	uint8_t* buffer_t = new uint8_t[width*channels];
        	getline(buffer_t,line);
        	convert(buffer,buffer_t,width*channels);
        	delete [] buffer_t;
        	return width*channels;
	}

        /** Destructeur
      Suppression des images */
        virtual ~ExtendedCompoundMaskImage() {}

};

#endif
