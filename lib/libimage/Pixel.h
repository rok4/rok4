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
 * \file Pixel.h
 * \~french
 * \brief Définition de la classe Pixel, permettant la manipulation et la conversion de pixels.
 * \~english
 * \brief Define the Pixel class , to manipulate and convert pixels.
 */

#ifndef PIXEL_H
#define PIXEL_H

#include <limits>

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Représentation d'un pixel flottant
 */
template<typename T = float>
class Pixel {

    public:
        float s1, s2, s3, alpha;

        // avec Alpha
        Pixel ( float* values, int srcSpp) {
            switch (srcSpp) {
                case 1:
                    s1 = s2 = s3 = values[0];
                    alpha = 1.0;
                    break;
                case 2:
                    s1 = s2 = s3 = values[0];
                    alpha = values[1];
                    break;
                case 3:
                    s1 = values[0]; s2 = values[1]; s3 = values[2];
                    alpha = 1.0;
                    break;
                case 4:
                    s1 = values[0]; s2 = values[1]; s3 = values[2];
                    alpha = values[3];
                    break;
            }
        }

        void write(float* buffer, int outChannels) {
            switch (outChannels) {
                case 1:
                    buffer[0] = (0.2125*s1 + 0.7154*s2 + 0.0721*s3)*alpha;
                    break;
                case 2:
                    buffer[0] = 0.2125*s1 + 0.7154*s2 + 0.0721*s3;
                    buffer[3] = alpha;
                    break;
                case 3:
                    buffer[0] = alpha*s1;
                    buffer[1] = alpha*s2;
                    buffer[2] = alpha*s3;
                    break;
                case 4:
                    buffer[0] = s1;
                    buffer[1] = s2;
                    buffer[2] = s3;
                    buffer[3] = alpha;
                    break;
            }
        }
};

/** \~ \author Institut national de l'information géographique et forestière
 ** \~french
 * \brief Représentation d'un pixel entier
 */
template<typename T = uint8_t>
class Pixel {

    public:
        uint8_t s1, s2, s3;
        float alpha;

        // avec Alpha
        Pixel ( T* values, int srcSpp) {
            switch (srcSpp) {
                case 1:
                    s1 = s2 = s3 = values[0];
                    alpha = 1.0;
                    break;
                case 2:
                    s1 = s2 = s3 = values[0];
                    alpha = (float) values[1]/255.;
                    break;
                case 3:
                    s1 = values[0]; s2 = values[1]; s3 = values[2];
                    alpha = 1.0;
                    break;
                case 4:
                    s1 = values[0]; s2 = values[1]; s3 = values[2];
                    // Sur 8 bits, la transparence va de 0 à 255, on la ramène donc entre 0 et 1
                    alpha = (float) values[3]/255.;
                    break;
            }
        }

        void isItTransparent(Pixel* transparent) {
            if (s1 == transparent->s1 && s2 == transparent->s2 && s3 == transparent->s3) {
                alpha = 0.0;
            }
        }

        void alphaBlending(Pixel* back) {
            alpha = alpha + back->alpha * (1. - alpha);
            s1 = (T) ((s1*alpha + back->s1 * back->alpha * (1 - alpha)) / alpha);
            s2 = (T) ((s2*alpha + back->s2 * back->alpha * (1 - alpha)) / alpha);
            s3 = (T) ((s3*alpha + back->s3 * back->alpha * (1 - alpha)) / alpha);
        }

        void multiply(Pixel* back) {
            alpha *= back->alpha;
            s1 = (s1 * back->s1) / 255;
            s2 = (s2 * back->s2) / 255;
            s3 = (s3 * back->s3) / 255;
        }

        void write(uint8_t* buffer, int outChannels) {
            switch (outChannels) {
                case 1:
                    buffer[0] = (uint8_t) (0.2125*s1 + 0.7154*s2 + 0.0721*s3)*alpha;
                    break;
                case 2:
                    buffer[0] = (uint8_t) 0.2125*s1 + 0.7154*s2 + 0.0721*s3;
                    buffer[3] = (uint8_t) (alpha * 255);
                    break;
                case 3:
                    buffer[0] = (uint8_t) alpha*s1;
                    buffer[1] = (uint8_t) alpha*s2;
                    buffer[2] = (uint8_t) alpha*s3;
                    break;
                case 4:
                    buffer[0] = s1;
                    buffer[1] = s2;
                    buffer[2] = s3;
                    buffer[3] = (uint8_t) (alpha * 255);
                    break;
            }
        }
};

#endif
