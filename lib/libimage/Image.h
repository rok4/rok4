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
 * \file Image.h
 * \~french
 * \brief Définition de la classe abstraite Image, abstrayant les différents types d'images.
 * \~english
 * \brief Define the Image abstract class , to abstract all kind of images.
 */

#ifndef IMAGE_H
#define IMAGE_H

#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

#include <stdint.h>
#include <string.h>
#include <typeinfo>
#include "BoundingBox.h"
#include "math.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Interface de manipulation d'images
 * \details Cette classe abstraite permet d'utiliser toutes les images de la même manière, en assurant les définitions des fonctions permettant de lire une ligne, connaître l'emprise géographique de l'image... Toutes ces informations que l'on veut connaître sur une image, qu'elle soit directement liée à un fichier, ou le réechantillonnage d'un ensemble d'images.
 *
 * On permet l'association d'un masque de donnée, qui n'est autre qu'une image à un canal sur 8 bits qui permet la distinction entre un pixel de l'image qui contient de la vraie donnée de celui qui contient du nodata.
 *
 * Ne sont gérés que les formats suivant pour les canaux :
 * \li flottant sur 32 bits
 * \li entier non signé sur 8 bits
 *
 * Le géoréférencement est assuré par le renseignement des résolutions et du rectangle englobant. Cependant, on peut également gérer des images simples. Dans ce cas, on mettra par convention une bbox à 0,0,0,0 et des résolutions à -1. Aucun test ne sera fait par les fonctions qui utilisent ces attributs. On doit donc bien faire attention à rester cohérent.
 */
class Image {
public:
    /**
     * \~french \brief Largeur de l'image en pixel
     * \~english \brief Image's width, in pixel
     */
    const int width;

    /**
     * \~french \brief Hauteur de l'image en pixel
     * \~english \brief Image's height, in pixel
     */
    const int height;

    /**
     * \~french \brief Nombre de canaux de l'image
     * \~english \brief Number of samples per pixel
     */
    const int channels;

private:
    /**
     * \~french \brief Masque de donnée associé à l'image, facultatif
     * \~english \brief Mask associated to the image, optionnal
     */
    Image* mask;

    /**
     * \~french \brief Emprise rectangulaire au sol de l'image
     * \~english \brief Image's bounding box
     */
    BoundingBox<double> bbox;

    /**
     * \~french \brief Resolution de l'image, dans le sens des X
     * \~english \brief Image's resolution, X wise
     */
    double resx;
    /**
     * \~french \brief Resolution de l'image, dans le sens des Y
     * \~english \brief Image's resolution, Y wise
     */
    double resy;

    /**
     * \~french
     * \brief Calcul des resolutions en x et en y, calculées à partir des dimensions et de la bouding box
     * \~english
     * \brief Resolutions calculation, from pixel size and bounding box
     */
    void computeResolutions() {
        resx= ( bbox.xmax - bbox.xmin ) /double ( width );
        resy= ( bbox.ymax - bbox.ymin ) /double ( height );
    }


public:

    /**
     * \~french
     * \brief Définit l'emprise rectangulaire de l'image et calcule les résolutions
     * \param[in] box Emprise rectangulaire de l'image
     * \~english
     * \brief Define the image's bounding box and calculate resolutions
     * \param[in] box Image's bounding box
     */
    inline void setBbox ( BoundingBox<double> box ) {
        bbox = box;
        computeResolutions();
    }

    /**
     * \~french
     * \brief Retourne l'emprise rectangulaire de l'image
     * \return emprise
     * \~english
     * \brief Return the image's bounding box
     * \return bounding box
     */
    BoundingBox<double> inline getBbox() const {
        return bbox;
    }

    /**
     * \~french
     * \brief Retourne le xmin de l'emprise rectangulaire
     * \return xmin
     * \~english
     * \brief Return bounding box's xmin
     * \return xmin
     */
    double inline getXmin() const {
        return bbox.xmin;
    }
    /**
     * \~french
     * \brief Retourne le ymax de l'emprise rectangulaire
     * \return ymax
     * \~english
     * \brief Return bounding box's ymax
     * \return ymax
     */
    double inline getYmax() const {
        return bbox.ymax;
    }
    /**
     * \~french
     * \brief Retourne le xmax de l'emprise rectangulaire
     * \return xmax
     * \~english
     * \brief Return bounding box's xmax
     * \return xmax
     */
    double inline getXmax() const {
        return bbox.xmax;
    }
    /**
     * \~french
     * \brief Retourne le ymin de l'emprise rectangulaire
     * \return ymin
     * \~english
     * \brief Return bounding box's ymin
     * \return ymin
     */
    double inline getYmin() const {
        return bbox.ymin;
    }

    /**
     * \~french
     * \brief Retourne la résolution dans le sens des X
     * \return résolution en X
     * \~english
     * \brief Return the X wise resolution
     * \return X resolution
     */
    inline double getResX() const {
        return resx;
    }
    /**
     * \~french
     * \brief Retourne la résolution dans le sens des Y
     * \return résolution en Y
     * \~english
     * \brief Return the Y wise resolution
     * \return Y resolution
     */
    inline double getResY() const {
        return resy;
    }

