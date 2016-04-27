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
 * \file BoundingBox.h
 ** \~french
 * \brief Définition de la classe template BoundingBox
 ** \~english
 * \brief Define template class BoundingBox
 */

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "Logger.h"
#include <proj_api.h>
#include <sstream>

static pthread_mutex_t mutex_proj = PTHREAD_MUTEX_INITIALIZER;

/**
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Gestion d'un rectangle englobant
 * \details Cette classe template gère des coordonnées de plusieurs types, avec des constructeurs avec conversion. Elle met également à disposition des fonctions de reprojection et topologiques.
 * \~english \brief Manage a bounding box
 */
template<typename T>
class BoundingBox {

public:
    /**
     * \~french \brief Extrema du rectangle englobant
     * \~english \brief Bounding box limits
     */
    T xmin, ymin, xmax, ymax;

    /** \~french
     * \brief Crée un objet BoundingBox à partir de tous ses éléments constitutifs
     ** \~english
     * \brief Create a BoundingBox object, from all attributes
     */
    BoundingBox ( T xmin, T ymin, T xmax, T ymax ) :
        xmin ( xmin ), ymin ( ymin ), xmax ( xmax ), ymax ( ymax ) {}
    /** \~french \brief Crée un objet BoundingBox par copie et conversion
     * \param[in] bbox rectangle englobant à copier et éventuellement convertir
     ** \~english \brief Create a BoundingBox object, copying and converting
     * \param[in] bbox bounding box to copy and possibly convert
     */
    template<typename T2>
    BoundingBox ( const BoundingBox<T2>& bbox ) :
        xmin ( ( T ) bbox.xmin ), ymin ( ( T ) bbox.ymin ), xmax ( ( T ) bbox.xmax ), ymax ( ( T ) bbox.ymax ) {}

    /** \~french
     * \brief Reprojette le rectangle englobant (SRS sous forme de chaîne de caractères)
     * \details Pour reprojeter la bounding box, on va découper chaque côté du rectangle en N, et identifier les extrema parmi ces 4*N points reprojetés.
     * \param[in] from_srs système spatial source, celui du rectangle englobant initialement
     * \param[in] to_srs système spatial de destination, celui dans lequel on veut le rectangle englobant
     * \param[in] nbSegment nombre de points intérmédiaire à reprojeter sur chaque bord. 256 par défaut.
     * \return code de retour, 0 si succès, 1 sinon.
     */
    int reproject ( std::string from_srs, std::string to_srs , int nbSegment = 256 ) {

        pthread_mutex_lock ( & mutex_proj );

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

        int err = reproject ( pj_src, pj_dst, nbSegment );

        pj_free ( pj_src );
        pj_free ( pj_dst );
        pj_ctx_free ( ctx );
        pthread_mutex_unlock ( & mutex_proj );

        return err;
    }

    /** \~french
     * \brief Reprojette le rectangle englobant (SRS sous forme d'objets PROJ)
     * \details Pour reprojeter la bounding box, on va découper chaque côté du rectangle en N, et identifier les extrema parmi ces 4*N points reprojetés.
     * \param[in] pj_src système spatial source, celui du rectangle englobant initialement
     * \param[in] pj_dst système spatial de destination, celui dans lequel on veut le rectangle englobant
     * \param[in] nbSegment nombre de points intérmédiaire à reprojeter sur chaque bord. 256 par défaut.
     * \return code de retour, 0 si succès, 1 sinon.
     */
    int reproject ( projPJ pj_src, projPJ pj_dst, int nbSegment = 256 ) {

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

    /** \~french \brief Sortie des informations sur le rectangle englobant
     ** \~english \brief Bounding box description output
     */
    void print() {
        LOGGER_DEBUG ( "BBOX = " << xmin << " " << ymin << " " << xmax << " " << ymax );
    }

    /** \~french \brief Conversion des informations sur le rectangle englobant en string
     * \return chaîne de carcactère décrivant le rectangle englobant
     ** \~english \brief Convert bounding box description to string
     * \return string describing the bounding box
     */
    std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << xmin << "," << ymin << "," << xmax << "," << ymax;
        return oss.str() ;
    }

    /** \~french \brief Détermine si deux bounding box s'intersectent
     * \param[in] bbox rectangle englobant avec lequel tester l'intersection
     ** \~english \brief Determine if 2 bounding box intersect each other
     * \param[in] bbox bounding box which whom we have to test intersection
     */
    bool intersects ( BoundingBox<T> bbox ) {
        return ! ( xmax < bbox.xmin || bbox.xmax < xmin || ymax < bbox.ymin || bbox.ymax < ymin );
    }

    /** \~french \brief Détermine une bounding box contient l'autre
     * \param[in] bbox rectangle englobant dont on veut savoir s'il est contenu dans l'autre
     ** \~english \brief Determine if a bounding box contains the other
     * \param[in] bbox bounding box : is it contained by the other ?
     */
    bool contains ( BoundingBox<T> bbox ) {
        return ( xmin < bbox.xmin && bbox.xmax < xmax && ymin < bbox.ymin && bbox.ymax < ymax );
    }
    /** \~french \brief Détermine une bounding box contient l'autre ou touche intérieurement
     * \param[in] bbox rectangle englobant dont on veut savoir s'il est contenu dans l'autre
     ** \~english \brief Determine if a bounding box contains the other or touch inside
     * \param[in] bbox bounding box : is it contained by the other ?
     */
    bool containsInside ( BoundingBox<T> bbox ) {
        return ( xmin <= bbox.xmin && bbox.xmax <= xmax && ymin <= bbox.ymin && bbox.ymax <= ymax );
    }

