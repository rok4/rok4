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
 * \file Format.h
 ** \~french
 * \brief Définition de la classe SampleType des namespaces Compression et Format
 * \details
 * \li SampleType : gère les types de canaux acceptés par les classes d'Image
 * \li Compression : énumère et manipule les différentes compressions
 * \li Format : énumère et manipule les différents format d'image
 ** \~english
 * \brief Define class SampleType and the namespaces Compression and Format
 * \details
 * \li SampleType : managed sample type accepted by Image classes
 * \li Compression : enumerate and managed different compressions
 * \li Format : enumerate and managed different formats
 */

#ifndef FORMAT_H
#define FORMAT_H

#include <string>
#include <stdint.h>
#include "tiff.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées au format de canal
 * \~english \brief Manage informations in connection with sample format
 */
namespace SampleFormat {
/**
 * \~french \brief Énumération des formats de canal disponibles
 * \~english \brief Available sample formats enumeration
 */
enum eSampleFormat {
    UNKNOWN = 0,
    UINT = 1,
    FLOAT = 2
};

/**
 * \~french \brief Nombre de formats de canal disponibles
 * \~english \brief Number of sample formats compressions
 */
const int sampleformat_size = 2;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une format du canal de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return la format du canal correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a sample formats enumeration member
 * \param[in] strComp string to convert
 * \return the binding sample format, UNKNOWN (0) if string is not recognized
 */
eSampleFormat fromString ( std::string strPh );

/**
 * \~french \brief Conversion d'une format du canal vers une chaîne de caractères
 * \param[in] comp format du canal à convertir
 * \return la chaîne de caractère nommant la format du canal
 * \~english \brief Convert a sample format to a string
 * \param[in] comp sample format to convert
 * \return string namming the sample format
 */
std::string toString ( eSampleFormat ph );

}






/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées à la compression
 * \~english \brief Manage informations in connection with compression
 */
namespace Compression {
/**
 * \~french \brief Énumération des compressions disponibles
 * \~english \brief Available compressions enumeration
 */
enum eCompression {
    UNKNOWN = 0,
    NONE = 1,
    DEFLATE = 2,
    JPEG = 3,
    PNG = 4,
    LZW = 5,
    PACKBITS = 6
};

/**
 * \~french \brief Nombre de compressions disponibles
 * \~english \brief Number of available compressions
 */
const int compression_size = 6;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une compression de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return la compression correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a compressions enumeration member
 * \param[in] strComp string to convert
 * \return the binding compression, UNKNOWN (0) if string is not recognized
 */
eCompression fromString ( std::string strComp );

/**
 * \~french \brief Conversion d'une compression vers une chaîne de caractères
 * \param[in] comp compression à convertir
 * \return la chaîne de caractère nommant la compression
 * \~english \brief Convert a compression to a string
 * \param[in] comp compression to convert
 * \return string namming the compression
 */
std::string toString ( eCompression comp );

}






/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées à la photométrie
 * \~english \brief Manage informations in connection with photometric
 */
namespace Photometric {
/**
 * \~french \brief Énumération des photométries disponibles
 * \~english \brief Available photometrics enumeration
 */
enum ePhotometric {
    UNKNOWN = 0,
    GRAY = 1,
    RGB = 2,
    MASK = 3
};

/**
 * \~french \brief Nombre de photométries disponibles
 * \~english \brief Number of photometrics compressions
 */
const int photometric_size = 3;

/**
 * \~french \brief Conversion d'une chaîne de caractères vers une photométrie de l'énumération
 * \param[in] strComp chaîne de caractère à convertir
 * \return la photométrie correspondante, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a photometrics enumeration member
 * \param[in] strComp string to convert
 * \return the binding photometric, UNKNOWN (0) if string is not recognized
 */
ePhotometric fromString ( std::string strPh );

/**
 * \~french \brief Conversion d'une photométrie vers une chaîne de caractères
 * \param[in] comp photométrie à convertir
 * \return la chaîne de caractère nommant la photométrie
 * \~english \brief Convert a photometric to a string
 * \param[in] comp photometric to convert
 * \return string namming the photometric
 */
std::string toString ( ePhotometric ph );

}






/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Gestion des types des canaux supportés
 * \details Un type correspond à :
 * \li un nombre de bits par canal
 * \li le format du canal : entier, flottant, signé ou non...
 *
 * Sont supportés par la librairie image
 * \li les entiers non-signés sur 8 bits
 * \li les flottants sur 32 bits
 */
class SampleType {
private :
    /**
     * \~french \brief Nombre de bits occupé par le canal
     * \~english \brief Number of bits used by the sample
     */
    int bitspersample;

    /**
     * \~french \brief Format du canal : entier, flottant, signé ou non...
     * \~english \brief Sample format : integer, float, signed or not...
     */
    SampleFormat::eSampleFormat sampleformat;

public:

