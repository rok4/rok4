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
 * \file Grid.cpp
 ** \~french
 * \brief Implémentation de la classe Grid
 ** \~english
 * \brief Implement class Grid
 */

#include <proj_api.h>
#include <pthread.h>

#include "Grid.h"
#include "Logger.h"

#include <algorithm>

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

// Définit dans BoundingBox
//static pthread_mutex_t mutex_proj = PTHREAD_MUTEX_INITIALIZER;

Grid::Grid ( int width, int height, BoundingBox<double> bbox ) : width ( width ), height ( height ), bbox ( bbox ) {

    if (width == 0 || height == 0) {
        LOGGER_ERROR("One grid's dimension is null");
    }

    nbxReg = 1 + ( width-1 ) /stepInt;
    nbyReg = 1 + ( height-1 ) /stepInt;

    nbx = nbxReg;
    nby = nbyReg;

    /* On veut toujours que le dernier pixel reprojeté soit le dernier de la ligne, ou de la colonne.
     * On ajoute donc toujours le dernier pixel à ceux de la grille, même si celui ci y était déjà.
     * On en ajoute donc un, qui aura potentiellement un écart avec l'avant dernier plus petit (voir même 0).
     * Il faudra donc faire attention à cette différence lors de l'interpolation d'une ligne
     */
    nbx = nbxReg + 1;
    nby = nbyReg + 1;

    endX = width - 1 - ( nbxReg-1 ) * stepInt;
    endY = height - 1 - ( nbyReg-1 ) * stepInt;

    double resX = ( bbox.xmax - bbox.xmin ) / double ( width );
    double resY = ( bbox.ymax - bbox.ymin ) / double ( height );

    // Coordonnées du centre du pixel en haut à droite
    double left = bbox.xmin + 0.5 * resX;
    double top = bbox.ymax - 0.5 * resY;

    // Calcul du pas en unité terrain (et non pixel)
    double stepX = stepInt * resX;
    double stepY = stepInt * resY;

    gridX = new double[ nbx * nby ];
    gridY = new double[ nbx * nby ];

    for ( int y = 0 ; y < nby; y++ ) {
        for ( int x = 0 ; x < nbx; x++ ) {
            if ( y == nbyReg ) {
                // Last reprojected pixel = last pixel
                gridY[nbx*y + x] = top  - ( height-1 ) *resY;
            } else {
                gridY[nbx*y + x] = top  - y*stepY;
            }
            if ( x == nbxReg ) {
                // Last reprojected pixel = last pixel
                gridX[nbx*y + x] = left + ( width-1 ) *resX;
            } else {
                gridX[nbx*y + x] = left + x*stepX;
            }
        }
    }

    calculateDeltaY();
}

inline void Grid::calculateDeltaY() {
    double min = gridY[0];
    double max = gridY[0];
    for ( int x = 1 ; x < nbx; x++ ) {
        min = std::min ( min, gridY[x] );
        max = std::max ( max, gridY[x] );
    }

    deltaY = max - min;
}

void Grid::affine_transform ( double Ax, double Bx, double Ay, double By ) {
    for ( int i = 0; i < nbx*nby; i++ ) {
        gridX[i] = Ax * gridX[i] + Bx;
        gridY[i] = Ay * gridY[i] + By;
    }

    // Mise à jour de la bbox
    if ( Ax > 0 ) {
        bbox.xmin = bbox.xmin*Ax + Bx;
        bbox.xmax = bbox.xmax*Ax + Bx;
    } else {
        double xmintmp = bbox.xmin;
        bbox.xmin = bbox.xmax*Ax + Bx;
        bbox.xmax = xmintmp*Ax + Bx;
    }

    if ( Ay > 0 ) {
        bbox.ymin = bbox.ymin*Ay + By;
        bbox.ymax = bbox.ymax*Ay + By;
    } else {
        double ymintmp = bbox.ymin;
        bbox.ymin = bbox.ymax*Ay + By;
        bbox.ymax = ymintmp*Ay + By;
    }

    deltaY = deltaY * fabs ( Ay );
    LOGGER_DEBUG ( "New first line Y-delta :" << deltaY );
}

double Grid::getRatioY()
{
    double ratio = 0;

    if (height == 1) {
        /* Dans le cas d'une hauteur nulle, on ne peut pas faire la différence entre le premier et le dernier pixel de la colonne.
         * On va donc utiliser la bbox. On doit cependant retrancher à la différence ymax-ymin les écarts dûs à la déformation engendrée par la reprojection
         */
        double min = gridY[0];
        double max = gridY[0];
        for ( int x = 1 ; x < nbx; x++ ) {
            min = std::min(min, gridY[x]);
            max = std::max(max, gridY[x]);
        }

        double delta = max - min;
        
        return (bbox.ymax - bbox.ymin - delta);
    }

    for ( int x = 0 ; x < nbx; x++ ) {
        ratio = __max (ratio, fabs( gridY[x] - gridY[nbx*(nby-1) + x] ) / (double) (height - 1));
    }

    return ratio;
}

double Grid::getRatioX()
{
    double ratio = 0;

    if (width == 1) {
        /* Dans le cas d'une largeur nulle, on ne peut pas faire la différence entre le premier et le dernier pixel de la ligne.
         * On va donc utiliser la bbox. On doit cependant retrancher à la différence xmax-xmin les écarts dûs à la déformation engendrée par la reprojection
         */
        double min = gridX[0];
        double max = gridX[0];
        for ( int y = 1 ; y < nby; y++ ) {
            min = std::min(min, gridX[y*nbx]);
            max = std::max(max, gridX[y*nbx]);
        }

        double delta = max - min;

        return (bbox.xmax - bbox.xmin - delta);
    }
    
    for ( int y = 0 ; y < nby; y++ ) {
        ratio = __max (ratio, fabs( gridX[nbx*y] - gridX[nbx*(y+1) - 1] ) / (double) (width - 1));
    }

    return ratio;
}


