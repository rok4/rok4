/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file DecimatedImage.cpp
 ** \~french
 * \brief Implémentation des classes DecimatedImage, DecimatedMask et DecimatedImageFactory
 * \details
 * \li DecimatedImage : image calculée à  d'images compatibles, superposables
 * \li DecimatedMask : masque composé, associé à une image composée
 * \li DecimatedImageFactory : usine de création d'objet DecimatedImage
 ** \~english
 * \brief Implement classes DecimatedImage, DecimatedMask and DecimatedImageFactory
 * \details
 * \li DecimatedImage : image compounded with superimpose images
 * \li DecimatedMask : compounded mask, associated with a compounded image
 * \li DecimatedImageFactory : factory to create DecimatedImage object
 */

#include "DecimatedImage.h"
#include <boost/log/trivial.hpp>
#include "Utils.h"


/********************************************** DecimatedImage ************************************************/

template <typename T>
int DecimatedImage::_getline ( T* buffer, int line ) {
    
    // Initialisation de tous les pixels de la ligne avec la valeur de nodata
    for ( int i = 0; i < width * channels; i++ ) {
        buffer[i]= ( T ) nodata[i%channels];
    }
    
    if ( numberX == 0 ) {
        // On est à gauche ou à droite de l'image source
        return width*channels;
    }
    
    // Ordonnée du centre de la ligne demandée
    double y_centre = bbox.ymax - (0.5 + line) * resy;
    
    int src_ligne = sourceImage->y2l ( y_centre );

    if ( src_ligne < 0 || src_ligne >= sourceImage->getHeight() ) {
        // On est au dessus ou en dessous de l'image source
        return width*channels;
    }
    
    T* buffer_t = new T[sourceImage->getWidth() * sourceImage->getChannels()];
    sourceImage->getline ( buffer_t, src_ligne );
    
    T* pix_src = buffer_t + sourceOffsetX * sourceImage->getChannels();
    T* pix_dst = buffer + imageOffsetX * channels;
    
    if ( sourceImage->getMask() == NULL ) {
        for (int i = 0; i < numberX; i++) {
            memcpy(pix_dst, pix_src, channels*sizeof ( T ));
            pix_src += ratioX * sourceImage->getChannels();
            pix_dst += channels;
        }
    } else {

        uint8_t* buffer_m = new uint8_t[sourceImage->getMask()->getWidth()];
        sourceImage->getMask()->getline ( buffer_m, src_ligne );
        
        uint8_t* pix_src_mask = buffer_m + sourceOffsetX;
        for (int i = 0; i < numberX; i++) {
            if (*pix_src_mask) {
                memcpy(pix_dst, pix_src, channels*sizeof ( T ));
            }
            pix_src += ratioX * sourceImage->getChannels();
            pix_src_mask += ratioX;
            pix_dst += channels;
        }

        delete [] buffer_m;
    }
    
    delete [] buffer_t;
    
    return width*channels;
}


/* Implementation de getline pour les uint8_t */
int DecimatedImage::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de getline pour les float */
int DecimatedImage::getline ( uint16_t* buffer, int line ) {
    return _getline ( buffer, line );
}

/* Implementation de getline pour les float */
int DecimatedImage::getline ( float* buffer, int line ) {
    return _getline ( buffer, line );
}