    /** \~french
     * \brief Crée un objet SampleType à partir de tous ses éléments constitutifs
     * \param[in] bitspersample nombre de bits par canal
     * \param[in] sampleformat format des canaux
     ** \~english
     * \brief Create an SampleType object, from all attributes
     * \param[in] bitspersample number of bits per sample
     * \param[in] sampleformat sample's format
     */
    SampleType ( int bitspersample, SampleFormat::eSampleFormat sampleformat ) : bitspersample ( bitspersample ), sampleformat ( sampleformat ) {}

    /** \~french
     * \brief Constructeur vide
     ** \~english
     * \brief Empty constructor
     */
    SampleType () {}

    /**
     * \~french
     * \brief Retourne le nombre de bits par canal
     * \return nombre de bits par canal
     * \~english
     * \brief Return the number of bits per sample
     * \return number of bits per sample
     */
    int getBitsPerSample() {
        return bitspersample;
    }

    /**
     * \~french
     * \brief Retourne le format des canaux
     * \return format des canaux
     * \~english
     * \brief Return the samples' format
     * \return samples' format
     */
    SampleFormat::eSampleFormat getSampleFormat() {
        return sampleformat;
    }

    /**
     * \~french
     * \brief Précise si le type correspondant est uint8_t
     * \~english
     * \brief Precise if corresponding type is uint8_t
     */
    bool isUInt8() {
        return ( bitspersample == 8 && sampleformat == SampleFormat::UINT );
    }

    /**
     * \~french
     * \brief Précise si le type correspondant est float
     * \~english
     * \brief Precise if corresponding type is float
     */
    bool isFloat() {
        return ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT );
    }

    /**
     * \~french
     * \brief Précise si le type des canaux (nombre de bits et format) est géré
     * \details Sont gérés :
     * \li les entiers non-signés sur 8 bits
     * \li les flottants sur 32 bits
     * \~english
     * \brief Precises if sample type (bits per sample and format) is supported
     * \details Are supported :
     * \li 8-bit unsigned integer
     * \li 32-bit float
     */
    bool isSupported() {
        return ( bitspersample == 8 && sampleformat == SAMPLEFORMAT_UINT ) ||
               ( bitspersample == 32 && sampleformat == SAMPLEFORMAT_IEEEFP ) ;
    }

    /**
     * \~french
     * \brief Renvoie la liste des types gérés
     * \~english
     * \brief Return the handled type list
     */
    static std::string getHandledFormat() {
        return  "\t - 8-bit unsigned integer\n\t - 32-bit float\n";
    }
};





/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion des informations liées au format des données
 * \~english \brief Manage informations in connection with data format
 */
namespace Format {

/**
 * \~french \brief Énumération des formats d'images disponibles
 * \~english \brief Available images formats enumeration
 */
enum eformat_data {
    UNKNOWN = 0,
    // Les formats entiers en premier
    TIFF_RAW_INT8 = 1,
    TIFF_JPG_INT8 = 2,
    TIFF_PNG_INT8 = 3,
    TIFF_LZW_INT8 = 4,
    TIFF_ZIP_INT8 = 5,
    TIFF_PKB_INT8 = 6,
    // Les formats flottant doivent bien être à partir d'ici (et corriger eformat_float si le nombre de format entier augmente)
    TIFF_RAW_FLOAT32 = 7,
    TIFF_LZW_FLOAT32 = 8,
    TIFF_ZIP_FLOAT32 = 9,
    TIFF_PKB_FLOAT32 = 10
};

/**
 * \~french \brief Nombre de formats disponibles
 * \~english \brief Number of available formats
 */
const int eformat_size = 10;

/**
 * \~french \brief Indice du premier format flottant dans l'énumération
 * \~english \brief First float format indice into enumeration
 */
const int eformat_float = 7;

/**
 * \~french \brief Conversion d'une chaîne de caractère vers un format
 * \param[in] strFormat chaîne de caractère à convertir
 * \return le format correspondant, UNKNOWN (0) si la chaîne n'est pas reconnue
 * \~english \brief Convert a string to a format
 * \param[in] strFormat string to convert
 * \return the binding format, UNKNOWN (0) if string is not recognized
 */
eformat_data fromString ( std::string strFormat );

/**
 * \~french \brief Conversion d'un format vers une chaîne de caractère
 * \param[in] format format à convertir
 * \return la chaîne de caractère nommant le format
 * \~english \brief Convert a format to a string
 * \param[in] format format to convert
 * \return string namming the format
 */
std::string toString ( eformat_data format );

/**
 * \~french \brief Conversion d'un format vers une chaîne de caractère (type MIME)
 * \param[in] format format à convertir
 * \return type MIME du format
 * \~english \brief Convert a format to a string (type MIME)
 * \param[in] format format to convert
 * \return MIME type of the format
 */
std::string toMimeType ( eformat_data format );

}

#endif //FORMAT_H

