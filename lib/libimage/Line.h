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
 * \file Line.h
 * \~french
 * \brief Définition de la classe Line, permettant la manipulation et la conversion de lignes d'image.
 * \~english
 * \brief Define the Line class , to manipulate and convert image's lines.
 */

#ifndef LINE_H
#define LINE_H

#include <string.h>
#include <stdio.h>
#include <Pixel.h>
#include <Logger.h>

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Représentation d'une ligne entière ou flottante, sur 4 canaux RGB + A, alpha prémultiplié.
 */
template<typename T>
class Line {

    public:
        T* samples;
        float* alpha;
        uint8_t* mask;
        int width;

        Line (int width) : width(width) {
            samples = new T[3*width];
            alpha = new float[width];
            mask = new uint8_t[width];
        }

        Line ( T* imageIn, uint8_t* maskIn, int srcSpp, int width, T* transparent) : width(width) {
            samples = new T[3*width];
            alpha = new float[width];
            mask = new uint8_t[width];
            store(imageIn, mask, srcSpp, transparent);
        }

        Line ( T* imageIn, uint8_t* maskIn, int srcSpp, int width) : width(width) {
            samples = new T[3*width];
            alpha = new float[width];
            mask = new uint8_t[width];
            store(imageIn, mask, srcSpp);
        }

