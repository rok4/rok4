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

#ifndef REPROJECT_H
#define REPROJECT_H

#include "Image.h"
#include "Grid.h"
#include "Kernel.h"
#include "Interpolation.h"
#include <mm_malloc.h>

class ReprojectedImage : public Image {
private:
    // Image source à rééchantilloner.
    Image* sourceImage;

    /**
     * \~french \brief Précise si les masques doivent intervenir dans l'interpolation (lourd)
     * \~english \brief Precise if mask have to be used by interpolation (heavy)
     */
    bool useMask;

    /**
     * \~french \brief Noyau d'interpolation à utiliser
     * \~english \brief Interpolation kernel to use
     */
    const Kernel& K;

    /**
     * \~french \brief Nombre de pixels source intervenant dans l'interpolation, dans le sens des X
     * \~english \brief Number of source pixels used by interpolation, widthwise
     */
    int Kx;

    /**
     * \~french \brief Nombre de pixels source intervenant dans l'interpolation, dans le sens des Y
     * \~english \brief Number of source pixels used by interpolation, heightwise
     */
    int Ky;

    /**
     * \~french \brief Rapport des résolutions source et finale, dans le sens des X
     * \details Ratio de rééchantillonage en X = résolution X cible / résolution X source
     * \~english \brief Ratio between destination resolution and source resolution, widthwise
     * \details X ratio = X destination resolution / X source resolution
     */
    double ratio_x;
    /**
     * \~french \brief Rapport des résolutions source et finale, dans le sens des Y
     * \details Ratio de rééchantillonage en Y = résolution Y cible / résolution Y source
     * \~english \brief Ratio between destination resolution and source resolution, heighthwise
     * \details Y ratio = Y destination resolution / Y source resolution
     */
    double ratio_y;

    Grid* grid;

    float*  __buffer;
    
    float** src_image_buffer;
    float** src_mask_buffer;

    int dst_line_index;
    
    float* dst_image_buffer[4];
    float* mux_dst_image_buffer;

    float* dst_mask_buffer[4];
    float* mux_dst_mask_buffer;

    float* X[4];
    float* Y[4];
    float* Wx[1024];
    float* Wy[1024];


    float* WWx;
    float* WWy;
    int xmin[1024];
    int ymin[1024];

    float* tmp1Img;
    float* tmp2Img;

    float* tmp1Msk;
    float* tmp2Msk;

    float* compute_dst_line ( int line );

public:

    ReprojectedImage ( Image *image, BoundingBox<double> bbox, Grid* grid, Interpolation::KernelType KT = Interpolation::LANCZOS_2, bool bMask = false );

    int getline ( float* buffer, int line );

    int getline ( uint8_t* buffer, int line );

    ~ReprojectedImage() {
        delete sourceImage;
        _mm_free ( __buffer );
        delete[] src_image_buffer;
        if ( useMask ) {delete[] src_mask_buffer;}
    }

    /** \~french
     * \brief Sortie des informations sur l'image reprojetée
     ** \~english
     * \brief Reprojected image description output
     */
    void print() {
        LOGGER_INFO ( "" );
        LOGGER_INFO ( "--------- ReprojectedImage -----------" );
        Image::print();
        LOGGER_INFO ( "\t- Kernel size, x wise = " << Kx << ", y wise = " << Ky );
        LOGGER_INFO ( "\t- Ratio, x wise = " << ratio_x << ", y wise = " << ratio_y );
        grid->print();
        if ( useMask ) {
            LOGGER_INFO ( "\t- Use mask in interpolation" );
        } else {
            LOGGER_INFO ( "\t- Doesn't use mask in interpolation" );
        }
        LOGGER_INFO ( "" );
    }

};

#endif

