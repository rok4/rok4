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
 * \file PixelConverter.h
 * \~french
 * \brief Définition de la classe PixelConverter, permettant de convertir à la volée les pixels
 * \~english
 * \brief Define the PixelConverter class
 */

#ifndef PIXELCONVERTER_H
#define PIXELCONVERTER_H

#include <string.h>
#include <stdio.h>
#include <Logger.h>
#include <Utils.h>
#include "Format.h"

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief PixelConverter
 ** \~english
 * \brief PixelConverter
 */
class PixelConverter {

private:
    /**
     * \~french \brief Canaux de couleur, sans tenir compte de l'alpha
     * \~english \brief Color's samples, ignoring alpha
     */

    SampleFormat::eSampleFormat inSampleFormat;
    SampleFormat::eSampleFormat outSampleFormat;

    int inBitsPerSample;
    int outBitsPerSample;

    int inSamplesPerPixel;
    int outSamplesPerPixel;

    int width;

    bool yesWeCan;

public:
    /** \~french
     * \brief Crée un objet PixelConverter à partir de la largeur
     ** \~english
     * \brief Create a PixelConverter, from the width
     */
    PixelConverter ( int w, SampleFormat::eSampleFormat isf, int ibps, int ispp, SampleFormat::eSampleFormat osf, int obps, int ospp ) : 
        width(w), inSampleFormat (isf), inBitsPerSample(ibps), inSamplesPerPixel(ispp),
        outSampleFormat (osf), outBitsPerSample(obps), outSamplesPerPixel(ospp) 
    {
        yesWeCan = false;
        
        if (inSampleFormat == SampleFormat::FLOAT || outSampleFormat == SampleFormat::FLOAT) {
            LOGGER_WARN("PixelConverter doesn't handle float samples");
            return;
        }
        if (inSampleFormat != outSampleFormat) {
            LOGGER_WARN("PixelConverter doesn't handle different samples format");
            return;
        }
        if (inBitsPerSample != outBitsPerSample) {
            LOGGER_WARN("PixelConverter doesn't handle different number of bits per sample");
            return;
        }

        if (inSamplesPerPixel == outSamplesPerPixel) {
            LOGGER_WARN("PixelConverter have not to be used if number of samples per pixel is the same");
            return;
        }

        if (inBitsPerSample != 8) {
            LOGGER_WARN("PixelConverter only handle 8 bits sample");
            return;
        }

        yesWeCan = true;
    }

    bool youCan () {
        return yesWeCan;
    }

    SampleFormat::eSampleFormat getSampleFormat () {
        return outSampleFormat;
    }

    int getBitsPerSample () {
        return outBitsPerSample;
    }

    int getSamplesPerPixel () {
        return outSamplesPerPixel;
    }

    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "---------- PixelConverter ------------" );
        LOGGER_INFO ( "\t- Width : " << width );
        LOGGER_INFO ( "\t- SampleFormat : " << SampleFormat::toString(inSampleFormat) << " -> " << SampleFormat::toString(outSampleFormat) );
        LOGGER_INFO ( "\t- Bits per sample : " << inBitsPerSample << " -> " << outBitsPerSample );
        LOGGER_INFO ( "\t- Samples per pixel : " << inSamplesPerPixel << " -> " << outSamplesPerPixel );
        LOGGER_INFO ( "" );
    }

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~PixelConverter() {
    }

    template<typename T>
    void convertLine ( T* bufferto, T* bufferfrom ) {

        T defaultAlpha;
        if (sizeof(T) == 1) {
            defaultAlpha = (T) 255;
        } else {
            defaultAlpha = (T) 1;
        }


        /********************** Depuis 1 canal ********************/
        if ( inSamplesPerPixel == 1) {
            if (outSamplesPerPixel == 2) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[2*i] = bufferfrom[i];
                    bufferto[2*i+1] = defaultAlpha;
                }
                return;
            }
            if (outSamplesPerPixel == 3) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[3*i] = bufferto[3*i+1] = bufferto[3*i+2] = bufferfrom[i];
                }
                return;
            }
            if (outSamplesPerPixel == 4) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[4*i] = bufferto[4*i+1] = bufferto[4*i+2] = bufferfrom[i];
                    bufferto[4*i+3] = defaultAlpha;
                }
                return;
            }
        }
        
        /********************** Depuis 2 canaux *******************/
        if ( inSamplesPerPixel == 2) {
            if (outSamplesPerPixel == 1) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[i] = bufferfrom[2*i];
                }
                return;
            }
            if (outSamplesPerPixel == 3) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[3*i] = bufferto[3*i+1] = bufferto[3*i+2] = bufferfrom[2*i];
                }
                return;
            }
            if (outSamplesPerPixel == 4) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[4*i] = bufferto[4*i+1] = bufferto[4*i+2] = bufferfrom[2*i];
                    bufferto[4*i+3] = bufferfrom[2*i + 1];
                }
                return;
            }
        }
        
        /********************** Depuis 3 canaux *******************/
        if ( inSamplesPerPixel == 3) {
            if (outSamplesPerPixel == 1) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[i] = ( T ) (0.2125*bufferfrom[3*i] + 0.7154*bufferfrom[3*i+1] + 0.0721*bufferfrom[3*i+2]);
                }
                return;
            }
            if (outSamplesPerPixel == 2) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[2*i] = ( T ) (0.2125*bufferfrom[3*i] + 0.7154*bufferfrom[3*i+1] + 0.0721*bufferfrom[3*i+2]);
                    bufferto[2*i+1] = defaultAlpha;
                }
                return;
            }
            if (outSamplesPerPixel == 4) {
                for ( int i = 0; i < width; i++ ) {
                    memcpy(bufferto + 4*i, bufferfrom + 3*i, 3 * sizeof(T));
                    bufferto[4*i+3] = defaultAlpha;
                }
                return;
            }
        }
        
        /********************** Depuis 4 canaux *******************/
        if ( inSamplesPerPixel == 4) {
            if (outSamplesPerPixel == 1) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[i] = ( T ) (0.2125*bufferfrom[4*i] + 0.7154*bufferfrom[4*i+1] + 0.0721*bufferfrom[4*i+2]);
                }
                return;
            }
            if (outSamplesPerPixel == 2) {
                for ( int i = 0; i < width; i++ ) {
                    bufferto[2*i] = ( T ) (0.2125*bufferfrom[4*i] + 0.7154*bufferfrom[4*i+1] + 0.0721*bufferfrom[4*i+2]);
                    bufferto[2*i+1] = bufferfrom[4*i + 3];
                }
                return;
            }
            if (outSamplesPerPixel == 3) {
                for ( int i = 0; i < width; i++ ) {
                    memcpy(bufferto + 3*i, bufferfrom + 4*i, 3 * sizeof(T));
                }
                return;
            }
        }
    }
};


#endif
