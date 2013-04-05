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

#include <proj_api.h>
#include <pthread.h>

#include "Grid.h"
#include "Logger.h"

#include <algorithm>

static pthread_mutex_t mutex_proj= PTHREAD_MUTEX_INITIALIZER;

Grid::Grid ( int width, int height, BoundingBox<double> bbox ) : width ( width ), height ( height ), bbox ( bbox ) {
    nbxInt = 1 + ( width-1 ) /stepInt;
    nbyInt = 1 + ( height-1 ) /stepInt;
    nbx = nbxInt + 1;
    nby = nbyInt + 1;

    double resX = ( bbox.xmax - bbox.xmin ) / double ( width );
    double resY = ( bbox.ymax - bbox.ymin ) / double ( height );

    // Coordonnées du centre du pixel en haut à droite
    double left = bbox.xmin + 0.5 * resX;
    double top = bbox.ymax - 0.5 * resY;

    // Calcul du pas en unité terrain (et non pixel)
    double stepX = stepInt * resX;
    double stepY = stepInt * resY;

    gridX = new double[ nbx * nby];
    gridY = new double[ nbx * nby];

    for ( int y = 0 ; y < nby; y++ ) {
        for ( int x = 0 ; x < nbx; x++ ) {
            if ( y == nbyInt ) {
                // Last reprojected pixel = last pixel
                gridY[nbx*y + x] = top  - ( height-1 ) *resY;
            } else {
                gridY[nbx*y + x] = top  - y*stepY;
            }
            if ( x == nbxInt ) {
                // Last reprojected pixel = last pixel
                gridX[nbx*y + x] = left + ( width-1 ) *resX;
            } else {
                gridX[nbx*y + x] = left + x*stepX;
            }
        }
    }
}

Grid::~Grid() {
    delete[] gridX;
    delete[] gridY;
}

void Grid::affine_transform ( double Ax, double Bx, double Ay, double By ) {
    for ( int i = 0; i < nbx*nby; i++ ) {
        gridX[i] = Ax * gridX[i] + Bx;
        gridY[i] = Ay * gridY[i] + By;
    }

    // Mise à jour de la bbox
    bbox.xmin = bbox.xmin*Ax + Bx;
    bbox.xmax = bbox.xmax*Ax + Bx;

    bbox.ymin = bbox.ymin*Ay + By;
    bbox.ymax = bbox.ymax*Ay + By;
}

inline void update ( BoundingBox<double> &B, double x, double y ) {
    B.xmin = std::min ( B.xmin, x );
    B.xmax = std::max ( B.xmax, x );
    B.ymin = std::min ( B.ymin, y );
    B.ymax = std::max ( B.ymax, y );
}

/**
 * Effectue une reprojection sur les coordonnées de la grille
 *
 * (GridX[i], GridY[i]) = proj(GridX[i], GridY[i])
 * ou proj est une projection de from_srs vers to_srs
 */

// TODO : la projection est faite 2 fois. essayer de ne la faire faire qu'une fois.