bool Grid::reproject ( std::string from_srs, std::string to_srs ) {
    LOGGER_DEBUG ( from_srs<<" -> " <<to_srs );

    if (pthread_mutex_trylock ( & mutex_proj ) != 0 ) {
        LOGGER_ERROR("Can't lock mutex for reprojection.");
        return false;
    }


    projCtx ctx = pj_ctx_alloc();

    projPJ pj_src, pj_dst;
    if ( ! ( pj_src = pj_init_plus_ctx ( ctx, ( "+init=" + from_srs +" +wktext" ).c_str() ) ) ) {
        // Initialisation du système de projection source
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        LOGGER_ERROR ( "erreur d initialisation " << from_srs << " " << msg );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }
    if ( ! ( pj_dst = pj_init_plus_ctx ( ctx, ( "+init=" + to_srs +" +wktext +over" ).c_str() ) ) ) {
        // Initialisation du système de projection destination
        int err = pj_ctx_get_errno ( ctx );
        char *msg = pj_strerrno ( err );
        LOGGER_ERROR ( "erreur d initialisation " << to_srs << " " << msg );
        pj_free ( pj_src );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    LOGGER_DEBUG ( "Avant (centre du pixel en haut à gauche) "<< gridX[0] << " " << gridY[0] );
    LOGGER_DEBUG ( "Avant (centre du pixel en haut à droite) "<< gridX[nbx-1] << " " << gridY[nbx-1] );
    LOGGER_DEBUG ( "Avant (centre du pixel en bas à gauche) "<< gridX[nbx*(nby-1)] << " " << gridY[nbx*(nby-1)] );
    LOGGER_DEBUG ( "Avant (centre du pixel en bas à droite) "<< gridX[nbx*nby-1] << " " << gridY[nbx*nby-1] );

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
        LOGGER_ERROR ( "Code erreur proj4 : " << code );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    // On vérifie que le résultat renvoyé par la reprojection est valide
    for ( int i = 0; i < nbx*nby; i++ ) {
        if ( gridX[i] == HUGE_VAL || gridY[i] == HUGE_VAL ) {
            LOGGER_ERROR ( "Valeurs retournees par pj_transform invalides" );
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
    LOGGER_DEBUG ( "Apres (centre du pixel en bas à gauche) "<<gridX[nbx*(nby-1)]<<" "<<gridY[nbx*(nby-1)] );
    LOGGER_DEBUG ( "Apres (centre du pixel en bas à droite) "<<gridX[nbx*nby-1]<<" "<<gridY[nbx*nby-1] );

    /****************** Mise à jour de la bbox *********************
     * On n'utilise pas les coordonnées présentent dans les tableaux X et Y car celles ci correspondent
     * aux centres des pixels, et non au bords. On va donc reprojeter la bbox indépendemment.
     * On divise chaque côté de la bbox en la dimensions la plus grande.
     */
    if ( bbox.reproject ( pj_src, pj_dst ) ) {
        LOGGER_ERROR ( "Erreur reprojection bbox" );
        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );
        return false;
    }

    /****************** Mise à jour du deltaY **********************/
    calculateDeltaY();
    LOGGER_DEBUG ( "New first line Y-delta :" << deltaY );

    // Nettoyage
    pj_free ( pj_src );
    pj_free ( pj_dst );
    pj_ctx_free ( ctx );
    pthread_mutex_unlock ( & mutex_proj );

    return true;
}

int Grid::getline ( int line, float* X, float* Y ) {

    int dy = line / stepInt;
    double w = 0;

    if ( dy == nbyReg - 1 ) {
        if ( endY == 0 ) {
            w = 0;
        } else {
            w = ( ( line%stepInt ) ) / double ( endY );
        }
    } else {
        w = ( line%stepInt ) / double ( stepInt );
    }

    double LX[nbx], LY[nbx];

    // Interpolation dans les sens des Y
    for ( int i = 0; i < nbx; i++ ) {
        LX[i] = ( 1-w ) *gridX[dy*nbx + i] + w*gridX[ ( dy+1 ) * nbx + i];
        LY[i] = ( 1-w ) *gridY[dy*nbx + i] + w*gridY[ ( dy+1 ) * nbx + i];
    }

    // Indice dans la grille du dernier pixel reprojetée car respecte le pas de base (dans le sens des x)
    int lastRegularPixel = ( nbxReg-1 ) *stepInt;

    /* Interpolation dans le sens des X, sur la partie où la répartition des pixels reprojetés
     * est régulière (tous les stepInt pixels */
    for ( int i = 0; i <= lastRegularPixel; i++ ) {
        int dx = i / stepInt;
        double w = ( i%stepInt ) /double ( stepInt );
        X[i] = ( 1-w ) *LX[dx] + w*LX[dx+1];
        Y[i] = ( 1-w ) *LY[dx] + w*LY[dx+1];
    }

    /* Interpolation dans le sens des X, sur la partie où la distance entre les deux pixels de la grille est différente */
    for ( int i = 1; i <= endX; i++ ) {
        double w = i /double ( endX );
        X[lastRegularPixel + i] = ( 1-w ) *LX[nbxReg - 1] + w * LX[nbxReg];
        Y[lastRegularPixel + i] = ( 1-w ) *LY[nbxReg - 1] + w * LY[nbxReg];
    }

    return width;

}
