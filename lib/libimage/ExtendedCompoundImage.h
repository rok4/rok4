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
 * \file ExtendedCompoundImage.h
 ** \~french
 * \brief Définition des classes ExtendedCompoundImage, ExtendedCompoundMask et ExtendedCompoundImageFactory
 * \details
 * \li ExtendedCompoundImage : image composée d'images compatibles, superposables
 * \li ExtendedCompoundMask : masque composé, associé à une image composée
 * \li ExtendedCompoundImageFactory : usine de création d'objet ExtendedCompoundImage
 ** \~english
 * \brief Define classes ExtendedCompoundImage, ExtendedCompoundMask and ExtendedCompoundImageFactory
 * \details
 * \li ExtendedCompoundImage : image compounded with superimpose images
 * \li ExtendedCompoundMask : compounded mask, associated with a compounded image
 * \li ExtendedCompoundImageFactory : factory to create ExtendedCompoundImage object
 */

#ifndef EXTENTED_COMPOUND_IMAGE_H
#define EXTENTED_COMPOUND_IMAGE_H

#include <vector>
#include <cstring>
#include <iostream>
#include "math.h"

#include "Logger.h"
#include "Utils.h"
#include "Format.h"
#include "Image.h"
#include "MirrorImage.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'un paquet d'images compatibles
 * \details On va manipuler un paquet d'images superposables comme si elles n'en étaient qu'une seule. On parle d'images superposables lorsqu'elles ont :
 * \li la même résolution en X
 * \li la même résolution en Y
 * \li la même phase en X
 * \li la même phase en Y
 *
 * Les tests d'égalité acceptent un epsilon qui est le suivant :
 * \li 1% de la résolution la plus petite pour les résolutions
 * \li 1% pour les phases
 *
 * \~ \image html eci.png
 */
class ExtendedCompoundImage : public Image {

    friend class ExtendedCompoundImageFactory;

private:

    /**
     * \~french \brief Images sources, toutes superposables, utilisée pour assembler l'image composée
     * \~english \brief Source images, consistent , to make the compounded image
     */
    std::vector<Image*> images;

    /**
     * \~french \brief Nombre de miroirs dans les images sources
     * \details Certaines images sources peuvent etre des miroirs (MirrorImage). Lors de la composition de l'image, on ne veut pas que les données des vraies images soient écrasées par des données "miroirs". C'est pourquoi on veut connaître le nombre d'images miroirs dans le tableau et on sait qu'elles sont placées au début.
     * \~english \brief Mirror images' number among source images
     */
    uint mirrors;

    /**
     * \~french \brief Valeur de non-donnée
     * \details On a une valeur entière par canal. Tous les pixel de l'image composée seront initialisés avec cette valeur.
     * \~english \brief Nodata value
     */
    int* nodata;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \details Lorsque l'on veut récupérer une ligne d'une image composée, on va se reporter sur toutes les images source.
     *
     * \image html eci_getline.png
     *
     * On lit en parallèle de chaque image source l'éventuel masque qui lui est associé. Ainsi, si un pixel n'est pas réellement de la donnée, on évite d'écraser les données du dessous. Si il n'y a pas de masque, on considère l'image source comme pleine (ne contient pas de non-donnée).
     *
     * \param[in] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );

protected:

    /** \~french
     * \brief Crée un objet ExtendedCompoundImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine ExtendedCompoundImageFactory, qui fera différents tests et calculs.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] images images sources
     * \param[in] nodata valeur de non-donnée
     * \param[in] mirrors nombre d'images miroirs dans le tableau des images sources (placées au début)
     ** \~english
     * \brief Create an ExtendedCompoundImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] images source images
     * \param[in] nodata nodata value
     * \param[in] mirrors mirror images' number in source images (put in front)
     */
    ExtendedCompoundImage ( int width, int height, int channels, BoundingBox<double> bbox,
                            std::vector<Image*>& images, int* nodata, uint mirrors ) :
        Image ( width, height,channels,images.at ( 0 )->getResX(),images.at ( 0 )->getResY(),bbox ),
        images ( images ),
        nodata ( nodata ),
        mirrors ( mirrors ) {}