    /**
     * \~french
     * \brief Retourne le masque de donnée associé à l'image
     * \return masque
     * \~english
     * \brief Return the associated mask
     * \return mask
     */
    Image* getMask() {
        return mask;
    }

    /**
     * \~french
     * \brief Définit le masque de donnée et contrôle la cohérence avec l'image
     * \param[in] newMask Masque de donnée
     * \~english
     * \brief Defined data mask and check consistency
     * \param[in] newMask Masque de donnée
     */
    bool setMask ( Image* newMask ) {
        if ( newMask->width != width || newMask->height != height || newMask->channels != 1 ) {
            LOGGER_ERROR ( "Unvalid mask" );
            LOGGER_ERROR ( "\t - channels have to be 1, it is " << newMask->channels );
            LOGGER_ERROR ( "\t - width have to be " << width << ", it is " << newMask->width );
            LOGGER_ERROR ( "\t - height have to be " << height << ", it is " << newMask->height );
            return false;
        }
        mask = newMask;
        return true;
    }

    /**
     * \~french
     * \brief Conversion de l'abscisse terrain vers l'indice de colonne dans l'image
     * \param[in] x abscisse terrain
     * \return colonne
     * \~english
     * \brief Conversion from terrain coordinate X to image column indice
     * \param[in] x terrain coordinate X
     * \return column
     */
    int inline x2c ( double x ) {
        return lround ( ( x-bbox.xmin ) /resx );
    }
    /**
     * \~french
     * \brief Conversion de l'ordonnée terrain vers l'indice de ligne dans l'image
     * \param[in] y ordonnée terrain
     * \return ligne
     * \~english
     * \brief Conversion from terrain coordinate Y to image line indice
     * \param[in] y terrain coordinate Y
     * \return line
     */
    int inline y2l ( double y ) {
        return lround ( ( bbox.ymax-y ) /resy );
    }

    /**
     * \~french
     * \brief Conversion de l'indice de colonne dans l'image vers l'abscisse terrain
     * \param[in] c colonne
     * \return abscisse terrain
     * \~english
     * \brief Conversion from image column indice to terrain coordinate X
     * \param[in] c column
     * \return terrain coordinate X
     */
    double inline c2x ( int c ) {
        return ( bbox.xmin+c*resx );
    }
    /**
     * \~french
     * \brief Conversion de l'indice de ligne dans l'image vers l'ordonnée terrain
     * \param[in] l ligne
     * \return ordonnée terrain
     * \~english
     * \brief Conversion from image line indice to terrain coordinate X
     * \param[in] l line
     * \return terrain coordinate Y
     */
    double inline l2y ( int l ) {
        return ( bbox.ymax-l*resy );
    }

    /**
     * \~french
     * \brief Calcul de la phase dans le sens des X
     * \details La phase en X est le décalage entre le bord gauche du pixel et le 0 des abscisses, évalué en pixel. On a donc un nombre décimal appartenant à [0,1[.
     * \image html phases.png
     * \return phase X
     * \~english
     * \brief Phasis calculation, X wise
     * \image html phases.png
     * \return X phasis
     */
    double inline getPhaseX() {
        double intpart;
        double phi=modf ( bbox.xmin/resx, &intpart );
        if ( phi < 0. ) {
            phi += 1.0;
        }
        return phi;
    }

    /**
     * \~french
     * \brief Calcul de la phase dans le sens des Y
     * \details La phase en Y est le décalage entre le bord haut du pixel et le 0 des ordonnées, évalué en pixel. On a donc un nombre décimal appartenant à [0,1[.
     * \return phase Y
     * \~english
     * \brief Phasis calculation, Y wise
     * \return Y phasis
     */
    double inline getPhaseY() {
        double intpart;
        double phi=modf ( bbox.ymax/resy, &intpart );
        if ( phi < 0. ) {
            phi += 1.0;
        }
        return phi;
    }

    /**
     * \~french
     * \brief Détermine la compatibilité avec une autre image, en comparant phases et résolutions
     * \details On parle d'images compatibles lorsqu'elles ont :
     * \li la même résolution en X
     * \li la même résolution en Y
     * \li la même phase en X
     * \li la même phase en Y
     *
     * Les tests d'égalité acceptent un epsilon qui est le suivant :
     * \li 1 pour mille de la résolution la plus petite pour les résolutions
     * \li 1% pour les phases
     *
     * \param[in] pImage image à comparer
     * \return compatibilité
     * \~english
     * \brief Determine compatibility with another image, comparing phasis and resolutions
     * \param[in] pImage image to compare
     * \return compatibility
     */
    bool isCompatibleWith ( Image* pImage ) {
        double epsilon_x=__min ( getResX(), pImage->getResX() ) /1000.;
        double epsilon_y=__min ( getResY(), pImage->getResY() ) /1000.;

        if ( fabs ( getResX()-pImage->getResX() ) > epsilon_x ) {
            return false;
        }
        if ( fabs ( getResY()-pImage->getResY() ) > epsilon_y ) {
            return false;
        }

        if ( fabs ( getPhaseX()-pImage->getPhaseX() ) > 0.01 && fabs ( getPhaseX()-pImage->getPhaseX() ) < 0.99 ) {
            return false;
        }
        if ( fabs ( getPhaseY()-pImage->getPhaseY() ) > 0.01 && fabs ( getPhaseY()-pImage->getPhaseY() ) < 0.99 ) {
            return false;
        }

        return true;
    }

