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
#include "Context.h"
#include "StoreDataSource.h"

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
class Rok4Image : public Image {

    friend class Rok4ImageFactory;

private:

    /**
     * \~french \brief Nom de l'image
     * \~english \brief image's name
     */
    std::string name;
    /**
     * \~french \brief Photométrie des données (rgb, gray...)
     * \~english \brief Data photometric (rgb, gray...)
     */
    Photometric::ePhotometric photometric;
    /**
     * \~french \brief type de l'éventuel canal supplémentaire
     * \details En écriture ou dans les traitements, on considère que les canaux ne sont pas prémultipliés par la valeur d'alpha.
     * En lecture, on accepte des images pour lesquelles l'alpha est associé. On doit donc mémoriser cette information et convertir à la volée lors de la lecture des données.
     * \~english \brief extra sample type (if exists)
     */
    ExtraSample::eExtraSample esType;
    /**
     * \~french \brief Compression des données (jpeg, packbits...)
     * \~english \brief Data compression (jpeg, packbits...)
     */
    Compression::eCompression compression;
    /**
     * \~french \brief Format des canaux
     * \~english \brief Sample format
     */
    SampleFormat::eSampleFormat sampleformat;
    /**
     * \~french \brief Nombre de bits par canal
     * \~english \brief Number of bits per sample
     */
    int bitspersample;
    
    /**
     * \~french \brief Taille d'un pixel en octet
     * \~english \brief Byte pixel's size
     */
    int pixelSize;

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
    

    /**
     * \~french \brief Contexte de stockage de l'image ROK4
     * \~english \brief Image's storage context
     */    
    Context* context;

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

    /**
     * \~french \brief Taille du buffer #Buffer temporaire contenant la tuile à écrire (dans writeTile), compressée
     * \~english \brief Temporary buffer #Buffer size, containing the compressed tile to write
     */
    size_t BufferSize;
    
    /**
     * \~french \brief Buffer temporaire contenant la tuile à écrire (dans writeTile), compressée
     * \~english \brief Buffer size, containing the compressed tile to write
     */
    uint8_t* Buffer;

    /**
     * \~french \brief Buffer utilisé par la zlib
     * \details Pour les compressions PNG et DEFLATE uniquement
     * \~english \brief Buffer used by zlib
     */
    uint8_t* zip_buffer;
    /**
     * \~french \brief Flux utilisé par la zlib
     * \details Pour les compressions PNG et DEFLATE uniquement
     * \~english \brief Stream used by zlib
     */
    z_stream zstream;
    
    /**
     * \~french \brief Structure d'informations, utilisée par la libjpeg
     * \details Pour la compression JPEG uniquement
     * \~english \brief Informations structure used by libjpeg
     */
    struct jpeg_compress_struct cinfo;
    /**
     * \~french \brief Structure d'erreur utilisée par la libjpeg
     * \details Pour la compression JPEG uniquement
     * \~english \brief Error structure used by libjpeg
     */
    struct jpeg_error_mgr jerr;


    /**
     * \~french \brief Charge l'index des tuiles de l'image ROK4 à lire
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Load index of ROK4 image to read
     * \return TRUE if success, FALSE otherwise
     */
    bool loadIndex();

    /**
     * \~french \brief Écrit l'en-tête TIFF de l'image ROK4
     * \details L'en-tête est de taille fixe (ROK4_IMAGE_HEADER_SIZE) et contient toutes les métadonnées sur l'image. Elle ne sera pas lue par le serveur ROK4 (c'est pourquoi sa taille doit être fixe), mais permet de lire l'image avec un logiciel autre (avoir une image TIFF respectant les spécifications).
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Write the ROK4 image's TIFF header
     * \return TRUE if success, FALSE otherwise
     */
    bool writeHeader();
    /**
     * \~french \brief Finalise l'écriture de l'image ROK4
     * \details Cela comprend l'écriture des index et tailles des tuiles
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief End the ROK4 image's writting
     * \return TRUE if success, FALSE otherwise
     */
    bool writeFinal();

    /**
     * \~french \brief Prépare les buffers pour les éventuelles compressions
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Prepare buffers for compressions
     * \return TRUE if success, FALSE otherwise
     */
    bool prepareBuffers();
    /**
     * \~french \brief Nettoie les buffers de fonctionnement
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Clean temporary buffers
     * \return TRUE if success, FALSE otherwise
     */
    bool cleanBuffers();

