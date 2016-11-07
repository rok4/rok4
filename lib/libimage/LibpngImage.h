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
 * \file LibpngImage.h
 ** \~french
 * \brief Définition des classes LibpngImage et LibpngImageFactory
 * \details
 * \li LibpngImage : gestion d'une image au format PNG, en lecture, utilisant la librairie libpng
 * \li LibpngImageFactory : usine de création d'objet LibpngImage
 ** \~english
 * \brief Define classes LibpngImage and LibpngImageFactory
 * \details
 * \li LibpngImage : manage a PNG format image, reading, using the library libpng
 * \li LibpngImageFactory : factory to create LibpngImage object
 */

#ifndef LIBPNG_IMAGE_H
#define LIBPNG_IMAGE_H

#include <png.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Format.h"
#include "FileImage.h"
#include "Image.h"
#include "Data.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image PNG
 * \details Une image PNG est une vraie image dans ce sens où elle est rattachée à un fichier, pour la lecture de données au format PNG.
 *
 * Cette classe va utiliser la librairie libpng afin de lire les données et de récupérer les informations sur les images. L'utilisation de la librairie permet de lire des PNG ayant une palette, ayant un canal (gris) sur 1, 2 ou 4 bits. La conversion sera faite à la volée au moment de la lecture, afin de récupérer une ligne dans un format lisible par la libimage (entier sur 8 bits, gris ou rgb, avec ou sans alpha non associé).
 *
 * Si l'image gère la transparence, l'alpha est forcément non-associé aux autres canaux (spécifications PNG). Il n'y a donc pas besoin de préciser #associatedalpha.
 * 
 * \todo Lire au fur et à mesure l'image PNG et ne pas la charger intégralement en mémoire lors de la création de l'objet LibpngImage.
 */
class LibpngImage : public FileImage {

    friend class LibpngImageFactory;

private:

    /**
     * \~french \brief Stockage de l'image entière, décompressée
     * \~english \brief Full uncompressed image storage
     */
    png_bytep * data;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );    

protected:
    /** \~french
     * \brief Crée un objet LibpngImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine LibpngImageFactory, qui fera différents tests et calculs.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] name chemin du fichier image
     * \param[in] sampleformat format des canaux
     * \param[in] bitspersample nombre de bits par canal
     * \param[in] photometric photométrie des données
     * \param[in] compression compression des données
     * \param[in] pngData image complète, dans un tableau
     ** \~english
     * \brief Create a LibpngImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] name path to image file
     * \param[in] sampleformat samples' format
     * \param[in] bitspersample number of bits per sample
     * \param[in] photometric data photometric
     * \param[in] compression data compression
     * \param[in] pngData whole image, in an array
     */
    LibpngImage (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, char* name,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
        png_bytep* pngData
    );

public:
    
    static bool canRead ( int bps, SampleFormat::eSampleFormat sf) {
        return ( 
            ( bps == 1 && sf == SampleFormat::UINT ) ||
            ( bps == 2 && sf == SampleFormat::UINT ) ||
            ( bps == 4 && sf == SampleFormat::UINT ) ||
            ( bps == 8 && sf == SampleFormat::UINT )
        );
    }
    
    static bool canWrite ( int bps, SampleFormat::eSampleFormat sf) {
        return false;
    }

    int getline ( uint8_t* buffer, int line );
    int getline ( uint16_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Ecrit une image PNG, à partir d'une image source
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn ) {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image PNG, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint8_t* buffer ) {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image PNG, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint16_t* buffer ) {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image PNG, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( float* buffer)  {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image PNG, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint8_t* buffer, int line ) {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image PNG, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint16_t* buffer, int line ) {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image PNG, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture au format PNG, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( float* buffer, int line) {
        LOGGER_ERROR ( "Cannot write PNG image" );
        return -1;
    }

    
    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression du buffer de lecture #row_pointers
     * \~english
     * \brief Default destructor
     * \details We remove read buffer #row_pointers
     */
    ~LibpngImage() {
        /* cleanup heap allocation */
        for (int y = 0; y < height; y++)
            free(data[y]);
        free(data);
    }

    /** \~french
     * \brief Sortie des informations sur l'image PNG
     ** \~english
     * \brief PNG image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- LibpngImage ------------" );
        FileImage::print();
    }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image PNG
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe LibpngImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de LibpngImage et de sortir en erreur en cas de problème. Dans le cas d'une image PNG pour la lecture, on récupère dans le fichier toutes les méta-informations sur l'image.
 */
class LibpngImageFactory {
public:
    /** \~french
     * \brief Crée un objet LibpngImage, pour la lecture
     * \details On considère que les informations d'emprise et de résolutions ne sont pas présentes dans le PNG, on les précise donc à l'usine. Tout le reste sera lu dans les en-têtes PNG. On vérifiera aussi la cohérence entre les emprise et résolutions fournies et les dimensions récupérées dans le fichier PNG.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement. Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \return un pointeur d'objet LibpngImage, NULL en cas d'erreur
     ** \~english
     * \brief Create an LibpngImage object, for reading
     * \details Bbox and resolutions are not present in the PNG file, so we precise them. All other informations are extracted from PNG header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation. Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \return a LibpngImage object pointer, NULL if error
     */
    LibpngImage* createLibpngImageToRead ( char* filename, BoundingBox<double> bbox, double resx, double resy );

    LibpngImage* createLibpngImageToReadFromBuffer (RawDataStream *rawStream, BoundingBox<double> bbox, double resx, double resy );

};


#endif