DecimatedImage::DecimatedImage ( int width, int height, int channels, double resx, double resy, BoundingBox<double> bbox,
                        Image* image, int* nd ) :
    Image ( width, height, channels, resx, resy, bbox ),
    sourceImage ( image ) {

    nodata = new int[channels];
    memcpy ( nodata, nd, channels*sizeof ( int ) );
    
    ratioX = (int) resx/image->getResX() + 0.5;
    ratioY = (int) resy/image->getResY() + 0.5;
    
    // Centre X de la première colonne de pixels de l'image décimée
    double x1_centre = bbox.xmin + 0.5 * resx;
    BOOST_LOG_TRIVIAL(debug) << "x1centre = " << x1_centre;
    
    // Centre X de la dernière colonne de pixels de l'image décimée
    double xd_centre = bbox.xmax - 0.5 * resx;
    
    // Colonne des premiers et dernier pixel
    
    int x1_src = image->x2c ( x1_centre );
    int xd_src = image->x2c ( xd_centre );
    
    if ( x1_src >= image->getWidth() || xd_src < 0 ) {
        // On est à gauche ou à droite de l'image source
        // Tous les pixels de l'image décimée sont à côté de l'image source. Pourquoi pas, on aura une image de nodata
        numberX = 0;
    } else {
    
        // On va chercher le premier pixel de l'image source qui est utilisé
        double x_cur = x1_centre;
        imageOffsetX = 0;
        while (x_cur < image->getBbox().xmin) {
        BOOST_LOG_TRIVIAL(debug) << "x1centre = " << x_cur;
            imageOffsetX++;
            x_cur += resx;
        }
        BOOST_LOG_TRIVIAL(debug) << "x1centre = " << x_cur;
        sourceOffsetX = image->x2c ( x_cur );
        
        // On va chercher le nombre de pixels à piocher dans l'image source pour constituer une ligne
        x_cur = xd_centre;
        while (x_cur > image->getBbox().xmax) {
            x_cur -= resx;
        }
        
        xd_src = image->x2c ( x_cur );
        
        numberX = (xd_src - sourceOffsetX) / ratioX + 1;
        
        // TEST
        if ((xd_src - sourceOffsetX) % ratioX != 0) {
            BOOST_LOG_TRIVIAL(warning) << "(xd_src - sourceOffsetX) % ratioX != 0";
        }
    }    
    
}


/****************************************** DecimatedImageFactory *********************************************/

DecimatedImage* DecimatedImageFactory::createDecimatedImage ( Image* image, BoundingBox<double> bb, double res_x, double res_y, int* nodata ) {

    if ( image == NULL ) {
        BOOST_LOG_TRIVIAL(error) <<  "No source image to define decimated image" ;
        return NULL;
    }
    
    // On vérifie que la résolution est bien un mutiple de la résolution source
    double intpart;
    double phi = modf ( res_x/image->getResX(), &intpart );
    if (phi > 0.0001 && phi < 0.9999) {
        BOOST_LOG_TRIVIAL(error) <<  "Decimated image resolution must be a multiple of source image's resolution (x wise)" ;
        return NULL;        
    }
    
    phi = modf ( res_y/image->getResY(), &intpart );
    if (phi > 0.0001 && phi < 0.9999) {
        BOOST_LOG_TRIVIAL(error) <<  "Decimated image resolution must be a multiple of source image's resolution (y wise)" ;
        return NULL;        
    }
    
    // On vérifie que le centre d'un pixel de l'image décimée est bien aligné avec un centre de pixel de l'image source
    
    // Centre du pixel en haut à gauche de l'image décimée
    double x_centre = bb.xmin + 0.5 * res_x;
    double y_centre = bb.ymax - 0.5 * res_y;
    
    BoundingBox<double> bb_source = image->getBbox();
    
    // Centre du pixel en haut à gauche de l'image source
    double x_centre_src = bb_source.xmin + 0.5 * image->getResX();
    double y_centre_src = bb_source.ymax - 0.5 * image->getResY();
    
    // On regarde si la différence entre les deux coordonnées est bien un nombre entier * la résolution source
    
    phi = modf ( (x_centre - x_centre_src) / image->getResX(), &intpart );
    if (phi > 0.0001 && phi < 0.9999) {
        BOOST_LOG_TRIVIAL(error) <<  "Decimated image pixel center must be aligned with a source image's pixel's center (x wise)" ;
        return NULL;        
    }
    
    phi = modf ( (y_centre - y_centre_src) / image->getResY(), &intpart );
    if (phi > 0.0001 && phi < 0.9999) {
        BOOST_LOG_TRIVIAL(error) <<  "Decimated image pixel center must be aligned with a source image's pixel's center (y wise)" ;
        return NULL;        
    }
    
    // On calcule les dimensions de l'image

    int w = ( int ) ( bb.xmax - bb.xmin ) / res_x + 0.5 ;
    int h = ( int ) ( bb.ymax - bb.ymin ) / res_y + 0.5 ;
    
    DecimatedImage* pDI = new DecimatedImage ( w, h, image->getChannels(), res_x, res_y, bb, image, nodata);

    return pDI;
}