    /**
     * \~french \brief Écrit une tuile de l'image ROK4
     * \details L'écriture tiendra compte de la compression voulue #compression. Les tuiles doivent être écrites dans l'ordre (de gauche à droite, de haut en bas).
     * \param[in] tileInd indice de la tuile à écrire
     * \param[in] data données brutes (sans compression) à écrire
     * \param[in] crop option pour le jpeg (voir #emptyWhiteBlock)
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Write a ROK4 image's tile
     * \param[in] tileInd tile indice
     * \param[in] data raw data (no compression) to write
     * \param[in] crop jpeg option (see #emptyWhiteBlock)
     * \return TRUE if success, FALSE otherwise
     */
    bool writeTile ( int tileInd, uint8_t *data, bool crop = false );

    /**
     * \~french \brief Écrit une tuile indépendante en tant qu'objet Ceph
     * \details Le nom de l'objet/tuile sera #name _ col _ row
     * \param[in] tileCol colonne de la tuile à écrire
     * \param[in] tileRow ligne de la tuile à écrire
     * \param[in] data données brutes (sans compression) à écrire
     * \param[in] crop option pour le jpeg (voir #emptyWhiteBlock)
     * \return VRAI en cas de succès, FAUX sinon
     * \~english \brief Write a ROK4 tile as a ceph object
     * \param[in] tileCol tile column
     * \param[in] tileRow tile row
     * \param[in] data raw data (no compression) to write
     * \param[in] crop jpeg option (see #emptyWhiteBlock)
     * \return TRUE if success, FALSE otherwise
     */
    bool writeTile( int tileCol, int tileRow, uint8_t* data, bool crop );


    /**
     * \~french \brief Compresse les données brutes en RAW
     * \details Consiste en une simple copie.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into RAW compression
     * \details A simple copy
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t computeRawTile ( uint8_t *buffer, uint8_t *data );

     /**
     * \~french \brief Compresse les données brutes en JPEG
     * \details Utilise la libjpeg.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \param[in] crop option pour le jpeg (voir #writeImage)
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into JPEG compression
     * \details Use libjpeg
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \param[in] crop jpeg option (see #writeImage)
     * \return data' size in buffer, 0 if failure
     */
    size_t computeJpegTile ( uint8_t *buffer, uint8_t *data, bool crop );

    /**
     * \~french \brief Remplit les blocs qui contiennent un pixel blanc de blanc
     * \details Le JPEG  utilise des blocs de 16 sur 16 pour compresser. Si le blanc est réservé pour le nodata, et qu'on ne veut pas qu'il soit "sali" lors de la compression, on doit identifier les blocs contenant du blanc (nodata) et les remplir de cette couleur.
     * \param[in,out] buffer données brutes à croper
     * \param[in] l nombre de ligne de la tuile (buffer) à considérer (16 ou moins quand le bas de la tuile est atteint)
     * \~english \brief Fill blocs, which contains a white pixel, with white
     * \param[in,out] buffer raw data to crop
     * \param[in] l number of tile's line (in buffer) to consider (16 or less when tile's bottom is reached)
     */
    void emptyWhiteBlock ( uint8_t *buffer, int l );
    
    /**
     * \~french \brief Compresse les données brutes en LZW
     * \details Utilise la liblzw.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into LZW compression
     * \details Use liblzw.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t computeLzwTile ( uint8_t *buffer, uint8_t *data );
    /**
     * \~french \brief Compresse les données brutes en PACKBITS
     * \details Utilise la libpkb.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into PACKBITS compression
     * \details Use libpkb.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t computePackbitsTile ( uint8_t *buffer, uint8_t *data );
    /**
     * \~french \brief Compresse les données brutes en PNG
     * \details Utilise la zlib. Les données retournées contiennent l'en-tête PNG.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into PNG compression
     * \details Use zlib. Returned data contains PNG header.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t computePngTile ( uint8_t *buffer, uint8_t *data );
    /**
     * \~french \brief Compresse les données brutes en DEFLATE
     * \details Utilise la zlib.
     * \param[out] buffer buffer de stockage des données compressées. Doit être alloué.
     * \param[in] data données brutes (sans compression) à compresser
     * \return taille utile du buffer, 0 si erreur
     * \~english \brief Compress raw data into DEFLATE compression
     * \details Use zlib.
     * \param[out] buffer Storage buffer for compressed data. Have to be allocated.
     * \param[in] data raw data (no compression) to write
     * \return data' size in buffer, 0 if failure
     */
    size_t computeDeflateTile ( uint8_t *buffer, uint8_t *data );
    
    template<typename T>
    int _getline ( T* buffer, int line );

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
     * \param[in] esType type du canal supplémentaire, si présent.
     * \param[in] tileWidth largeur en pixel de la tuile
     * \param[in] tileHeight hauteur en pixel de la tuile
     * \param[in] context contexte de stockage
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
     * \param[in] esType extra sample type
     * \param[in] tileWidth tile's pixel width
     * \param[in] tileHeight tile's pixel height
     * \param[in] context storage's context
     */
    Rok4Image (
        int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, std::string name,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression, ExtraSample::eExtraSample es,
        int tileWidth, int tileHeight,
        Context* context
    );

public:
    