    /** \~french \brief Récupère la partie qui intersecte la bbox donnée en paramètre
     * \param[in] bbox rectangle
     ** \~english \brief Get the part intersected the bbox given in parameter
     * \param[in] bbox bounding box
     */
    BoundingBox<T> cutIntersectionWith ( BoundingBox<T> bbox ) {

        if (this->xmin > bbox.xmin && this->xmax < bbox.xmax && this->ymin < bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (this->xmin,bbox.ymin,this->xmax,bbox.ymax);
        }
        if (this->xmin < bbox.xmin && this->xmax > bbox.xmax && this->ymin > bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,this->ymin,bbox.xmax,this->ymax);
        }

        if (this->xmin > bbox.xmin && this->xmax > bbox.xmax && this->ymin > bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (this->xmin,this->ymin,bbox.xmax,this->ymax);
        }
        if (this->xmin < bbox.xmin && this->xmax < bbox.xmax && this->ymin > bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,this->ymin,this->xmax,this->ymax);
        }
        if (this->xmin > bbox.xmin && this->xmax < bbox.xmax && this->ymin > bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (this->xmin,this->ymin,this->xmax,bbox.ymax);
        }
        if (this->xmin > bbox.xmin && this->xmax < bbox.xmax && this->ymin < bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (this->xmin,bbox.ymin,this->xmax,this->ymax);
        }

        if (this->xmin < bbox.xmin && this->xmax > bbox.xmax && this->ymin > bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,this->ymin,bbox.xmax,bbox.ymax);
        }
        if (this->xmin < bbox.xmin && this->xmax < bbox.xmax && this->ymin < bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,bbox.ymin,this->xmax,bbox.ymax);
        }
        if (this->xmin < bbox.xmin && this->xmax > bbox.xmax && this->ymin < bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,bbox.ymin,bbox.xmax,this->ymax);
        }
        if (this->xmin > bbox.xmin && this->xmax > bbox.xmax && this->ymin < bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (this->xmin,bbox.ymin,bbox.xmax,bbox.ymax);
        }

        if (this->xmin < bbox.xmin && this->xmax < bbox.xmax && this->ymin > bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,this->ymin,this->xmax,bbox.ymax);
        }
        if (this->xmin < bbox.xmin && this->xmax < bbox.xmax && this->ymin < bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (bbox.xmin,bbox.ymin,this->xmax,this->ymax);
        }
        if (this->xmin > bbox.xmin && this->xmax > bbox.xmax && this->ymin < bbox.ymin && this->ymax < bbox.ymax) {
            return BoundingBox<T> (this->xmin,bbox.ymin,bbox.xmax,this->ymax);
        }
        if (this->xmin > bbox.xmin && this->xmax > bbox.xmax && this->ymin > bbox.ymin && this->ymax > bbox.ymax) {
            return BoundingBox<T> (this->xmin,this->ymin,bbox.xmax,bbox.ymax);
        }

    }

    /** \~french \brief Récupère la partie utile de la bbox qui appelle la fonction, en fonction de la bbox en parametre
     * ATTENTION: on part du principe que les bbox sont dans le même CRS
     * \param[in] bbox
     ** \~english \brief Get the useful part of the bbox which call the function, depending of the parameter bbox
     ** WARNING: the two bbox must have the same CRS
     * \param[in] bbox
     */
    BoundingBox<T> adaptTo ( BoundingBox<T> dataBbox ) {

        BoundingBox<T> askBbox = BoundingBox<T>(this->xmin,this->ymin,this->xmax,this->ymax);

        if (askBbox.containsInside(dataBbox)) {
            //les données sont a l'intérieur de la bbox demandée
            LOGGER_DEBUG ( "les données sont a l'intérieur de la bbox demandée " );
            return dataBbox;

        } else {

            if (dataBbox.containsInside(askBbox)) {
                //la bbox demandée est plus petite que les données disponibles
                LOGGER_DEBUG ("la bbox demandée est plus petite que les données disponibles");
                return askBbox;

            } else {

                if (!dataBbox.intersects(askBbox)) {
                    //les deux ne s'intersectent pas donc on renvoit une image de nodata
                    LOGGER_DEBUG ("les deux ne s'intersectent pas");
                    return BoundingBox<T> (0,0,0,0);

                } else {
                    //les deux s'intersectent
                    LOGGER_DEBUG ("les deux bbox s'intersectent");
                    return askBbox.cutIntersectionWith(dataBbox);
                }

            }

        }

    }

    /** \~french \brief Détermine si une boundingBox est égale à une autre
     * \param[in] bbox
     ** \~english \brief Determine if a bounding box is equal to an other
     * \param[in] bbox
     */
    bool isEqual ( BoundingBox<T> bbox ) {
        return ( xmin == bbox.xmin && bbox.xmax == xmax && ymin == bbox.ymin && bbox.ymax == ymax );
    }

    /** \~french \brief Détermine si une boundingBox est nulle
     ** \~english \brief Determine if a bounding box is null
     */
    bool isNull ( ) {
        return ( xmin == 0 && xmax == 0 && ymin == 0 && ymax == 0 );
    }


};
#endif