bool Grid::reproject ( std::string from_srs, std::string to_srs ) {
    LOGGER_DEBUG ( from_srs<<" -> " <<to_srs );

    pthread_mutex_lock ( & mutex_proj );


    projCtx ctx = pj_ctx_alloc();

    projPJ pj_src, pj_dst;
    if ( ! ( pj_src = pj_init_plus_ctx ( ctx, ( "+init=" + from_srs +" +wktext" ).c_str() ) ) ) {
        // Initialisation du système de projection source
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        LOGGER_DEBUG ( "erreur d initialisation " << from_srs << " " << msg );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }
    if ( ! ( pj_dst = pj_init_plus_ctx ( ctx, ( "+init=" + to_srs +" +wktext +over" ).c_str() ) ) ) {
        // Initialisation du système de projection destination
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        LOGGER_DEBUG ( "erreur d initialisation " << to_srs << " " << msg );
        pj_free ( pj_src );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    LOGGER_DEBUG ( "Avant (centre du pixel en haut à gauche) "<<gridX[0]<<" "<<gridY[0] );
    LOGGER_DEBUG ( "Avant (centre du pixel en haut à droite) "<<gridX[nbx-1]<<" "<<gridY[nbx-1] );

    // Note that geographic locations need to be passed in radians, not decimal degrees,
    // and will be returned similarly

    if ( pj_is_latlong ( pj_src ) )
        for ( int i = 0; i < nbx*nby; i++ ) {
            gridX[i] *= DEG_TO_RAD;
            gridY[i] *= DEG_TO_RAD;
        }

    // On reprojette toutes les coordonnées
    int code = pj_transform ( pj_src, pj_dst, nbx*nby, 0, gridX, gridY, 0 );

    if ( code != 0 ) {
        LOGGER_DEBUG ( "Code erreur proj4 : " << code );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    // On vérifie que le résultat renvoyé par la reprojection est valide
    for ( int i = 0; i < nbx*nby; i++ ) {
        if ( gridX[i] == HUGE_VAL || gridY[i] == HUGE_VAL ) {
            LOGGER_DEBUG ( "Valeurs retournees par pj_transform invalides" );
            pj_free ( pj_src );
            pj_free ( pj_dst );
            pj_ctx_free ( ctx );
            pthread_mutex_unlock ( & mutex_proj );
            return false;
        }
    }

    if ( pj_is_latlong ( pj_dst ) )
        for ( int i = 0; i < nbx*nby; i++ ) {
            gridX[i] *= RAD_TO_DEG;
            gridY[i] *= RAD_TO_DEG;
        }

    LOGGER_DEBUG ( "Apres (centre du pixel en haut à gauche) "<<gridX[0]<<" "<<gridY[0] );
    LOGGER_DEBUG ( "Apres (centre du pixel en haut à droite) "<<gridX[nbx-1]<<" "<<gridY[nbx-1] );

    /****************** Mise à jour de la bbox *********************
     * On n'utilise pas les coordonnées présentent dans les tableaux X et Y car celles ci correspondent aux centres des pixels,
     * et non au bords. On va donc reprojeter la bbox indépendemment.
     * On divise chaque côté de la bbox en 10.
     */

    bbox.xmin = gridX[0];
    bbox.xmax = gridX[0];
    bbox.ymin = gridY[0];
    bbox.ymax = gridY[0];

    int nbSegment = 10;
    double stepX = ( bbox.xmax - bbox.xmin ) / double ( nbSegment );
    double stepY = ( bbox.ymax - bbox.ymin ) / double ( nbSegment );

    double segX[nbSegment*4];
    double segY[nbSegment*4];

    for ( int i = 0; i < nbSegment; i++ ) {
        segX[4*i] = bbox.xmin + i*stepX;
        segY[4*i] = bbox.ymin;

        segX[4*i+1] = bbox.xmin + i*stepX;
        segY[4*i+1] = bbox.ymax;

        segX[4*i+2] = bbox.xmin;
        segY[4*i+2] = bbox.ymin + i*stepY;

        segX[4*i+3] = bbox.xmax;
        segY[4*i+3] = bbox.ymin + i*stepY;
    }

    code = pj_transform ( pj_src, pj_dst, nbSegment*4, 0, segX, segY, 0 );

    if ( code != 0 ) {
        LOGGER_DEBUG ( "Code erreur proj4 : " << code );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    for ( int i = 0; i < nbSegment*4; i++ ) {
        if ( segX[i] == HUGE_VAL || segY[i] == HUGE_VAL ) {
            LOGGER_DEBUG ( "Valeurs retournees par pj_transform invalides" );
            pj_free ( pj_src );
            pj_free ( pj_dst );
            pj_ctx_free ( ctx );
            pthread_mutex_unlock ( & mutex_proj );
            return false;
        }
    }

    if ( pj_is_latlong ( pj_dst ) )
        for ( int i = 0; i < nbSegment*4; i++ ) {
            segX[i] *= RAD_TO_DEG;
            segY[i] *= RAD_TO_DEG;
        }

    for ( int i = 0; i < nbSegment*4; i++ ) {
        update ( bbox, segX[i], segY[i] );
    }

    // Nettoyage
    pj_free ( pj_src );
    pj_free ( pj_dst );
    pj_ctx_free ( ctx );
    pthread_mutex_unlock ( & mutex_proj );

    return true;
}

int Grid::interpolate_line ( int line, float* X, float* Y, int nb ) {
    int ky = line / stepInt;
    double w = ( stepInt - ( line%stepInt ) ) /double ( stepInt );
    double LX[nbx], LY[nbx];

    // Interpolation dans les sens des Y
    for ( int i = 0; i < nbx; i++ ) {
        LX[i] = w*gridX[ky*nbx + i] + ( 1-w ) *gridX[ky * nbx + nbx + i];
        LY[i] = w*gridY[ky*nbx + i] + ( 1-w ) *gridY[ky * nbx + nbx + i];
    }

    // Interpolation dans le sens des X
    for ( int i = 0; i < nb; i++ ) {
        int kx = i / stepInt;
        double w = ( stepInt - ( i%stepInt ) ) /double ( stepInt );
        X[i] = w*LX[kx] + ( 1-w ) *LX[kx+1];
        Y[i] = w*LY[kx] + ( 1-w ) *LY[kx+1];
    }

    return nb;
}