public:
    /**
     * \~french
     * \brief Retourne le tableau des images sources
     * \return images sources
     * \~english
     * \brief Return the array of source images
     * \return source images
     */
    std::vector<Image*>* getImages() {
        return &images;
    }

    /**
     * \~french
     * \brief Précise si au moins une image source possède un masque de donnée
     * \return présence de masque
     * \~english
     * \brief Precise if one or more source images own a mask
     * \return mask presence
     */
    bool useMasks() {
        for ( uint i=0; i < images.size(); i++ ) {
            if ( getMask ( i ) ) return true;
        }
        return false;
    }

    /**
     * \~french
     * \brief Retourne le masque de l'image source d'indice i
     * \param[in] i indice de l'image source dont on veut le masque
     * \return masque
     * \~english
     * \brief Return the mask of source images with indice i
     * \param[in] i source image indice, whose mask is wanted
     * \return mask
     */
    Image* getMask ( int i ) {
        return images.at ( i )->getMask();
    }
    /**
     * \~french
     * \brief Retourne l'image source d'indice i
     * \param[in] i indice de l'image source voulue
     * \return image
     * \~english
     * \brief Return the source images with indice i
     * \param[in] i wanted source image indice
     * \return image
     */
    Image* getImage ( int i ) {
        return images.at ( i );
    }

    /**
     * \~french
     * \brief Retourne le nombre de miroirs parmi les images sources
     * \return nombre d'images miroirs
     * \~english
     * \brief Return the mirror images' number among source images
     * \return mirror images' number
     */
    uint getMirrorsNumber() {
        return mirrors;
    }

    /**
     * \~french
     * \brief Retourne la valeur de non-donnée
     * \return Valeur de non-donnée
     * \~english
     * \brief Return the nodata value
     * \return nodata value
     */
    int* getNodata() {
        return nodata;
    }

    int getline ( uint8_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression de toutes les images composant l'ExtendedCompoundImage
     * \~english
     * \brief Default destructor
     */
    virtual ~ExtendedCompoundImage() {
        for ( uint i=0; i < images.size(); i++ ) {
            delete images[i];
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image composée
     ** \~english
     * \brief Compounded image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ ExtendedCompoundImage -------" );
        Image::print();
        LOGGER_INFO ( "\t- Number of images = " << images.size() );
        LOGGER_INFO ( "\t- Number of mirrors = " << mirrors );
        LOGGER_INFO ( "\t- Nodata value " << nodata << "\n" );
    }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image composée
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe ExtendedCompoundImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de ExtendedCompoundImage et de sortir en erreur en cas de problème.
 */
class ExtendedCompoundImageFactory {
public:
    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image composée et crée un objet ExtendedCompoundImage
     * \details Largeur, hauteur, nombre de canaux et bbox sont déduits des composantes de l'image source et des paramètres. On vérifie la superposabilité des images sources.
     * \param[in] images images sources
     * \param[in] nodata valeur de non-donnée
     * \param[in] mirrors nombre d'images miroirs dans le tableau des images sources (placées au début)
     * \return un pointeur d'objet ExtendedCompoundImage, NULL en cas d'erreur
     ** \~english
     * \brief Check and calculate compounded image components and create an ExtendedCompoundImage object
     * \details Height, width, samples' number and bbox are deduced from source image's components and parameters. We check if source images are superimpose.
     * \param[in] images source images
     * \param[in] nodata nodata value
     * \param[in] mirrors mirror images' number in source images (put in front)
     * \return a ExtendedCompoundImage object pointer, NULL if error
     */
    ExtendedCompoundImage* createExtendedCompoundImage ( std::vector<Image*>& images, int* nodata, uint mirrors );

    /** \~french
     * \brief Vérifie la superposabilité des images sources et crée un objet ExtendedCompoundImage
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] images images sources
     * \param[in] nodata valeur de non-donnée
     * \param[in] mirrors nombre d'images miroirs dans le tableau des images sources (placées au début)
     * \return un pointeur d'objet ExtendedCompoundImage, NULL en cas d'erreur
     ** \~english
     * \brief Check if source images are superimpose and create an ExtendedCompoundImage object
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] images source images
     * \param[in] nodata nodata value
     * \param[in] mirrors mirror images' number in source images (put in front)
     * \return a ExtendedCompoundImage object pointer, NULL if error
     */
    ExtendedCompoundImage* createExtendedCompoundImage ( int width, int height, int channels,
            BoundingBox<double> bbox,
            std::vector<Image*>& images, int* nodata, uint mirrors );
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'un masque composé, s'appuyant sur une image composée
 */
class ExtendedCompoundMask : public Image {

private:
    /**
     * \~french \brief Image composée, à laquelle le masque composé est associé
     * \~english \brief Compounded images, with which compounded mask is associated
     */
    ExtendedCompoundImage* ECI;

    /** \~french
     * \brief Retourne une ligne entière
     * \details Lors ce que l'on veut récupérer une ligne d'un masque composé, on va se reporter sur tous les masques des images source de l'image composée associée. Si une des images sources n'a pas de masque, on considère que celle-ci est pleine (ne contient pas de non-donnée).
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    int _getline ( uint8_t* buffer, int line );

public:
    /** \~french
     * \brief Crée un ExtendedCompoundMask
     * \details Les caractéristiques du masque sont extraites de l'image composée.
     * \param[in] ECI Image composée
     ** \~english
     * \brief Create a ExtendedCompoundMask
     * \details Mask's components are extracted from the compounded image.
     * \param[in] ECI Compounded image
     */
    ExtendedCompoundMask ( ExtendedCompoundImage*& ECI ) :
        Image ( ECI->width, ECI->height, 1, ECI->getResX(), ECI->getResY(),ECI->getBbox() ),
        ECI ( ECI ) {}

    int getline ( uint8_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~ExtendedCompoundMask() {}

    /** \~french
     * \brief Sortie des informations sur le masque composé
     ** \~english
     * \brief Compounded mask description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ ExtendedCompoundMask -------" );
        Image::print();
    }

};

#endif