    static bool canRead ( int bps, SampleFormat::eSampleFormat sf) {
        return ( 
            ( bps == 32 && sf == SampleFormat::FLOAT ) || 
            ( bps == 16 && sf == SampleFormat::UINT ) || 
            ( bps == 8 && sf == SampleFormat::UINT )
        );
    }
    
    static bool canWrite ( int bps, SampleFormat::eSampleFormat sf) {
        return ( 
            ( bps == 32 && sf == SampleFormat::FLOAT ) || 
            ( bps == 16 && sf == SampleFormat::UINT ) || 
            ( bps == 8 && sf == SampleFormat::UINT )
        );
    }

    
    /**
     * \~french
     * \brief Retourne le type du canal supplémentaire
     * \return esType
     * \~english
     * \brief Return extra sample type
     * \return esType
     */
    inline ExtraSample::eExtraSample getExtraSample() {
        return esType;
    }
    
    /**
     * \~french
     * \brief Modifie le type du canal supplémentaire
     * \~english
     * \brief Modify extra sample type
     */
    inline void setExtraSample(ExtraSample::eExtraSample es) {
        esType = es;
    }
    /**
     * \~french
     * \brief Retourne la compression des données
     * \return compression
     * \~english
     * \brief Return data compression
     * \return compression
     */
    inline Compression::eCompression getCompression() {
        return compression;
    }

    /**
     * \~french
     * \brief Retourne le format des canaux (entier, flottant)
     * \return format des canaux
     * \~english
     * \brief Return sample format (integer, float)
     * \return sample format
     */
    inline SampleFormat::eSampleFormat getSampleFormat() {
        return sampleformat;
    }

    /**
     * \~french
     * \brief Retourne le nombre de bits par canal
     * \return nombre de bits par canal
     * \~english
     * \brief Return number of bits per sample
     * \return number of bits per sample
     */
    inline int getBitsPerSample() {
        return bitspersample;
    }
    
    /**
     * \~french
     * \brief Retourne la photométrie des données image (rgb, gray...)
     * \return photométrie
     * \~english
     * \brief Return data photometric (rgb, gray...)
     * \return photometric
     */
    inline Photometric::ePhotometric getPhotometric() {
        return photometric;
    }

