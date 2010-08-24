#ifndef EXTENTED_COMPOUND_IMAGE_H
#define EXTENTED_COMPOUND_IMAGE_H

#include "GeoreferencedImage.h"
#include <vector>
#include <cstring>
#include "Logger.h"
#include "math.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

class ExtendedCompoundImage : public GeoreferencedImage {

	friend class extendedCompoundImageFactory;

private:

	std::vector<GeoreferencedImage*> images;

	/**
	Remplissage iteratif d'une ligne
	Copie de la portion recouvrante de chaque ligne d'une image dans l'image finale
	retourne le nombre d'octets de la ligne
	*/

	template<typename T>
	int _getline(T* buffer, int line) {

		double y=l2y(line);
		for (uint i=0;i<images.size();i++){
			/* On écarte les images qui ne se trouvent pas sur la ligne*/
			if (images[i]->getymin()>=y||images[i]->getymax()<=y)
				continue;
			if (images[i]->getxmin()>=getxmax()||images[i]->getxmax()<=getxmin())
				continue;

			/* c0 est la première colonne de l'ExtendedCompoundImage remplie par l'image en cours.*/
			int c0=__max(0,x2c(images[i]->getxmin()));
			/* c1-1 est la dernière colonne de l' ExtendedCompoundImage remplie par l'image en cours.*/
			/* FIXME: NV: j'ai quand même un doute sur le fait que ce ne soit pas parfois c1*/
			int c1=__min(width,x2c(images[i]->getxmax()));
			T* buffer_t = new T[images[i]->width*images[i]->channels];
			images[i]->getline(buffer_t,images[i]->y2l(y));

			memcpy(&buffer[c0*channels],
				   &buffer_t[-(__min(0,x2c(images[i]->getxmin())))*channels],
				   (c1-c0)*channels*sizeof(T));

//			if (line==0 || line==1 || line==2000 || line==2010){
//				cout<<"image:"<<i<<endl;
//				cout<<"line"<<line<<endl;
//				cout<<"c0:"<<c0<<" c1:"<<c1<<endl;
//				cout<<"@ dans buffer (px):"<< c0<<endl;
//				cout<<"@ dans line   (px):"<< -__min(0,x2c(images[i]->getxmin())) <<endl;
//				cout<<"taille de la copie (px):"<< c1-c0 <<endl;
//				cout<<"---------------------------" <<endl;
//			}

			delete [] buffer_t;
		}
		return width*channels*sizeof(T);
	}

protected:

	/** Constructeur
  Appelé via une fabrique de type extendedCompoundImageFactory
  Les GeoreferencedImage sont detruites ensuite en meme temps que l'objet
  Il faut donc les creer au moyen de l operateur new et ne pas s'occuper de leur suppression
	 */
	ExtendedCompoundImage(int width, int height, int channels, double x0, double y0, double resx, double resy, std::vector<GeoreferencedImage*>& images) :
		GeoreferencedImage(width, height, channels,x0,y0,resx,resy),
		images(images) {}

public:

	/** D */
	int getline(uint8_t* buffer, int line) { return _getline(buffer, line); }

	/** D */
	int getline(float* buffer, int line) { return _getline(buffer, line); }

	/** Destructeur
      Suppression des images */
	virtual ~ExtendedCompoundImage() {
		for(uint i = 0; i < images.size(); i++)
			delete images[i];
	}

};

class extendedCompoundImageFactory {
public:
	ExtendedCompoundImage* createExtendedCompoundImage(int width, int height, int channels, double x0, double y0, double resx, double resy, std::vector<GeoreferencedImage*>& images)
	{
		uint i;
		double intpart;
		for (i=0;i<images.size()-1;i++)
		{
			if (images[i]->getresx()!=images[i+1]->getresx() || images[i]->getresy()!=images[i+1]->getresy() )
			{
				LOGGER_DEBUG("Les images ne sont pas toutes a la meme resolution");
				return NULL;				
			}
			if (modf(images[i]->getxmin()/images[i]->getresx(),&intpart)!=modf(images[i+1]->getxmin()/images[i+1]->getresx(),&intpart)
					|| modf(images[i]->getymax()/images[i]->getresy(),&intpart)!=modf(images[i+1]->getymax()/images[i+1]->getresy(),&intpart))
			{
				LOGGER_DEBUG("Les images ne sont pas toutes en phase");
				return NULL;
			}	
		}

		return new ExtendedCompoundImage(width,height,channels,x0,y0,resx,resy,images);
	}
};

#endif