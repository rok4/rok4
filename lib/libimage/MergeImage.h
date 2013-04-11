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
 * \file MergeImage.h
 ** \~french
 * \brief Définition des classes MergeImage, MergeImageFactory et MergeMask et du namespace Merge
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeImageFactory : usine de création d'objet MergeImage
 * \li MergeMask : masque fusionné, associé à une image fusionnée
 * \li Merge : énumère et manipule les différentes méthodes de fusion
 ** \~english
 * \brief Define classes MergeImage, MergeImageFactory and MergeMask and the namespace Merge
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeImageFactory : factory to create MergeImage object
 * \li MergeMask : merged mask, associated with a merged image
 * \li Merge : enumerate and managed different merge methods
 */

#ifndef MERGEIMAGE_H
#define MERGEIMAGE_H

#include "Image.h"
#include <string.h>
#include "Format.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées à la méthode de fusion
 * \~english \brief Manage informations in connection with merge method
 */
namespace Merge {
/**
 * \~french \brief Énumération des méthodes de fusion disponibles
 * \~english \brief Available merge methods enumeration
 */
enum MergeType {
    UNKNOWN = 0,
    NORMAL = 1,
    LIGHTEN = 2,
    DARKEN = 3,
    MULTIPLY = 4,
    TRANSPARENCY = 5,
    MASK = 6
};

/**
 * \~french \brief Nombre de méthodes disponibles
 * \~english \brief Number of available merge methods
 */
const int mergeType_size = 6;

/**
 * \~french \brief Conversion d'une chaîne de caractère vers une méthode de fusion de l'énumération
 * \param[in] strMergeMethod chaîne de caractère à convertir
 * \return la méthode de fusion correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a merge methods enumeration member
 * \param[in] strMergeMethod string to convert
 * \return the binding merge method, UNKNOWN (0) if string is not recognized
 */
MergeType fromString ( std::string strMergeMethod );

/**
 * \~french \brief Conversion d'une métthode de fusion vers une chaîne de caractère
 * \param[in] mergeMethod méthode de fusion à convertir
 * \return la chaîne de caractère nommant la méthode de fusion
 * \~english \brief Convert a merge method to a string
 * \param[in] mergeMethod merge method to convert
 * \return string namming the merge method
 */
std::string toString ( MergeType mergeMethod );
}

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Fusion d'images de mêmes dimensions
 * \details On va manipuler un paquet d'images semblables comme si elles n'en étaient qu'une seule. On parle d'images semblables lorsqu'elles ont :
 * \li les mêmes dimensions
 * \li le format de canaux
 *
 * On va disposer de plusieurs manières de fusionner les images :
 * \li MULTIPLY
 * \li TRANSPARENCY
 * \li MASK
 * \li NORMAL
 *
 * \todo Travailler sur un nombre de canaux variable (pour l'instant, systématiquement 4).
 */
class MergeImage : public Image {

    friend class MergeImageFactory;

private:

    /**
     * \~french \brief Images sources, toutes semblables, utilisée pour assembler l'image fusionnée
     * \~english \brief Source images, similar, to make the merged image
     */
    std::vector<Image*> images;

    /**
     * \~french \brief Méthode d'assemblage des images
     * \~english \brief Way to merge images
     */
    Merge::MergeType composition;

    /**
     * \~french \brief Type du canal
     * \~english \brief Sample type
     */
    SampleType ST;

    /**
     * \~french \brief Valeur de fond
     * \details On a une valeur entière par canal. Tous les pixels de l'image fusionnée seront initialisés avec cette valeur.
     * \~english \brief Background value
     */
    int* bgValue;

    /**
     * \~french \brief Valeur de transparence
     * \details On a 4 valeurs entières. Tous les pixels de cette valeur seront considérés comme transparent (en mode TRANSPARENCY)
     * \~english \brief Transparent value
     */
    int* transparentValue;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \param[in] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );

protected:

    /** \~french
     * \brief Crée un objet MergeImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine MergeImageFactory, qui fera différents tests et calculs.
     * \param[in] images images sources
     * \param[in] channel nombre de canaux par pixel en sortie
     * \param[in] sampleType type des canaux
     * \param[in] bgValue valeur de pixel à utiliser comme fond, un entier par canal en sortie
     * \param[in] transparentValue valeur de pixel à considérer comme transparent (peut être NULL), 3 valeurs entières
     * \param[in] composition méthode de fusion à utiliser
     ** \~english
     * \brief Create an MergeImage object, from all attributes
     * \param[in] images source images
     * \param[in] channel number of samples per output pixel
     * \param[in] sampleType samples' type
     * \param[in] bgValue pixel's value to use as background, one integer per output sample
     * \param[in] transparentValue pixel's value to consider as transparent, 3 integers
     * \param[in] composition merge method to use
     */
    MergeImage ( std::vector< Image* >& images, int channels, SampleType ST,
                 int* bg, int* transparent, Merge::MergeType composition = Merge::NORMAL ) :
        Image ( images.at ( 0 )->width,images.at ( 0 )->height,images.at ( 0 )->getResX(),images.at ( 0 )->getResY(),
                channels,images.at ( 0 )->getBbox() ),
        images ( images ), composition ( composition ), bgValue ( bg ), transparentValue ( transparent ), ST ( ST ) {

        if ( transparentValue != NULL ) {
            transparentValue = new int[3];
            memcpy ( transparentValue, transparent, 3*sizeof ( int ) );
        }

        bgValue = new int[channels];
        memcpy ( bgValue, bg, channels*sizeof ( int ) );
    }


public:

    virtual int getline ( float* buffer, int line );
    virtual int getline ( uint8_t* buffer, int line );

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
     * \brief Destructeur par défaut
     * \details Suppression de toutes les images composant la MergeImage
     * \~english
     * \brief Default destructor
     */
    virtual ~MergeImage() {
        for ( int i = 0; i < images.size(); i++ ) {
            delete images[i];
        }
    }

    /** \~french
     * \brief Sortie des informations sur l'image fusionnée
     ** \~english
     * \brief Merged image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ MergeImage -------" );
        Image::print();
        LOGGER_INFO ( "\t- Number of images = " << images.size() );
        LOGGER_INFO ( "\t- Merge method : " << toString ( composition ) << "\n" );
        LOGGER_INFO ( "\t- Background value : " << bgValue << "\n" );
    }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image fusionnée
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe MergeImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de MergeImage et de sortir en erreur en cas de problème.
 */
class MergeImageFactory {
public:

    /** \~french
     * \brief Teste et calcule les caractéristiques d'une image fusionnée et crée un objet MergeImage
     * \details Toutes les images sources doivent avoir les même dimensions pixel.
     * \param[in] images images sources
     * \param[in] sampleType type des canaux
     * \param[in] channel nombre de canaux par pixel en sortie
     * \param[in] bgValue valeur de pixel à utiliser comme fond, un entier par canal en sortie
     * \param[in] transparentValue valeur de pixel à considérer comme transparent (peut être NULL), 3 valeurs entières
     * \param[in] composition méthode de fusion à utiliser
     ** \~english
     * \brief Check and calculate compounded image components and create an MergeImage object
     * \details All source images have to own same dimesions.
     * \param[in] images source images
     * \param[in] sampleType samples' type
     * \param[in] channel number of samples per output pixel
     * \param[in] bgValue pixel's value to use as background, one integer per output sample
     * \param[in] transparentValue pixel's value to consider as transparent, 3 integers
     * \param[in] composition merge method to use
     */
    MergeImage* createMergeImage ( std::vector< Image* >& images, SampleType ST, int channels,
                                   int* bgValue, int* transparentValue, Merge::MergeType composition = Merge::NORMAL );
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'un masque fusionné, s'appuyant sur une image fusionné
 */
class MergeMask : public Image {

private:
    /**
     * \~french \brief Image fusionnée, à laquelle le masque fusionné est associé
     * \~english \brief Merged images, with which merged mask is associated
     */
    MergeImage* MI;

    /** \~french
     * \brief Retourne une ligne entière
     * \details Lors ce que l'on veut récupérer une ligne d'un masque fusionné, on va se reporter sur tous les masques des images source de l'image fusionnée associée. Si une des images sources n'a pas de masque, on considère que celle-ci est pleine (ne contient pas de non-donnée).
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    int _getline ( uint8_t* buffer, int line );

public:
    /** \~french
     * \brief Crée un MergeMask
     * \details Les caractéristiques du masque sont extraites de l'image fusionnée.
     * \param[in] MI Image composée
     ** \~english
     * \brief Create a MergeMask
     * \details Mask's components are extracted from the merged image.
     * \param[in] MI Compounded image
     */
    MergeMask ( MergeImage*& MI ) :
        Image ( MI->width, MI->height, MI->getResX(), MI->getResY(), 1,MI->getBbox() ),
        MI ( MI ) {}

    int getline ( uint8_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~MergeMask() {}

    /** \~french
     * \brief Sortie des informations sur le masque fusionné
     ** \~english
     * \brief Merged mask description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "------ MergeMask -------" );
        Image::print();
    }

};

#endif // MERGEIMAGE_H
