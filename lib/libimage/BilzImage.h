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
 * \file BilzImage.h
 ** \~french
 * \brief Définition des classes BilzImage et BilzImageFactory
 * \details
 * \li BilzImage : gestion d'une image au format (Z)bil, en lecture
 * \li BilzImageFactory : usine de création d'objet BilzImage
 ** \~english
 * \brief Define classes BilzImage and BilzImageFactory
 * \details
 * \li BilzImage : manage a (Z)bil format image, reading
 * \li BilzImageFactory : factory to create BilzImage object
 */

#ifndef BILZ_IMAGE_H
#define BILZ_IMAGE_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Format.h"
#include "FileImage.h"
#include "Image.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image PNG
 * \details Une image (Z)bil est une vraie image dans ce sens où elle est rattachée à un fichier, pour la lecture de données au format bil, potentiellement compressé.
 * 
 * Lors de la lecture des données, si celle ci sont moins volumineuse qu'elles ne devraient l'être, on estime alors qu'elles sont compressées en deflate (zip). La décompression est alors automatiquement essayées.
 * 
 * \todo Lire au fur et à mesure l'image BIL et ne pas la charger intégralement en mémoire lors de la création de l'objet BilzImage.
 */
class BilzImage : public FileImage {

    friend class BilzImageFactory;

private:

    /**
     * \~french \brief Stockage de l'image entière, décompressée
     * \~english \brief Full uncompressed image storage
     */
    uint8_t* data;

protected:
    /** \~french
     * \brief Crée un objet BilzImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine BilzImageFactory, qui fera différents tests et calculs.
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
     * \param[in] bilData image complète, dans un tableau
     ** \~english
     * \brief Create a BilzImage object, from all attributes
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
     * \param[in] bilData whole image, in an array
     */
    BilzImage (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, char* name,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
        uint8_t* bilData
    );

public:
    
    static bool canRead ( int bps, SampleFormat::eSampleFormat sf) {
        return (
            ( bps == 32 && sf == SampleFormat::FLOAT ) ||
            ( bps == 8 && sf == SampleFormat::UINT )
        );
    }
    
    static bool canWrite ( int bps, SampleFormat::eSampleFormat sf) {
        return false;
    }

    int getline ( uint8_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**
     * \~french
     * \brief Ecrit une image (Z)BIL, à partir d'une image source
     * \warning Pas d'implémentation de l'écriture au format (Z)BIL, retourne systématiquement une erreur
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn ) {
        LOGGER_ERROR ( "Cannot write (Z)BIL image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image (Z)BIL, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format (Z)BIL, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint8_t* buffer ) {
        LOGGER_ERROR ( "Cannot write (Z)BIL image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image (Z)BIL, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture au format (Z)BIL, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( float* buffer)  {
        LOGGER_ERROR ( "Cannot write (Z)BIL image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image (Z)BIL, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture au format (Z)BIL, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint8_t* buffer, int line ) {
        LOGGER_ERROR ( "Cannot write (Z)BIL image" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image (Z)BIL, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture au format (Z)BIL, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( float* buffer, int line) {
        LOGGER_ERROR ( "Cannot write (Z)BIL image" );
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
    ~BilzImage() {
        /* cleanup heap allocation */
        delete [] data;
    }

    /** \~french
     * \brief Sortie des informations sur l'image (Z)BIL
     ** \~english
     * \brief (Z)BIL image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- BilzImage ------------" );
        FileImage::print();
    }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image (Z)BIL
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe BilzImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de BilzImage et de sortir en erreur en cas de problème. Dans le cas d'une image (Z)BIL pour la lecture, on récupère dans le fichier toutes les méta-informations sur l'image.
 */
class BilzImageFactory {
public:
    /** \~french
     * \brief Crée un objet BilzImage, pour la lecture
     * \details L'emprise et les résolutions ne sont pas récupérées dans le fichier HRD associé à l'image (Z)BIL, on les précise donc à l'usine. Tout le reste sera lu dans le fichier HDR. On vérifiera aussi la cohérence entre les emprises et résolutions fournies et les dimensions récupérées dans le fichier HDR.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement. Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \return un pointeur d'objet BilzImage, NULL en cas d'erreur
     ** \~english
     * \brief Create an BilzImage object, for reading
     * \details We precise bbox and resolutions. All other informations are extracted from HRD file. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation. Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \return a BilzImage object pointer, NULL if error
     */
    BilzImage* createBilzImageToRead ( char* filename, BoundingBox<double> bbox, double resx, double resy );

};


#endif

