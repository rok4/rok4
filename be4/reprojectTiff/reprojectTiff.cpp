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
 * \file reprojectTiff.cpp
 */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include "tiffio.h"
#include "Logger.h"
#include "LibtiffImage.h"
#include "ReprojectedImage.h"
#include "ExtendedCompoundImage.h"
#include "MirrorImage.h"
#include "Interpolation.h"
#include "CRS.h"
#include "Format.h"
#include "math.h"
#include "../be4version.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

int main ( int argc, char **argv ) {

    LibtiffImage* image_src;
    LibtiffImage* mask_src;
    LibtiffImage* image_out;
    LibtiffImage* mask_out;
    ReprojectedImage* reprojImage;
    ReprojectedImage* reprojMask;
    
    /*********************************************************************/
    
    /* Initialisation des Loggers */
    Logger::setOutput ( STANDARD_OUTPUT_STREAM_FOR_ERRORS );

    Accumulator* acc = new StreamAccumulator();
    Logger::setAccumulator ( DEBUG, acc );
    Logger::setAccumulator ( INFO , acc );
    Logger::setAccumulator ( WARN , acc );
    Logger::setAccumulator ( ERROR, acc );
    Logger::setAccumulator ( FATAL, acc );

    std::ostream &logd = LOGGER ( DEBUG );
    logd.precision ( 16 );
    logd.setf ( std::ios::fixed,std::ios::floatfield );

    std::ostream &logw = LOGGER ( WARN );
    logw.precision ( 16 );
    logw.setf ( std::ios::fixed,std::ios::floatfield );


    bool useMask = false;
    bool writeMask = false;

    LOGGER_INFO ( "IMPORTANT : fonction de test de la classe ReprojectedImage avec masque de donnée" );

    CRS SRS_src ( "IGNF:LAMB93" );
    BoundingBox<double> BBOX_src ( 600000., 6030000., 1150000., 6580000. );
    double resX_src = 100.;
    double resY_src = 100.;

    //CRS SRS_dst ( "EPSG:3857" );
    CRS SRS_dst ( "IGNF:LAMB93" );
    BoundingBox<double> BBOX_dst ( 700000, 6280000, 900000, 6480000 );
    //BoundingBox<double> BBOX_dst ( 700000., 5350000, 900000., 5550000. );
    //BoundingBox<double> BBOX_dst (500000., 5500000, 700000., 5700000.);
    int width_dst = 2000;
    int height_dst = 2000;

    double resX_dst = ( BBOX_dst.xmax - BBOX_dst.xmin ) / width_dst;
    double resY_dst = ( BBOX_dst.ymax - BBOX_dst.ymin ) / height_dst;

    /*********************************************************************/

    LOGGER_DEBUG ( "On crée l'image source LibtiffImage" );
    LibtiffImageFactory factory;
    
    image_src = factory.createLibtiffImageToRead ( "/home/theo/TEST/reprojectTiff/sources/source.tif", BBOX_src, resX_src, resY_src );
    if ( ! image_src ) {
        LOGGER_ERROR ( "Impossible de charger l'image en lecture" );
        return 1;
    }

    if (useMask) {
        mask_src = factory.createLibtiffImageToRead("/home/theo/TEST/reprojectTiff/sources/source.msk", BBOX_src, resX_src, resY_src);
        if (! mask_src) {
            LOGGER_ERROR("Impossible de charger le masque en lecture");
            return 1;
        }

        if ( ! image_src->setMask ( mask_src ) ) {
            LOGGER_ERROR ( "Cannot add mask to the Resampled Image" );
            return 1;
        }
        LOGGER_INFO("pouet");
    }

    /*********************************************************************/

    LOGGER_DEBUG ( "On crée la grille de reprojection" );
    Grid* grid = new Grid ( width_dst, height_dst, BBOX_dst );
    grid->bbox.print();
    if ( ! ( grid->reproject ( SRS_dst.getProj4Code(), SRS_src.getProj4Code() ) ) ) {
        LOGGER_ERROR ( "Bbox image invalide" );
        return 1;
    }
    grid->bbox.print();
    /* On repasse la grille en coordonnées pixel dans l'image source
     * La grille nous dit ainsi tout de suite quel pixel de l'image source utiliser pour calculer tel pixel de l'image reprojetée
     */
    grid->affine_transform ( 1./resX_src, -image_src->getBbox().xmin/resX_src - 0.5,
                             -1./resY_src, image_src->getBbox().ymax/resY_src - 0.5 );
    grid->bbox.print();

    /*********************************************************************/

    LOGGER_DEBUG ( "On crée l'image reprojetée" );
    reprojImage = new ReprojectedImage ( image_src, BBOX_dst, grid, Interpolation::LANCZOS_4, useMask );

    if (writeMask) {
        LOGGER_DEBUG("On crée le masque reprojeté");
        reprojMask = new ReprojectedImage ( mask_src, BBOX_dst, grid, Interpolation::NEAREST_NEIGHBOUR );
    }

    /*********************************************************************/

    LOGGER_DEBUG ( "On crée l'image de sortie LibtiffImage" );
    image_out = factory.createLibtiffImageToWrite ( "/home/theo/TEST/reprojectTiff/reprojectedImage/ri_2_100_lcz.tif", BBOX_dst, resX_dst, resY_dst, width_dst, height_dst, 3, SampleType ( 8, SAMPLEFORMAT_UINT ), PHOTOMETRIC_RGB,COMPRESSION_NONE,1 );

    LOGGER_DEBUG ( "On écrit l'image" );
    if ( image_out->writeImage ( reprojImage ) < 0 ) {
        LOGGER_ERROR ( "Echec enregistrement de l image finale" );
        return 1;
    }

    if (writeMask) {
        LOGGER_DEBUG("On crée le masque de sortie LibtiffImage");
        mask_out = factory.createLibtiffImageToWrite("/home/theo/TEST/reprojectTiff/reprojectedImage/ri_2_100_lcz.msk", BBOX_dst, resX_dst, resY_dst, width_dst, height_dst, 1, SampleType(8, SAMPLEFORMAT_UINT), PHOTOMETRIC_MINISBLACK,COMPRESSION_NONE,1);

        LOGGER_DEBUG("On écrit le masque");
        if (mask_out->writeImage(reprojMask) < 0) {
            LOGGER_ERROR("Echec enregistrement du masque final");
            return 1;
        }
    }

    /*********************************************************************/

    LOGGER_DEBUG ( "Clean" );
    // Nettoyage
    delete acc;
    delete reprojImage;
    delete image_out;

    if (useMask) {
        delete reprojMask;
        if (writeMask) delete mask_out;
    }

    return 0;
}
