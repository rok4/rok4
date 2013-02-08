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
 * \brief Définition des classes MergeImage, MergeImageFactory et Pixel
 * \details
 * \li MergeImage : image résultant de la fusion d'images semblables, selon différents modes de composition
 * \li MergeImageFactory : usine de création d'objet MergeImage
 * \li Pixel
 ** \~english
 * \brief Define classes MergeImage, MergeImageFactory and ExtendedCompoundImageFactory
 * \details
 * \li MergeImage : image merged with similar images, with different merge methods
 * \li MergeImageFactory : factory to create MergeImage object
 * \li Pixel
 */

#ifndef MERGEIMAGE_H
#define MERGEIMAGE_H

#include "Image.h"
#include "Format.h"

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
 * \li Multiply
 * \li Transparency
 * \li Mask
 * \li Normal
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
         * \details On a une valeur entière par canal. Tous les pixel de l'image fusionnée seront initialisés avec cette valeur.
         * \~english \brief Background value
         */
        int* bgValue;
        
        void mergeline ( uint8_t* buffer, uint8_t* back, uint8_t* front );
        void mergeline ( float* buffer, uint8_t* back, uint8_t* front );

        /** \~french
         * \brief Retourne une ligne, flottante ou entière
         * \param[in] buffer Tableau contenant au moins width*channels valeurs
         * \param[in] line Indice de la ligne à retourner (0 <= line < height)
         * \return taille utile du buffer, 0 si erreur
         */
        template<typename T>
        int _getline(T* buffer, int line);

    protected:

        MergeImage ( std::vector< Image* >& images, int channels, SampleType ST, int* bgValue, Merge::MergeType composition = Merge::NORMAL ) :
            Image(images.at(0)->width,images.at(0)->height,images.at(0)->getResX(),images.at(0)->getResY(),
                  channels,images.at(0)->getBbox()),
            images ( images ), composition ( composition ), bgValue(bgValue), ST(ST) {}
        

    public:
        
        virtual int getline ( float* buffer, int line );
        virtual int getline ( uint8_t* buffer, int line );

        /**
         * \~french
         * \brief Destructeur par défaut
         * \details Suppression de toutes les images composant la MergeImage
         * \~english
         * \brief Default destructor
         */
        virtual ~MergeImage() {
            for(int i = 0; i < images.size(); i++) { delete images[i]; }
        }

        /** \~french
         * \brief Sortie des informations sur l'image fusionnée
         ** \~english
         * \brief Merged image description output
         */
        void print() {
            LOGGER_INFO("");
            LOGGER_INFO("------ MergeImage -------");
            Image::print();
            LOGGER_INFO("\t- Number of images = " << images.size());
            LOGGER_INFO("\t- Merge method : " << composition << "\n");
            LOGGER_INFO("\t- Background value : " << bgValue << "\n");
        }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image fusionnée
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe MergeImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de MergeImage et de sortir en erreur en cas de problème.
 */
class MergeImageFactory {
    public:
        MergeImage* createMergeImage(std::vector< Image* >& images, SampleType ST, int* bgValue, Merge::MergeType composition = Merge::NORMAL);
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Représentation d'un pixel
 */
class Pixel {
    public:
        float Sr, Sg, Sb, Sa;
        float Sra, Sga, Sba;
        
        // 3 Chan + Alpha
        Pixel ( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) : Sr(r), Sb(b), Sg(g) {
            Sa  = a/255.;
            Sra = r * Sa;
            Sga = g * Sa;
            Sba = b * Sa;
        }
        
        // 1 Chan + Alpha
        Pixel ( uint8_t x, uint8_t a = 255) : Sr(x/255), Sb(x/255), Sg(x/255), Sa(a/255) {
            Sra = Sr * Sa;
            Sga = Sra;
            Sba = Sra;
        }
};

#endif // MERGEIMAGE_H
