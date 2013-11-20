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
 * \file Rok4Image.h
 ** \~french
 * \brief Définition des classes Rok4Image et Rok4ImageFactory
 * \details
 * \li Rok4Image : gestion d'une image aux spécifications ROK4 Server (TIFF tuilé), en écriture et lecture
 * \li Rok4ImageFactory : usine de création d'objet Rok4Image
 ** \~english
 * \brief Define classes Rok4Image and Rok4ImageFactory
 * \details
 * \li Rok4Image : manage a ROK4 Server specifications image (tiled TIFF), reading and writting
 * \li Rok4ImageFactory : factory to create Rok4Image object
 */

#ifndef LIBTIFF_IMAGE_H
#define LIBTIFF_IMAGE_H

#include "Image.h"
#include "tiffio.h"
#include <string.h>
#include "Format.h"
#include "zlib.h"
#include <jpeglib.h>
#include "FileImage.h"

#define ROK4_IMAGE_HEADER_SIZE 2048
#define JPEG_BLOC_SIZE 16

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image ROK4 Server
 * \details Une image aux specifications ROK4 est une image qui pourra être utilisée par le serveur WMS / WMTS ROK4. Une telle image est au format TIFF, est tuilée, et supporte différentes compressions des données :
 * \li Aucune
 * \li JPEG
 * \li LZW
 * \li Deflate
 * \li PackBit
 * \li PNG : format propre au projet ROK4, cela consiste en des fichiers PNG indépendants (avec en-tête) en lieu et place des tuile de l'image. Ce fonctionnement permet de retourner des tuiles en PNG sans traitement par le serveur, comme pour le format JPEG (format officiel du TIFF).
 *
 * Particularité d'une image ROK4 :
 * \li l'en-tête TIFF fait toujours 2048 (ROK4_IMAGE_HEADER) octets. CEla permet au serveur de ne pas lire cette en-tête, et de passer directement aux données.
 *
 * Toutes les spécifications sont disponible à [cette adresse](http://www.rok4.org/documentation/specifications-pyramides-dimage).
 */
class Rok4Image : public FileImage {

    friend class Rok4ImageFactory;

private:

    /**************************** Pour la lecture ****************************/

    /**
     * \~french \brief Largeur d'une tuile de l'image en pixel
     * \~english \brief Image's tile width, in pixel
     */
    int tileWidth;
    /**
     * \~french \brief Hauteur d'une tuile de l'image en pixel
     * \~english \brief Image's tile height, in pixel
     */
    int tileHeight;
    /**
     * \~french \brief Nombre de tuile dans le sens de la largeur
     * \~english \brief Number of tiles, widthwise
     */
    int tileWidthwise;
    /**
     * \~french \brief Nombre de tuile dans le sens de la hauteur
     * \~english \brief Number of tiles, heightwise
     */
    int tileHeightwise;
    /**
     * \~french \brief Nombre de tuile dans l'image
     * \~english \brief Number of tiles
     */
    int tilesNumber;

    /**
     * \~french \brief Taille brut en octet d'une ligne d'une tuile
     * \~english \brief Raw byte size of a tile's line
     */
    int rawTileLineSize;

    /**
     * \~french \brief Taille brut en octet d'une tuile
     * \~english \brief Raw byte size of a tile
     */
    int rawTileSize;

    /**************************** Pour la lecture ****************************/
    /**
     * \~french \brief Nombre de tuiles mémorisées
     * \~english \brief Number of memorized tiles
     */
    int memorySize;

    /**
     * \~french \brief Buffer contenant les tuiles mémorisées
     * \details On mémorise les tuiles décompressées
     * Taille : memorySize
     * \~english \brief Buffer containing memorized tiles
     * \details We memorize uncompressed tiles
     * Size : memorySize
     */
    uint8_t **memorizedTiles;

    /**
     * \~french \brief Buffer précisant pour chaque tuile mémorisée dans memorizedTiles son indice
     * \details -1 si aucune tuile n'est mémorisée à cet emplacement dans memorizedTiles
     * Taille : memorySize.
     * \~english \brief Buffer precising for each memorized tile's indice
     * \details -1 if no tile is memorized into this place in memorizedTiles
     * Size : memorySize
     */
    int* memorizedIndex;
    
    /**
     * \~french \brief Mémorise la tuile demandée au format brut (sans compression)
     * \details Si la tuile est déjà mémorisée dans memorizedTiles (et on le sait grâce à memorizedIndex), on retourne directement l'adresse mémoire.
     * \return pointeur vers le tableau contenant la tuile voulue
     * \~english \brief Buffer precising for each memorized tile's indice
     * \return pointer to array containing the wanted tile
     */
    uint8_t* memorizeRawTile ( size_t& size, int tile );

    /**************************** Pour l'écriture ****************************/
    
    /**
     * \~french \brief Position du pointeur dans le fichier en cours d'écriture
     * \~english \brief Pointer position in opened written file
     */
    uint32_t position;

    /**
     * \~french \brief Adresse du début de chaque tuile dans le fichier
     * \~english \brief Tile's start address, in the file
     */
    uint32_t *tilesOffset;
    /**
     * \~french \brief Taille de chaque tuile dans le fichier
     * \~english \brief Tile's size, in the file
     */
    uint32_t *tilesByteCounts;

    std::ofstream output;  // tiff file output stream

    size_t BufferSize;
    uint8_t* Buffer, *zip_buffer;
    z_stream zstream;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    bool prepare();
    bool writeTile ( int tileInd, uint8_t *data, bool crop = false );
    bool close();
    size_t computeRawTile ( uint8_t *buffer, uint8_t *data );
    size_t computeJpegTile ( uint8_t *buffer, uint8_t *data, bool crop );
    void emptyWhiteBlock ( uint8_t *buffheight, int l );
    size_t computeLzwTile ( uint8_t *buffer, uint8_t *data );
    size_t computePackbitsTile ( uint8_t *buffer, uint8_t *data );
    size_t computePngTile ( uint8_t *buffer, uint8_t *data );
    size_t computeDeflateTile ( uint8_t *buffer, uint8_t *data );

protected:
    /** \~french
     * \brief Crée un objet Rok4Image à partir de tous ses éléments constitutifs
     * \details Ce constructeur est protégé afin de n'être appelé que par l'usine Rok4ImageFactory, qui fera différents tests et calculs.
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
     * \param[in] tileWidth largeur en pixel de la tuile
     * \param[in] tileHeight hauteur en pixel de la tuile
     ** \~english
     * \brief Create a Rok4Image object, from all attributes
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
     * \param[in] tileWidth tile's pixel width
     * \param[in] tileHeight tile's pixel height
     */
    Rok4Image (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, char* name,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
        Compression::eCompression compression, int tileWidth, int tileHeight
    );

public:

    /**
     * \~french
     * \brief Destructeur par défaut
     * \details Suppression des buffer de lecture et de l'interface TIFF
     * \~english
     * \brief Default destructor
     * \details We remove read buffer and TIFF interface
     */
    ~Rok4Image() {
        for ( int i = 0; i < memorySize; i++ ) if ( memorizedTiles[i] ) delete[] memorizedTiles[i];
        delete[] memorizedTiles;
        delete[] memorizedIndex;
    }

    /** \~french
     * \brief Sortie des informations sur l'image ROK4
     ** \~english
     * \brief ROK4 image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- Rok4Image ------------" );
        FileImage::print();
        LOGGER_INFO ( "\t- tile width = " << tileWidth << ", tile height = " << tileHeight );
        LOGGER_INFO ( "" );
    }

    /**************************** Pour la lecture ****************************/

    int getRawTile ( uint8_t* buf, int tile );
    int getEncodedTile ( uint8_t* buf, int tile );
    int getline ( uint8_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**************************** Pour l'écriture ****************************/

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture d'une image sont dans l'objet Rok4Image, sauf les données à écrire. On renseigne cela via une seconde image.
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn );

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture d'une image sont dans l'objet Rok4Image, sauf les données à écrire. On renseigne cela via une seconde image. Cette méthode permet également de préciser s'il on veut "croper". Dans le cas d'une compression JPEG, on peut vouloir "vider" les blocs contenant un pixel blanc.
     * \param[in] pIn source des donnée de l'image à écrire
     * \param[in] crop option de cropage, pour le jpeg
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn, bool crop );

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture par buffer au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint8_t* buffer ) {
        LOGGER_ERROR ( "Cannot write ROK4 image from a buffer" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture par buffer au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( float* buffer ) {
        LOGGER_ERROR ( "Cannot write ROK4 image from a buffer" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image ROK4, à partir d'un buffer d'entiers
     * \warning Pas d'implémentation de l'écriture par ligne au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint8_t* buffer, int line ) {
        LOGGER_ERROR ( "Cannot write ROK4 image line by line" );
        return -1;
    }

    /**
     * \~french
     * \brief Ecrit une ligne d'image ROK4, à partir d'un buffer de flottants
     * \warning Pas d'implémentation de l'écriture par ligne au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( float* buffer, int line) {
        LOGGER_ERROR ( "Cannot write ROK4 image line by line" );
        return -1;
    }
    
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Usine de création d'une image ROK4
 * \details Il est nécessaire de passer par cette classe pour créer des objets de la classe Rok4Image. Cela permet de réaliser quelques tests en amont de l'appel au constructeur de Rok4Image et de sortir en erreur en cas de problème. Dans le cas d'une image pour la lecture, on récupère dans le fichier toutes les méta-informations sur l'image. Pour l'écriture, on doit tout préciser afin de constituer l'en-tête TIFF.
 */
class Rok4ImageFactory {
public:
    /** \~french
     * \brief Crée un objet Rok4Image, pour la lecture
     * \details On considère que les informations d'emprise et de résolutions ne sont pas présentes dans le TIFF, on les précise donc à l'usine. Tout le reste sera lu dans les en-têtes TIFF. On vérifiera aussi la cohérence entre les emprise et résolutions fournies et les dimensions récupérées dans le fichier TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement.
     * Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     *
     * \param[in] filename chemin du fichier image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \return un pointeur d'objet Rok4Image, NULL en cas d'erreur
     ** \~english
     * \brief Create an Rok4Image object, for reading
     * \details Bbox and resolutions are not present in the TIFF file, so we precise them. All other informations are extracted from TIFF header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation.
     * Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] filename path to image file
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \return a Rok4Image object pointer, NULL if error
     */
    Rok4Image* createRok4ImageToRead ( char* filename, BoundingBox<double> bbox, double resx, double resy );

    /** \~french
     * \brief Crée un objet Rok4Image, pour l'écriture
     * \details Toutes les méta-informations sur l'image doivent être précisées pour écrire l'en-tête TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement.
     * Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
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
     * \param[in] tileWidth largeur en pixel de la tuile
     * \param[in] tileHeight hauteur en pixel de la tuile
     * \return un pointeur d'objet Rok4Image, NULL en cas d'erreur
     ** \~english
     * \brief Create a Rok4Image object, for writting
     * \details All informations have to be provided to be written in the TIFF header.
     *
     * Negative resolutions leads to georeferencement calculation.
     * Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
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
     * \param[in] tileWidth tile's pixel width
     * \param[in] tileHeight tile's pixel height
     * \return a Rok4Image object pointer, NULL if error
     */
    Rok4Image* createRok4ImageToWrite (
        char* filename, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
        Compression::eCompression compression, int tileWidth, int tileHeight
    );
};


#endif

