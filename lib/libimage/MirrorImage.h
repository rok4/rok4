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
 * \file MirrorImage.h
 ** \~french
 * \brief Définition des classes MirrorImage et MirrorImageFactory
 * \details
 * \li MirrorImage : image par reflet
 * \li MirrorImageFactory : usine de création d'objet MirrorImage
 ** \~english
 * \brief Define classes MirrorImage and MirrorImageFactory
 * \details
 * \li MirrorImage : reflection image
 * \li MirrorImageFactory : factory to create MirrorImage object
 */

#ifndef MIRROR_IMAGE_H
#define MIRROR_IMAGE_H

#include "Image.h"
#include "Format.h"
#include <vector>
#include <cstring>
#include "Logger.h"
#include "Utils.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image miroir
 * \details Une image miroir est viruelle et n'est en fait que le reflet d'une autre image. Pour la définir, on a donc besoin d'une image source et de la position du miroir par rapport à cette image source. On définit aussi la largeur en pixel de ce miroir
 *
 * On peut voir aussi les miroirs comme un buffer, autour d'une image. Ces images factices sont utilisées notamment pour éviter les effets de bord lors de réechantillonnage.
 *
 * \~ \image html miroirs_pos.png
 */
class MirrorImage : public Image {

    /*   mirrorSize
        <--------->
         ________________________________________
        |                                        |  /\
        |             position 0                 |  |   mirrorSize
        |________________________________________|  V
        |         |                    |         |
        |         |                    |         |
        | position|                    |position |
        |   3     |   Image source     |    1    |
        |         |                    |         |
        |         |                    |         |
        |_________|____________________|_________|
        |                                        |
        |             position 2                 |
        |________________________________________| */

    friend class MirrorImageFactory;

private:
    /**
     * \~french \brief Image source, utilisée pour composer l'image miroir
     * \~english \brief Source images, to compose the mirror image
     */
    Image* sourceImage;
    /**
     * \~french \brief Position du miroir par rapport à l'image source
     * \details
     * \li 0 : en haut
     * \li 1 : à droite
     * \li 2 : en bas
     * \li 3 : à gauche
     * \~english \brief Mirror position to image source
     * \details
     * \li 0 : top
     * \li 1 : right
     * \li 2 : bottom
     * \li 3 : left
     */
    int position;
    /**
     * \~french \brief Taille du miroir en pixel
     * \details Selon la position du miroir, cette taille correspondra à la largeur ou à la hauteur de l'image.
     *
     * En pratique, cette taille correspondra au nombre de pixels nécessaires pour l'interpolation.
     * \~english \brief Mirror's size, in pixel
     */
    uint mirrorSize;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \details Lorsque l'on veut récupérer une ligne d'une image miroir, on va se reporter sur l'image source,
     *
     * Exemple : si on veut la denière ligne du miroir haut (position 0), on récupérera la première ligne de l'image source.
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );

protected:
    /** \~french
     * \brief Crée un objet MirrorImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine mirrorImageFactory, qui fera différents tests et calculs.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] image image source
     * \param[in] position position du miroir par rapport à l'image source
     * \param[in] mirrorSize taille du miroir en pixel
     ** \~english
     * \brief Create a MirrorImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] image source image
     * \param[in] position mirror position to image source
     * \param[in] mirrorSize mirror's size, in pixel
     */
    MirrorImage ( int width, int height, int channels, BoundingBox<double> bbox, Image* image, int position,uint mirrorSize ) : Image ( width,height,channels,image->getResX(),image->getResY(),bbox ), sourceImage ( image ), position ( position ), mirrorSize ( mirrorSize ) {}

public:

    int getline ( uint8_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~MirrorImage() {}

    /** \~french
     * \brief Sortie des informations sur l'image miroir
     ** \~english
     * \brief Mirror image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ MirrorImage -------" );
        Image::print();
        LOGGER_INFO ( "\t- Mirror's position = " << position );
        LOGGER_INFO ( "\t- Mirror's size = " << mirrorSize );
        LOGGER_INFO ( "" );
    }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image miroir
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe MirrorImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de MirrorImage et de sortir en erreur en cas de problème.
 */
class MirrorImageFactory {
public:
    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image miroir et crée un objet MirrorImage
     * \details Largeur, hauteur, nombre de canaux et bbox sont déduits des composantes de l'image source et des paramètres.
     * \param[in] pImageSrc image source
     * \param[in] position position du miroir par rapport à l'image source
     * \param[in] mirrorSize taille du miroir en pixel
     * \return un pointeur d'objet MirrorImage, NULL en cas d'erreur
     ** \~english
     * \brief Check and calculate mirror image components and create a MirrorImage object
     * \details Height, width, samples' number and bbox are deduced from source image's components and parameters.
     * \param[in] pImageSrc source image
     * \param[in] position mirror position to image source
     * \param[in] mirrorSize mirror's size, in pixel
     * \return a MirrorImage object pointer, NULL if error
     */
    MirrorImage* createMirrorImage ( Image* pImageSrc, int position,uint mirrorSize );
};

#endif
