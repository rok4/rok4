/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file LibtiffImage.h
 ** \~french
 * \brief Définition des classes LibtiffImage et LibtiffImageFactory
 * \details
 * \li LibtiffImage : gestion d'une image au format TIFF, en écriture et lecture, utilisant la librairie libtiff
 * \li LibtiffImageFactory : usine de création d'objet LibtiffImage
 ** \~english
 * \brief Define classes LibtiffImage and LibtiffImageFactory
 * \details
 * \li LibtiffImage : manage a TIFF format image, reading and writting, using the library libtiff
 * \li LibtiffImageFactory : factory to create LibtiffImage object
 */

#ifndef LIBTIFF_IMAGE_H
#define LIBTIFF_IMAGE_H

#include "Image.h"
#include "tiffio.h"
#include <string.h>
#include "Format.h"
#include "FileImage.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image TIFF
 * \details Une image TIFF est une vraie image dans ce sens où elle est rattachée à un fichier, que ce soit pour la lecture ou l'écriture de données au format TIFF. Seule cette classe ne va pas s'appuyer sur une autre image pour être manipulée.
 *
 * Cette classe va utiliser la librairie TIFF afin de lire/écrire les données et de récupérer/fournir les informations sur les images.
 *
 * Si les images lues possèdent un canal alpha, celui-ci ne doit pas être associé, c'est-à-dire qu'on conserve la valeur des autres canaux. De même en écriture, on considère que s'il y a un canal alpha, il n'a pas été prémultiplié aux autres canaux lors des traitements et l'image écrite est en alpha non associé.
 *
 * \todo Utiliser le code de la classe TiffReader pour permettre à LibtiffImage de lire des images tuilées.
 */
class LibtiffImage : public FileImage {

    friend class LibtiffImageFactory;

private:
    /**
     * \~french \brief Image TIFF, servant d'interface entre le fichier et l'objet
     * \~english \brief TIFF image, used as interface between file and object
     */
    TIFF* tif;

    /**
     * \~french \brief Nombre de ligne dans un strip
     * \~english \brief Number of line in one strip
     */
    uint16_t rowsperstrip;

    /**
     * \~french \brief Buffer de lecture, de taille strip_size
     * \~english \brief Read buffer, strip_size long
     */
    uint8_t* strip_buffer;
    /**
     * \~french \brief Indice du strip en mémoire dans strip_buffer
     * \~english \brief Memorized strip indice, in strip_buffer
     */
    uint16_t current_strip;
    
    /**
     * \~french \brief Doit convertir les canaux de 1 à 8 bits
     * \details
     * \li 0 = pas de conversion
     * \li 1 = conversion nécessaire, et 0 est blanc (min-is-white)
     * \li 2 = conversion nécessaire, et 0 est noir (min-is-black)
     * \~english \brief Have we to convert samples from 1 to 8 bits
     * \details
     * \li 0 = no conversion
     * \li 1 = conversion, and 0 is white (min-is-white)
     * \li 2 = conversion, and 0 is black (min-is-black)
     */
    int oneTo8bits;
    
    /**
     * \~french \brief Buffer de conversion de 1 à 8 bits
     * \~english \brief Converting buffer, from 1 to 8 bits
     */
    uint8_t* oneTo8bits_buffer;

