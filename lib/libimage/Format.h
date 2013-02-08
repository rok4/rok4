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

#ifndef FORMAT_H
#define FORMAT_H

#include <string>
#include <stdint.h>
#include "tiff.h"

//To declare a new format change the implementation too

namespace Format {

    enum eformat_data {
        UNKNOWN = 0,
        TIFF_RAW_INT8 = 1,
        TIFF_JPG_INT8 = 2,
        TIFF_PNG_INT8 = 3,
        TIFF_LZW_INT8 = 4,
        TIFF_ZIP_INT8 = 5,
        TIFF_PKB_INT8 = 6,
        TIFF_RAW_FLOAT32 = 7,
        TIFF_LZW_FLOAT32 = 8,
        TIFF_ZIP_FLOAT32 = 9,
        TIFF_PKB_FLOAT32 = 10
    };
    
    eformat_data fromString(std::string strFormat);
    std::string toString(eformat_data format);
    std::string toMimeType(eformat_data format);
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
        uint16_t bitspersample;
        uint16_t sampleformat;

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
        SampleType(uint16_t bitspersample, uint16_t sampleformat) : bitspersample(bitspersample), sampleformat(sampleformat) {}

        /**
         * \~french
         * \brief Retourne le nombre de bits par canal
         * \return nombre de bits par canal
         * \~english
         * \brief Return the number of bits per sample
         * \return number of bits per sample
         */
        uint16_t getBitsPerSample() {return bitspersample;}

        /**
         * \~french
         * \brief Retourne le format des canaux
         * \return format des canaux
         * \~english
         * \brief Return the samples' format
         * \return samples' format
         */
        uint16_t getSampleFormat() {return sampleformat;}

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
        static string getHandledFormat() {
            return  "\t - 8-bit unsigned integer\n" <<
                     "\t - 32-bit float\n";
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
            return  (bitspersample == 32 && sampleformat == SAMPLEFORMAT_UINT) ||
                    (bitspersample == 8 && sampleformat == SAMPLEFORMAT_IEEEFP) ;
        }
};

#endif //FORMAT_H