        void store ( uint8_t* imageIn, uint8_t* maskIn, int srcSpp, uint8_t* transparent) {
            mask = maskIn;
            switch (srcSpp) {
                case 1:
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3)) {
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = 0;
                            alpha[i] = 0.0;
                        } else {
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
                            alpha[i] = 1.0;
                        }
                    }
                    break;
                case 2:
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3)) {
                            alpha[i] = 0.0;
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = 0;                            
                        } else {
                            alpha[i] = (float) imageIn[2*i+1] / 255;
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
                        }
                    }
                    break;
                case 3:
                    memcpy(samples, imageIn, 3*width);
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3)) {
                            alpha[i] = 0.0;
                            memcpy(samples+3*i, 0, 3);
                        } else {
                            alpha[i] = 1.0;
                        }
                    }
                    break;
                case 4:
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3)) {
                            memcpy(samples+i*3, 0, 3);
                            alpha[i] = 0.0;
                        } else {
                            memcpy(samples+i*3,imageIn+i*4,3);
                            alpha[i] = (float) imageIn[i*4+3] / 255.;
                        }
                    }
                    break;
            }
        }

        void store ( uint8_t* imageIn, uint8_t* maskIn, int srcSpp) {
            mask = maskIn;
            switch (srcSpp) {
                case 1:
                    for (int i = 0; i < width; i++) {
                        samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
                        alpha[i] = 1.0;
                    }
                    break;
                case 2:
                    for (int i = 0; i < width; i++) {
                        samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
                        alpha[i] = (float) imageIn[2*i+1] / 255;
                    }
                    break;
                case 3:
                    memcpy(samples, imageIn, 3*width);
                    for (int i = 0; i < width; i++) { alpha[i] = 1.0; }
                    break;
                case 4:
                    for (int i = 0; i < width; i++) {
                        memcpy(samples+i*3,imageIn+i*4,3);
                        alpha[i] = (float) imageIn[i*4+3] / 255.;
                    }
                    break;
            }
        }

        void store ( float* imageIn, uint8_t* maskIn, int srcSpp, float* transparent) {
            mask = maskIn;
            switch (srcSpp) {
                case 1:
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3*sizeof(float))) {
                            alpha[i] = 0.0;
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = 0.;
                        } else {
                            alpha[i] = 1.0;
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
                        }
                    }
                    break;
                case 2:
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3*sizeof(float))) {
                            alpha[i] = 0.0;
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = 0.;
                        } else {
                            alpha[i] = imageIn[2*i+1];
                            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
                        }
                    }
                    break;
                case 3:
                    memcpy(samples, imageIn, sizeof(float)*3*width);
                    for (int i = 0; i < width; i++) {
                        if (! memcmp(samples+3*i, transparent, 3*sizeof(float))) {
                            alpha[i] = 0.0;
                            memcpy(samples+3*i, 0, sizeof(float)*3);
                        } else {
                            alpha[i] = 1.0;
                        }
                    }
                    break;
                case 4:
                    for (int i = 0; i < width; i++) {
                        memcpy(samples+i*3, imageIn+i*4, sizeof(float)*3);
                        if (! memcmp(samples+3*i, transparent, 3*sizeof(float))) {
                            memcpy(samples+i*3, 0, sizeof(float)*3);
                            alpha[i] = 0.0;
                        } else {
                            memcpy(samples+i*3, imageIn+i*4, sizeof(float)*3);
                            alpha[i] = imageIn[i*4+3];
                        }
                    }
                    break;
            }
        }

        void store ( float* imageIn, uint8_t* maskIn, int srcSpp) {
            mask = maskIn;
            switch (srcSpp) {
                case 1:
                    for (int i = 0; i < width; i++) {
                        samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
                        alpha[i] = 1.0;
                    }
                    break;
                case 2:
                    for (int i = 0; i < width; i++) {
                        samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
                        alpha[i] = imageIn[2*i+1];
                    }
                    break;
                case 3:
                    memcpy(samples, imageIn, sizeof(float)*3*width);
                    for (int i = 0; i < width; i++) { alpha[i] = 1.0; }
                    break;
                case 4:
                    for (int i = 0; i < width; i++) {
                        memcpy(samples+i*3,imageIn+i*4,sizeof(float)*3);
                        alpha[i] = imageIn[i*4+3];
                    }
                    break;
            }
        }

        void write(float* buffer, int outChannels) {
            switch (outChannels) {
                case 1:
                    for (int i = 0; i < width; i++) {
                        buffer[i] = 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2];
                    }
                    break;
                case 2:
                    for (int i = 0; i < width; i++) {
                        buffer[2*i] = 0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2];
                        buffer[2*i+1] = alpha[i];
                    }
                    break;
                case 3:
                    for (int i = 0; i < width; i++) {
                        buffer[3*i] = samples[3*i];
                        buffer[3*i+1] = samples[3*i+1];
                        buffer[3*i+2] = samples[3*i+2];
                    }
                    break;
                case 4:
                    for (int i = 0; i < width; i++) {
                        buffer[4*i] = samples[3*i];
                        buffer[4*i+1] = samples[3*i+1];
                        buffer[4*i+2] = samples[3*i+2];
                        buffer[4*i+3] = alpha[i];
                    }
                    break;
            }
        }

        void write(uint8_t* buffer, int outChannels) {
            switch (outChannels) {
                case 1:
                    for (int i = 0; i < width; i++) {
                        buffer[i] = (uint8_t) (0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2]);
                    }
                    break;
                case 2:
                    for (int i = 0; i < width; i++) {
                        buffer[2*i] = (uint8_t) (0.2125*samples[3*i] + 0.7154*samples[3*i+1] + 0.0721*samples[3*i+2]);
                        buffer[2*i+1] = (uint8_t) (alpha[i]*255);
                    }
                    break;
                case 3:
                    for (int i = 0; i < width; i++) {
                        buffer[3*i] = samples[3*i];
                        buffer[3*i+1] = samples[3*i+1];
                        buffer[3*i+2] = samples[3*i+2];
                    }
                    break;
                case 4:
                    for (int i = 0; i < width; i++) {
                        buffer[4*i] = samples[3*i];
                        buffer[4*i+1] = samples[3*i+1];
                        buffer[4*i+2] = samples[3*i+2];
                        buffer[4*i+3] = (uint8_t) (alpha[i]*255);
                    }
                    break;
            }
        }

        void alphaBlending(Line<T>* above) {
            float finalAlpha;
            for (int i = 0; i < width; i++) {

                if (above->alpha[i] == 0. || ! above->mask[i]) {
                    // Le pixel de la ligne du dessus est transparent, ou n'est pas de la donnée, il ne change donc pas le pixel du dessous.
                    continue;
                }

                alpha[i] = above->alpha[i] + alpha[i] * (1. - above->alpha[i]);
                
                samples[3*i] = (T) (above->samples[3*i] + samples[3*i] * (1 - above->alpha[i]));
                samples[3*i+1] = (T) (above->samples[3*i+1] + samples[3*i+1] * (1 - above->alpha[i]));
                samples[3*i+2] = (T) (above->samples[3*i+2] + samples[3*i+2] * (1 - above->alpha[i]));
            }
        }

        void multiply(Line<T>* above) {
            for (int i = 0; i < width; i++) {
                if (! above->mask[i]) {
                    // Le pixel de la ligne du dessus n'est pas de la donnée, il ne change donc pas le pixel du dessous.
                    continue;
                }

                alpha[i] *= above->alpha[i];
                samples[3*i] = samples[3*i] * above->samples[3*i] / 255;
                samples[3*i+1] = samples[3*i+1] * above->samples[3*i+1] / 255;
                samples[3*i+2] = samples[3*i+2] * above->samples[3*i+2] / 255;
            }
        }

        void useMask(Line<T>* above) {
            for (int i = 0; i < width; i++) {
                if (mask[i]) {
                    alpha[i] = above->alpha[i];
                    samples[3*i] = above->samples[3*i];
                    samples[3*i+1] = above->samples[3*i+1];
                    samples[3*i+2] = above->samples[3*i+2];
                }
            }
        }

        /**
         * \~french
         * \brief Destructeur par défaut
         * \details Libération de la mémoire occupée par les tableaux.
         * \~english
         * \brief Default destructor
         */
        virtual ~Line() {
            delete[] alpha;
            delete[] samples;
        }
};

#endif