    /** \~french
     * \brief Retourne une ligne, flottante ou entière
     * \details Lorsque l'on veut récupérer une ligne d'une image TIFF, On fait appel à la fonction de la librairie TIFF TIFFReadEncodedStrip
     * \param[out] buffer Tableau contenant au moins width*channels valeurs
     * \param[in] line Indice de la ligne à retourner (0 <= line < height)
     * \return taille utile du buffer, 0 si erreur
     */
    template<typename T>
    int _getline ( T* buffer, int line );

protected:
    /** \~french
     * \brief Crée un objet LibtiffImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine LibtiffImageFactory, qui fera différents tests et calculs.
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
     * \param[in] tiff interface de la librairie TIFF entre le fichier et l'objet
     * \param[in] rowsperstrip taille de la bufferisation des données, en nombre de lignes
     * \param[in] esType type du canal supplémentaire, si présent.
     ** \~english
     * \brief Create a LibtiffImage object, from all attributes
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
     * \param[in] tiff interface between file and object
     * \param[in] rowsperstrip data buffering size, in line number
     * \param[in] esType extra sample type
     */
    LibtiffImage (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, char* name,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
        Compression::eCompression compression, TIFF* tif, int rowsperstrip, ExtraSample::eExtraSample esType = ExtraSample::ALPHA_UNASSOC
    );
    
    /** \~french
     * \brief Crée un objet LibtiffImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine LibtiffImageFactory, qui fera différents tests et calculs.
     * Les informations sur l'image sont passée au format TIFF (entiers), et sont convertis au format de la libimage (utilisant les enumérations SampleFormat, Compression et Photometric) par le constructeur. Cela permet de détecter le besoin de convertir à la volée les canaux sur 1 bit en 8 bits.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] resx résolution dans le sens des X
     * \param[in] resy résolution dans le sens des Y
     * \param[in] channel nombre de canaux par pixel
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] name chemin du fichier image
     * \param[in] sf format des canaux
     * \param[in] bps nombre de bits par canal
     * \param[in] ph photométrie des données
     * \param[in] comp compression des données
     * \param[in] tiff interface de la librairie TIFF entre le fichier et l'objet
     * \param[in] rowsperstrip taille de la bufferisation des données, en nombre de lignes
     * \param[in] esType type du canal supplémentaire, si présent.
     ** \~english
     * \brief Create a LibtiffImage object, from all attributes
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] resx X wise resolution
     * \param[in] resy Y wise resolution
     * \param[in] channel number of samples per pixel
     * \param[in] bbox bounding box
     * \param[in] name path to image file
     * \param[in] sf samples' format
     * \param[in] bps number of bits per sample
     * \param[in] ph data photometric
     * \param[in] comp data compression
     * \param[in] tiff interface between file and object
     * \param[in] rowsperstrip data buffering size, in line number
     * \param[in] esType extra sample type
     */
    LibtiffImage (
        int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
        int sf, int bps, int ph,
        int comp, TIFF* tif, int rowsperstrip, ExtraSample::eExtraSample esType = ExtraSample::ALPHA_UNASSOC
    );

public:
    
    static bool canRead ( int bps, SampleFormat::eSampleFormat sf) {
        return ( 
            ( bps == 32 && sf == SampleFormat::FLOAT ) || 
            ( bps == 16 && sf == SampleFormat::UINT ) ||
            ( bps == 8 && sf == SampleFormat::UINT ) ||
            ( bps == 1 && sf == SampleFormat::UINT )
        );
    }
    
    static bool canWrite ( int bps, SampleFormat::eSampleFormat sf) {
        return ( 
            ( bps == 32 && sf == SampleFormat::FLOAT ) || 
            ( bps == 8 && sf == SampleFormat::UINT ) || 
            ( bps == 16 && sf == SampleFormat::UINT )
        );
    }

    int getline ( uint8_t* buffer, int line );
    int getline ( uint16_t *buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Ecrit une image TIFF, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture d'une image sont dans l'objet LibtiffImage, sauf les données à écrire. On renseigne cela via une seconde image.
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn );

    /**
     * \~french
     * \brief Ecrit une image TIFF, à partir d'un buffer d'entiers
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint8_t* buffer );

    /**
     * \~french
     * \brief Ecrit une image TIFF, à partir d'un buffer d'entiers 16 bits
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint16_t* buffer );

    /**
     * \~french
     * \brief Ecrit une image TIFF, à partir d'un buffer de flottants
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( float* buffer );

    /**
     * \~french
     * \brief Ecrit une ligne d'image TIFF, à partir d'un buffer d'entiers
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint8_t* buffer, int line );

    /**
     * \~french
     * \brief Ecrit une ligne d'image TIFF, à partir d'un buffer d'entiers
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint16_t* buffer, int line );

    /**
     * \~french
     * \brief Ecrit une ligne d'image TIFF, à partir d'un buffer de flottants
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( float* buffer, int line);

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression des buffer de lecture et de l'interface TIFF
     * \~english
     * \brief Default destructor
     * \details We remove read buffer and TIFF interface
     */
    ~LibtiffImage() {
        delete [] strip_buffer;
        if (oneTo8bits) delete [] oneTo8bits_buffer;
        TIFFClose ( tif );
    }

    /** \~french
     * \brief Sortie des informations sur l'image TIFF
     ** \~english
     * \brief TIFF image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- LibtiffImage ------------" );
        FileImage::print();
        LOGGER_INFO ( "\t- Rows per strip : " << rowsperstrip );
        if (oneTo8bits == 1) LOGGER_INFO ( "\t- We have to convert samples to 8 bits (min is white)");
        if (oneTo8bits == 2) LOGGER_INFO ( "\t- We have to convert samples to 8 bits (min is black)");
        LOGGER_INFO ( "" );
    }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image TIFF
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe LibtiffImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de LibtiffImage et de sortir en erreur en cas de problème. Dans le cas d'une image TIFF pour la lecture, on récupère dans le fichier toutes les méta-informations sur l'image. Pour l'écriture, on doit tout préciser afin de constituer l'en-tête TIFF.
 */
class LibtiffImageFactory {
public:
    /** \~french
     * \brief Crée un objet LibtiffImage, pour la lecture
     * \details On considère que les informations d'emprise et de résolutions ne sont pas présentes dans le TIFF, on les précise donc à l'usine. Tout le reste sera lu dans les en-têtes TIFF. On vérifiera aussi la cohérence entre les emprise et résolutions fournies et les dimensions récupérées dans le fichier TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement. Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \return un pointeur d'objet LibtiffImage, NULL en cas d'erreur
     ** \~english
     * \brief Create an LibtiffImage object, for reading
     * \details Bbox and resolutions are not present in the TIFF file, so we precise them. All other informations are extracted from TIFF header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation. Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \return a LibtiffImage object pointer, NULL if error
     */
    LibtiffImage* createLibtiffImageToRead ( char* filename, BoundingBox<double> bbox, double resx, double resy );

    /** \~french
     * \brief Crée un objet LibtiffImage, pour l'écriture
     * \details Toutes les méta-informations sur l'image doivent être précisées pour écrire l'en-tête TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement. Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \param[in] width largeur de l'image en pixel
     * \param[in] height hauteur de l'image en pixel
     * \param[in] channel nombre de canaux par pixel
     * \param[in] sampleformat format des canaux
     * \param[in] bitspersample nombre de bits par canal
     * \param[in] photometric photométie des données
     * \param[in] compression compression des données
     * \param[in] rowsperstrip taille de la bufferisation des données, en nombre de lignes
     * \return un pointeur d'objet LibtiffImage, NULL en cas d'erreur
     ** \~english
     * \brief Create an LibtiffImage object, for writting
     * \details All informations have to be provided to be written in the TIFF header.
     *
     * Negative resolutions leads to georeferencement calculation. Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \param[in] width image width, in pixel
     * \param[in] height image height, in pixel
     * \param[in] channel number of samples per pixel
     * \param[in] sampleformat samples' format
     * \param[in] bitspersample number of bits per sample
     * \param[in] photometric data photometric
     * \param[in] compression data compression
     * \param[in] rowsperstrip data buffering size, in line number
     * \return a LibtiffImage object pointer, NULL if error
     */
    LibtiffImage* createLibtiffImageToWrite (
        char* filename, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
        Compression::eCompression compression, uint16_t rowsperstrip = 16
    );
};

#endif