    /**
     * \~french
     * \brief Retourne la taille d'une tuile brute (décompessée)
     * \~english
     * \brief Return raw tile size (uncompressed)
     */
    int getRawTileSize() {
        return rawTileSize;
    }

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
        delete[] tilesOffset;
        delete[] tilesByteCounts;
    }


    /** \~french
     * \brief Sortie des informations sur l'image ROK4
     ** \~english
     * \brief ROK4 image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- Rok4Image ------------" );
        Image::print();
        LOGGER_INFO ( "\t- Compression : " << Compression::toString ( compression ) );
        LOGGER_INFO ( "\t- Photometric : " << Photometric::toString ( photometric ) );
        LOGGER_INFO ( "\t- Bits per sample : " << bitspersample );
        LOGGER_INFO ( "\t- Sample format : " << SampleFormat::toString ( sampleformat ) );
        LOGGER_INFO ( "\t- tile width = " << tileWidth << ", tile height = " << tileHeight );
        LOGGER_INFO ( "\t- Image name : " << name );
        LOGGER_INFO ( "" );
    }

    /**************************** Pour la lecture ****************************/
    /**
     * \~french
     * \brief Retourne une tuile décompressée
     * \param[out] buf buffer contenant la tuile. Doit être alloué.
     * \param[in] line Indice de la tuile à retourner (0 <= tile < tilesNumber)
     * \return taille utile du buffer, 0 si erreur
     */
    int getRawTile ( uint8_t* buf, int tile );
    /**
     * \~french
     * \brief Retourne une tuile compressée
     * \param[out] buf buffer contenant la tuile. Doit être alloué.
     * \param[in] line Indice de la tuile à retourner (0 <= tile < tilesNumber)
     * \return taille utile du buffer, 0 si erreur
     */
    int getEncodedTile ( uint8_t* buf, int tile );
    
    int getline ( uint8_t* buffer, int line );
    int getline ( uint16_t* buffer, int line );
    int getline ( float* buffer, int line );

    /**************************** Pour l'écriture ****************************/

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture d'une image sont dans l'objet Rok4Image, sauf les données à écrire. On renseigne cela via une seconde image.
     * \param[in] pIn source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn ) {
        return writeImage(pIn, false);
    }

    /**
     * \~french
     * \brief Ecrit une image ROK4, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture d'une image sont dans l'objet Rok4Image, sauf les données à écrire. On renseigne cela via une seconde image. Cette méthode permet également de préciser s'il on veut "croper". Dans le cas d'une compression JPEG, on peut vouloir "vider" les blocs (16x16 pixels) contenant un pixel blanc.
     * \param[in] pIn source des donnée de l'image à écrire
     * \param[in] crop option de cropage, pour le jpeg
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( Image* pIn, bool crop );

    /**
     * \~french
     * \brief Ecrit les tuiles indépendantes sur un cluster Ceph, à partir d'une image source
     * \details Toutes les informations nécessaires à l'écriture des tuiles sont dans l'objet Rok4Image, sauf les données à écrire. On renseigne cela via une seconde image.
     *
     * On précise également les indices (colonne et ligne) de la dalle, ce qui permet de calculer les indices de chaque tuile,  on utilise alors le nom de l'image comme préfixe, et on peut nommer les objets Ceph (<NAME>_<TILECOL>_<TILROW>).
     *
     * Exemple :
     * \li nom : EXEMPLE
     * \li colonne = 4 et ligne = 7
     * \li nombre de tuile dans la largeur et dans la hauteur : 2
     * \li writeTiles entraînera l'écriture de 4 objets sur Ceph (tuiles) : EXEMPLE_8_14, EXEMPLE_8_15, EXEMPLE_9_14, EXEMPLE_9_15
     *
     * Cette méthode permet également de préciser s'il on veut "croper". Dans le cas d'une compression JPEG, on peut vouloir "vider" les blocs (16x16 pixels) contenant un pixel blanc.     
     *
     * \param[in] pIn source des donnée de l'image à écrire
     * \param[in] imageCol option de cropage, pour le jpeg
     * \param[in] imageRow option de cropage, pour le jpeg
     * \param[in] crop option de cropage, pour le jpeg
     * \return 0 en cas de succes, -1 sinon
     */
    int writeTiles ( Image* pIn, int imageCol, int imageRow, bool crop );

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
     * \brief Ecrit une image ROK4, à partir d'un buffer d'entiers 16 bits
     * \warning Pas d'implémentation de l'écriture par buffer au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeImage ( uint16_t* buffer ) {
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
     * \brief Ecrit une ligne d'image ROK4, à partir d'un buffer d'entiers 16 bits
     * \warning Pas d'implémentation de l'écriture par ligne au format ROK4, retourne systématiquement une erreur
     * \param[in] buffer source des donnée de l'image à écrire
     * \param[in] line ligne de l'image à écrire
     * \return 0 en cas de succes, -1 sinon
     */
    int writeLine ( uint16_t* buffer, int line) {
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
     * \param[in] name nom de l'image
     * \param[in] bbox emprise rectangulaire de l'image
     * \param[in] resx résolution dans le sens des X.
     * \param[in] resy résolution dans le sens des Y.
     * \param[in] contexte de stockage (fichier, objet ceph ou objet swift)
     * \return un pointeur d'objet Rok4Image, NULL en cas d'erreur
     ** \~english
     * \brief Create an Rok4Image object, for reading
     * \details Bbox and resolutions are not present in the TIFF file, so we precise them. All other informations are extracted from TIFF header. We have to check consistency between provided bbox and resolutions and read image's dimensions.
     *
     * Negative resolutions leads to georeferencement calculation.
     * Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] name image's name
     * \param[in] bbox bounding box
     * \param[in] resx X wise resolution.
     * \param[in] resy Y wise resolution.
     * \param[in] context storage context (file, ceph object or swift object)
     * \return a Rok4Image object pointer, NULL if error
     */
    Rok4Image* createRok4ImageToRead ( std::string name, BoundingBox<double> bbox, double resx, double resy, Context* context );

    /** \~french
     * \brief Crée un objet Rok4Image, pour l'écriture
     * \details Toutes les méta-informations sur l'image doivent être précisées pour écrire l'en-tête TIFF.
     *
     * Si les résolutions fournies sont négatives, cela signifie que l'on doit calculer un géoréférencement.
     * Dans ce cas, on prend des résolutions égales à 1 et une bounding box à (0,0,width,height).
     * \param[in] name nom de l'image
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
     * \param[in] contexte de stockage (fichier, objet ceph ou objet swift)
     * \return un pointeur d'objet Rok4Image, NULL en cas d'erreur
     ** \~english
     * \brief Create a Rok4Image object, for writting
     * \details All informations have to be provided to be written in the TIFF header.
     *
     * Negative resolutions leads to georeferencement calculation.
     * Both resolutions will be equals to 1 and the bounding box will be (0,0,width,height).
     * \param[in] name image's name
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
     * \param[in] context storage context (file, ceph object or swift object)
     * \return a Rok4Image object pointer, NULL if error
     */
    Rok4Image* createRok4ImageToWrite (
        std::string name, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
        SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
        Compression::eCompression compression, int tileWidth, int tileHeight, Context* context
    );
};


#endif

