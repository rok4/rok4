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

#ifndef _BOUNDINGBOX_
#define _BOUNDINGBOX_

/**
* @file BoundingBox.h
* @brief Implementation d'un rectangle englobant
* @author IGN France - Geoportail
*/

#include "Logger.h"
#include <proj_api.h>

/*
* @struct BoundingBox
*/

template<typename T>
struct BoundingBox {
    
public:
    T xmin, ymin, xmax, ymax;
    BoundingBox ( T xmin, T ymin, T xmax, T ymax ) :
        xmin ( xmin ), ymin ( ymin ), xmax ( xmax ), ymax ( ymax ) {}

    template<typename T2>        
    BoundingBox ( const BoundingBox<T2>& bbox ) :
        xmin ( (T) bbox.xmin ), ymin ( (T) bbox.ymin ), xmax ( (T) bbox.xmax ), ymax ( (T) bbox.ymax ) {}

    int reproject ( projPJ pj_src, projPJ pj_dst ) {
        int nbSegment = 10;
        T stepX = ( xmax - xmin ) / T ( nbSegment );
        T stepY = ( ymax - ymin ) / T ( nbSegment );

        T segX[nbSegment*4];
        T segY[nbSegment*4];

        for ( int i = 0; i < nbSegment; i++ ) {
            segX[4*i] = xmin + i*stepX;
            segY[4*i] = ymin;

            segX[4*i+1] = xmin + i*stepX;
            segY[4*i+1] = ymax;

            segX[4*i+2] = xmin;
            segY[4*i+2] = ymin + i*stepY;

            segX[4*i+3] = xmax;
            segY[4*i+3] = ymin + i*stepY;
        }

        if ( pj_is_latlong ( pj_src ) )
            for ( int i = 0; i < nbSegment*4; i++ ) {
                segX[i] *= DEG_TO_RAD;
                segY[i] *= DEG_TO_RAD;
            }

        int code = pj_transform ( pj_src, pj_dst, nbSegment*4, 0, segX, segY, 0 );

        if ( code != 0 ) {
            LOGGER_ERROR ( "Code erreur proj4 : " << code );
            return 1;
        }

        for ( int i = 0; i < nbSegment*4; i++ ) {
            if ( segX[i] == HUGE_VAL || segY[i] == HUGE_VAL ) {
                LOGGER_ERROR ( "Valeurs retournees par pj_transform invalides" );
                return 1;
            }
        }

        if ( pj_is_latlong ( pj_dst ) )
            for ( int i = 0; i < nbSegment*4; i++ ) {
                segX[i] *= RAD_TO_DEG;
                segY[i] *= RAD_TO_DEG;
            }

        xmin = segX[0];
        xmax = segX[0];
        ymin = segY[0];
        ymax = segY[0];

        for ( int i = 1; i < nbSegment*4; i++ ) {
            xmin = std::min ( xmin, segX[i] );
            xmax = std::max ( xmax, segX[i] );
            ymin = std::min ( ymin, segY[i] );
            ymax = std::max ( ymax, segY[i] );
        }

        return 0;
    }

    void print() {
        LOGGER_DEBUG ( "BBOX = " << xmin << " " << ymin << " " << xmax << " " << ymax );
    }
};
#endif

