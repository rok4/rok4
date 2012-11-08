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

#ifndef RESAMPLED_IMAGE_H
#define RESAMPLED_IMAGE_H

#include "Image.h"
#include "Kernel.h"
#include "Interpolation.h"
#include <mm_malloc.h>

class ResampledImage : public Image {
    private:
        // Image source à rééchantilloner.
        Image* image;

        // A-t-on besoin de tenir compte des masques dans l'interpolation (lourd)
        bool bUseMask;

        // Noyau de la méthode d'interpolation à utiliser.
        const Kernel& K;

        // Nombre maximal de pixels sources pris en compte pour une interpolation en x.
        int Kx;
        // Nombre maximal de pixels sources pris en compte pour une interpolation en y.
        int Ky;

        // Ratio de rééchantillonage en x = résolution x source / résolution x cible
        double ratio_x;
        // Ratio de rééchantillonage en y = résolution y source / résolution y cible
        double ratio_y;

        // Offset en y du haut de l'image rééchantillonée par rapport à l'image source (en nombre de pixels source)
        double top;
        // Offset en x de bord gauche l'image rééchantillonée par rapport à l'image source (en nombre de pixels source)
        double left;

        float* __buffer;

        // Buffers de travail en float
        float* src_image_buffer[4];
        float* mux_src_image_buffer;

        float* src_mask_buffer[4];
        float* mux_src_mask_buffer;

        // Nombre de lignes rééchantillonnée mémorisées : image et masque
        int memorize_line;

        float* mux_resampled_image;
        float** resampled_image;

        float* mux_resampled_mask;
        float** resampled_mask;
        float* own_mask_buffer;
        
        int* resampled_line_index;

        float *dst_image_buffer;
        float *weight_buffer;
        
        /**
        * Rééchantillonne une ligne source selon les x.
        *
        * @param line Indice de la ligne de l'image source
        * @return pointeur vers la ligne rééchantillonnée de width pixels.
        */
        int resampleSourceLine(int line);


        float* Wx;
        int* xmin;

    public:
        ResampledImage(Image *image, int width, int height, double resx, double resy, double left, double top,
                       double ratio_x, double ratio_y, bool bUseMask = false,
                       Interpolation::KernelType KT = Interpolation::LANCZOS_3,
                       BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.));
        
        ~ResampledImage() {
            //std::cerr << "Delete ResampledImage" << std::endl; /*TEST*/
            _mm_free(__buffer);
            delete[] resampled_line_index;
            delete image;
        }
        
        int getline(float* buffer, int line);
        int getline(uint8_t* buffer, int line);

        /** Fonction d'export des informations sur l'image (pour le débug) */
        void print() {
            LOGGER_INFO("");
            LOGGER_INFO("--------- ResampledImage -----------");
            Image::print();
            LOGGER_INFO("\t- Kernel size, x wise = " << Kx << ", y wise = " << Ky);
            LOGGER_INFO("\t- Offsets, dx = " << left << ", dy = " << top);
            if (bUseMask) {
                LOGGER_INFO("\t- Use mask in interpolation");
            } else {
                LOGGER_INFO("\t- Doesn't use mask in interpolation");
            }
            LOGGER_INFO("");
        }
};

#endif