    /**
     * \~french
     * \brief Crée un objet Image à partir de tous ses éléments constitutifs
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] bbox emprise rectangulaire de l'image
     * \~english
     * \brief Create an Image object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] bbox bounding box
     */
    Image ( int width, int height, int channels, double resx, double resy,  BoundingBox<double> bbox ) :
        width ( width ), height ( height ), channels ( channels ), resx ( resx ), resy ( resy ), bbox ( bbox ), mask ( NULL ) {}

    /**
     * \~french
     * \brief Crée une Image sans préciser de géoréférencement, ni résolutions, ni rectangle englobant
     * \details La résolution sera de 1 dans les 2 sens et le rectangle englobant sera 0,0 width,height
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \~english
     * \brief Create an Image without providing georeferencement, neither resolutions nor bounding box
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     */
    Image ( int width, int height, int channels ) : width ( width ), height ( height ), channels ( channels ), resx ( 1.), resy (1.), bbox (BoundingBox<double>(0., 0., (double) width, (double) height)), mask ( NULL ) {}

    /**
     * \~french
     * \brief Crée une Image sans préciser les résolutions
     * \details Les résolutions sont calculées à partie du rectangle englobant et des dimensions en pixel.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \~english
     * \brief Create an Image without providing resolutions
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     */
    Image ( int width, int height, int channels,  BoundingBox<double> bbox ) :
        width ( width ), height ( height ), channels ( channels ), bbox ( bbox ), mask ( NULL ) {
        computeResolutions();
    }

    /**
     * \~french
     * \brief Crée un objet Image sans préciser le rectangle englobant
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \~english
     * \brief Create an Image object without providing bbox
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     */
    Image ( int width, int height, int channels, double resx, double resy) :
        width ( width ), height ( height ), channels ( channels ), resx ( resx ), resy ( resy ),
        bbox (BoundingBox<double>(0., 0., resx * (double) width, resy * (double) height)),
        mask ( NULL ) {}

    /**
     * \~french
     * \brief Retourne une ligne en entier 8 bits.
     * Les canaux sont entrelacés. ATTENTION : si les données ne sont pas intrinsèquement codées sur des entiers 8 bits, il n'y a pas de conversion.
     * \param[out] buffer Tableau contenant au moins width*channels uint8
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    virtual int getline ( uint8_t *buffer, int line ) = 0;

    /**
     * \~french
     * \brief Retourne une ligne en entier 8 bits, avec son masque en entier 8 bits.
     *//*
    int getline ( uint8_t *image, uint8_t *mask, int line ) {
        if (mask == NULL) {
            LOGGER_ERROR("Associated mask line is asked but mask is not defined");
            return -1;
        }
        int r;
        if (r = getline ( image, line ) < 0) {
            LOGGER_ERROR("Impossible to get the image line");
            return -1;
        }
        if (getline ( image, line ) < 0) {
            LOGGER_ERROR("Impossible to get the image line");
            return -1;
        }
    }*/

    /**
     * \~french
     * \brief Retourne une ligne en flottant 32 bits.
     * Les canaux sont entrelacés. Si les données ne sont pas intrinsèquement codées sur des flottants 32 bits
     * une conversion est effectuée.
     * \param[out] buffer Tableau contenant au moins width*channels float32
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    virtual int getline ( float *buffer, int line ) = 0;

    /**
     * \~french
     * \brief Retourne une ligne en flottant 32 bits, avec son masque en flottant 32 bits.
     */
    int getline ( float *image, float *mask, int line ) {


    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Image() {
        //if (mask != NULL) delete mask;
    }

    /**
     * \~french
     * \brief Sortie des informations sur l'image
     * \~english
     * \brief Image description output
     */
    virtual void print() {
        LOGGER_INFO ( "\t- width = " << width << ", height = " << height );
        LOGGER_INFO ( "\t- samples per pixel = " << channels );
        LOGGER_INFO ( "\t- bbox = " << bbox.xmin << " " << bbox.ymin << " " << bbox.xmax << " " << bbox.ymax );
        LOGGER_INFO ( "\t- x resolution = " << resx << ", y resolution = " << resy );
        if ( mask ) {
            LOGGER_INFO ( "\t- Own a mask\n" );
        } else {
            LOGGER_INFO ( "\t- No mask\n" );
        }
    }
};

#endif
