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
 * \file LibopenjpegImage.h
 ** \~french
 * \brief Définition des classes LibopenjpegImage et LibopenjpegImageFactory
 * \details
 * \li LibopenjpegImage : gestion d'une image au format JPEG2000, en lecture, utilisant la librairie openjpeg
 * \li LibopenjpegImageFactory : usine de création d'objet LibopenjpegImage
 ** \~english
 * \brief Define classes LibopenjpegImage and LibopenjpegImageFactory
 * \details
 * \li LibopenjpegImage : manage a JPEG2000 format image, reading, using the library openjpeg
 * \li LibopenjpegImageFactory : factory to create LibopenjpegImage object
 */

#ifndef LIBOPENJPEG_IMAGE_H
#define LIBOPENJPEG_IMAGE_H

#include "BoundingBox.h"
#include "Jpeg2000Image.h"
#include "FileImage.h"
#include "Image.h"
#include "opj_includes.h"

#define JP2_RFC3745_MAGIC    "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
#define JP2_MAGIC            "\x0d\x0a\x87\x0a"
#define J2K_CODESTREAM_MAGIC "\xff\x4f\xff\x51"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image JPEG2000, avec la librarie openjpeg
 * \details Une image JPEG2000 est une vraie image dans ce sens où elle est rattachée à un fichier, pour la lecture de données au format JPEG2000. La librairie utilisée est openjpeg (open source et intégrée statiquement dans le projet ROK4).
 * 
 * \todo Lire au fur et à mesure l'image JPEG2000 et ne pas la charger intégralement en mémoire lors de la création de l'objet LibopenjpegImage.
 */
class LibopenjpegImage : public Jpeg2000Image {
    
friend class LibopenjpegImageFactory;
    
private:

    /**
     * \~french \brief Stockage de l'image entière, décompressée
     * \~english \brief Full uncompressed image storage
     */
    opj_image_t* jp2image;

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
     * \brief Crée un objet LibopenjpegImage à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine LibopenjpegImageFactory, qui fera différents tests et calculs.
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
     * \param[in] jp2ptr pointeur vers l'image JPEG2000
     ** \~english
     * \brief Create a LibopenjpegImage object, from all attributes
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
     * \param[in] jp2ptr JPEG2000 image's pointer
     */
    LibopenjpegImage (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, char* name,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
        opj_image_t* jp2ptr
    );

public:
    
    static bool canRead ( int bps, SampleFormat::eSampleFormat sf) {
        return (
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
     * \brief Destructeur par défaut
     * \details Suppression du buffer de lecture #m_data
     * \~english
     * \brief Default destructor
     * \details We remove read buffer #m_data
     */
    ~LibopenjpegImage() {
        opj_image_destroy(jp2image);
    }

    /** \~french
     * \brief Sortie des informations sur l'image JPEG2000
     ** \~english
     * \brief JPEG2000 image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- LibopenjpegImage ------------" );
        FileImage::print();
        //LOGGER_INFO ( "\t- info sup : " << info sup );
        LOGGER_INFO ( "" );
    }

};


/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image JPEG2000, manipulée avec la librairie openjpeg
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe LibopenjpegImage. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de LibopenjpegImage et de sortir en erreur en cas de problème. Dans le cas d'une image JPEG2000 pour la lecture, on récupère dans le fichier toutes les méta-informations sur l'image.
 */
class LibopenjpegImageFactory {
public:
    /** \~french
     * \brief Crée un objet LibopenjpegImage, pour la lecture
     * \details On considère que les informations d'emprise et de résolutions ne sont pas présentes dans le JPEG2000, on les précise donc à l'usine. Tout le reste sera lu dans les en-têtes JPEG2000. On vérifiera aussi la cohérence entre les emprise et résolutions fournies et les dimensions récupérées dans le fichier JPEG2000.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement. Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \return un pointeur d'objet LibopenjpegImage, NULL en cas d'erreur
     ** \~english
     * \brief Create an LibopenjpegImage object, for reading
     * \details Bbox and resolutions are not present in the JPEG2000 file, so we precise them. All other informations are extracted from JPEG2000 header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation. Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \return a LibopenjpegImage object pointer, NULL if error
     */
    LibopenjpegImage* createLibopenjpegImageToRead ( char* filename, BoundingBox<double> bbox, double resx, double resy );

};


#endif

