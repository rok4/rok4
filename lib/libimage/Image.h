#ifndef IMAGE_H
#define IMAGE_H

#include <stdint.h>
#include "BoundingBox.h"

/**
 * @file Image.h
 * @brief Interface de base des classes Image.
 * @author IGN France - Geoportail
 */

/**
 * @class Image
 * @brief Interface de base des classes Image.
 * Les implémentations définiront des images avec différentes structures de pixel
 * (type et nombre de canaux). Cette interface basique permet de définir des
 * fonctions prenant en paramètre tout type d'image.
 * Images 1 bit non gerees
 */

class Image {
	public:
		/** Largeur de l'image en pixels */
		const int width;

		/** Hauteur de l'image en pixels */
		const int height;

		/** Nombre de canaux de l'image */
		const int channels;

	private:
		/** 
		 * BoundingBox de l'image dans son système de coordonnées. Celle-ci correspond 
		 * aux coordonnées du coin suppérieur gauche et du coin inférieur droit de l'image.
		 * Note : ces coordonnées sont aux coins des pixels et non au centre de ceux-ci
		 */
		BoundingBox<double> bbox;

		/** Resolution en x de l'image*/
		double resx;
		/** Resolution en y de l'image*/
		double resy;
		/** Calcul des resolutions en x et en y, calculées à partir des dimensions et de la BoudingBox **/
		void computeResxy() {

			resx=(bbox.xmax - bbox.xmin)/double(width);
			resy=(bbox.ymax - bbox.ymin)/double(height);
		}

	public:
		/** Affectation d'une bbox
		 * Necessite la mise a jour des emprises 
		 */
		inline void setbbox(BoundingBox<double> box) {bbox=box;computeResxy();}

		inline double getresx() const {return resx;}
		inline double getresy() const {return resy;}

		double inline getxmin() const {return bbox.xmin;}
		double inline getymax() const {return bbox.ymax;}
		double inline getxmax() const {return bbox.xmax;}
		double inline getymin() const {return bbox.ymin;}

		BoundingBox<double> inline getbbox() const {return bbox;}

		/** Fonctions de passage terrain <-> image */
		int inline x2c(double x) {return (int)((x-bbox.xmin)/resx+0.5);}
		int inline y2l(double y) {return (int)((bbox.ymax-y)/resy+0.5);}
		double inline c2x(int c) {return (bbox.xmin+c*resx);}
		double inline l2y(int l) {return (bbox.ymax-l*resy);}

		/** Constructeur */
		Image(int width, int height, int channels, BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.)) :
			width(width), height(height), channels(channels), bbox(bbox) {
				computeResxy();
			}
		/** 
		 * Retourne une ligne en entier 8 bits. 
		 * Les canaux sont entrelacés. Si les données ne sont pas intrinsèquement codées sur des entiers 8 bits
		 * une conversion est effectuée.
		 *
		 * @param buffer Tableau contenant au moins width*channels octets
		 * @param line Indice de la ligne à retourner (0 <= line < height)
		 *
		 * @return
		 *   - Nombre d'éléments effectivement copiés dans buffer en cas de succès
		 *   - 0 en cas d'échec
		 */
		virtual int getline(uint8_t *buffer, int line) = 0;

		/** 
		 * Retourne une ligne en float 32 bits.
		 * Les canaux sont entrelacés. Si les données ne sont pas intrinsèquement codées sur des flotants 32 bits
		 * une conversion est effectuée.
		 *
		 * @param buffer Tableau contenant au moins width*channels float
		 * @param line Indice de la ligne à retourner (0 <= line < height)
		 *
		 * @return 
		 *   - Nombre d'éléments effectivement copiés dans buffer en cas de succès
		 *   - 0 en cas d'échec
		 */
		virtual int getline(float *buffer, int line) = 0;

		/** 
		 * Destructeur virtuel. Permet de lancer les destructeurs des classes filles
		 * lors de la destruction d'un pointeur Image.
		 */
		virtual ~Image() {};
};


/**
 * Classe permettant de faire une copie d'une image. 
 * Cette copie pouvant être détruite sans que l'original le soit
 */
class ImageCopy : public Image {
	private:
		Image& image;
	public:
		ImageCopy(Image& image) : Image(image), image(image) {}
		int getline(uint8_t *buffer, int line) {return image.getline(buffer, line);}
		int getline(float *buffer, int line)   {return image.getline(buffer, line);}
};

#endif
