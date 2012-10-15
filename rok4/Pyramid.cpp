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

#include <cmath>
#include "Pyramid.h"
#include "Logger.h"
#include "Message.h"
#include "Grid.h"
#include "Decoder.h"
#include "JPEGEncoder.h"
#include "PNGEncoder.h"
#include "TiffEncoder.h"
#include "BilEncoder.h"
#include "ExtendedCompoundImage.h"
#include "TiffEncoder.h"
#include "Level.h"
#include <cfloat>
#include "intl.h"
#include "config.h"

Pyramid::Pyramid ( std::map<std::string, Level*> &levels, TileMatrixSet tms, eformat_data format, int channels ) : levels ( levels ), tms ( tms ), format ( format ), channels ( channels ) {
    std::map<std::string, Level*>::iterator itLevel;
    double minRes= DBL_MAX;
    double maxRes= DBL_MIN;
    for ( itLevel=levels.begin(); itLevel!=levels.end(); itLevel++ ) {
        //Empty Source as fallback
        DataSource* noDataSource;
        DataStream* nodatastream;

        if ( format==TIFF_JPG_INT8 ) {
            nodatastream = new JPEGEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ) );
        } else if ( format==TIFF_PNG_INT8 ) {
            nodatastream = new PNGEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ) );
        } else if ( format==TIFF_RAW_FLOAT32 ) {
            nodatastream = new BilEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ) );
        } else {
            nodatastream = TiffEncoder::getTiffEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ), format );
        }
        if ( nodatastream ) {
            noDataSource = new BufferedDataSource ( *nodatastream );
            delete nodatastream;
            nodatastream = NULL;
        } else {
            LOGGER_ERROR ( "Format non pris en charge : "<< format::toString ( format ) );
        }
        itLevel->second->setNoDataSource ( noDataSource );

        //Determine Higher and Lower Levels
        double d = itLevel->second->getRes();
        if ( minRes > d ) {
            minRes = d;
            lowestLevel = itLevel->second;
        }
        if ( maxRes < d ) {
            maxRes = d;
            highestLevel = itLevel->second;
        }
    }

}

DataSource* Pyramid::getTile ( int x, int y, std::string tmId, DataSource* errorDataSource ) {

    std::map<std::string, Level*>::const_iterator itLevel=levels.find ( tmId );
    if ( itLevel==levels.end() ) {
        if ( errorDataSource ) { // NoData Error
            return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * errorDataSource );
        }
        DataSource * noDataSource;

        //Pick the nearest available level for NoData
        std::map<std::string, TileMatrix>::iterator itTM;
        double askedRes;

        itTM = getTms().getTmList()->find ( tmId );
        if ( itTM==getTms().getTmList()->end() ) {
            //return the lowest Level available
            noDataSource = getLowestLevel()->getEncodedNoDataTile();
        } else {
            askedRes = itTM->second.getRes();
            noDataSource = ( askedRes > getLowestLevel()->getRes() ? getHighestLevel()->getEncodedNoDataTile() : getLowestLevel()->getEncodedNoDataTile() );
        }
        return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( noDataSource ) );
    }
    return itLevel->second->getTile ( x, y, errorDataSource );
}

std::string Pyramid::best_level ( double resolution_x, double resolution_y ) {

    // TODO: A REFAIRE !!!!
    // res_level/resx ou resy ne doit pas exceder une certaine valeur
    double resolution = sqrt ( resolution_x * resolution_y );

    std::map<std::string, Level*>::iterator it ( levels.begin() ), itend ( levels.end() );
    std::string best_h = it->first;
    double best = resolution_x / it->second->getRes();
    ++it;
    for ( ; it!=itend; ++it ) {
        double d = resolution / it->second->getRes();
        if ( ( best < 0.8 && d > best ) ||
                ( best >= 0.8 && d >= 0.8 && d < best ) ) {
            best = d;
            best_h = it->first;
        }
    }
    return best_h;
}


Level * Pyramid::getFirstLevel() {
    std::map<std::string, Level*>::iterator it ( levels.begin() );
    return it->second;
}

TileMatrixSet Pyramid::getTms() {
    return tms;
}


Image* Pyramid::getbbox ( ServicesConf& servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, Interpolation::KernelType interpolation, int& error ) {
    // On calcule la résolution de la requete dans le crs source selon une diagonale de l'image
    double resolution_x, resolution_y;
    if ( tms.getCrs() == dst_crs ) {
        resolution_x = ( bbox.xmax - bbox.xmin ) / width;
        resolution_y = ( bbox.ymax - bbox.ymin ) / height;
    } else {
        Grid* grid = new Grid ( width, height, bbox );


        LOGGER_DEBUG ( _ ( "debut pyramide" ) );
        if ( !grid->reproject ( dst_crs.getProj4Code(),getTms().getCrs().getProj4Code() ) ) {
            // BBOX invalide
            error=1;
            return 0;
        }
        LOGGER_DEBUG ( _ ( "fin pyramide" ) );

        resolution_x = ( grid->bbox.xmax - grid->bbox.xmin ) / width;
        resolution_y = ( grid->bbox.ymax - grid->bbox.ymin ) / height;
        delete grid;
    }
    std::string l = best_level ( resolution_x, resolution_y );
    LOGGER_DEBUG ( _ ( "best_level=" ) << l << _ ( " resolution requete=" ) << resolution_x << " " << resolution_y );
    if ( tms.getCrs() == dst_crs ) {
        return levels[l]->getbbox ( servicesConf, bbox, width, height, interpolation, error );
    } else {
        if ( dst_crs.validateBBox ( bbox ) ) {
            return levels[l]->getbbox ( servicesConf, bbox, width, height, tms.getCrs(), dst_crs, interpolation, error );
        } else {
            ExtendedCompoundImageFactory facto;
            std::vector<Image*> images;
            LOGGER_DEBUG ( _ ( "BBox en dehors de la definition du CRS" ) );
            BoundingBox<double> cropBBox = dst_crs.cropBBox ( bbox );

            if ( cropBBox.xmin == cropBBox.xmax || cropBBox.ymin == cropBBox.ymax ) { // BBox out of CRS definition area Only NoData
                LOGGER_DEBUG ( _ ( "BBox decoupe incorrect" ) );
            } else {

                double ratio_x = ( cropBBox.xmax - cropBBox.xmin ) / ( bbox.xmax - bbox.xmin );
                double ratio_y = ( cropBBox.ymax - cropBBox.ymin ) / ( bbox.ymax - bbox.ymin ) ;
                int newWidth = width * ratio_x;
                int newHeigth = height * ratio_y;
                LOGGER_DEBUG ( _ ( "New Width = " ) << newWidth << " " << _ ( "New Height = " ) << newHeigth );
                Image* tmp = 0;
                int cropError = 0;
                if ( newWidth > 0 && newHeigth > 0 ) {
                    tmp = levels[l]->getbbox ( servicesConf, cropBBox, newWidth, newHeigth, tms.getCrs(), dst_crs, interpolation, cropError );
                }
                if ( tmp != 0 ) {
                    LOGGER_DEBUG ( _ ( "Image decoupe valide" ) );
                    images.push_back ( tmp );
                }
            }

            if ( images.empty() ) {
                images.push_back ( levels[l]->getNoDataTile ( bbox ) );
            }
            int ndvalue[this->channels];
            memset(ndvalue,0,this->channels*sizeof(int));
            levels[l]->getNoDataValue(ndvalue);
            return facto.createExtendedCompoundImage ( width,height,channels,bbox,images,ndvalue,
                                                       levels[l]->getSampleFormat(),0);
        }

    }
}

Pyramid::~Pyramid() {
    std::map<std::string, DataSource*>::iterator itDataSource;
    /*for ( itDataSource=noDataSources.begin(); itDataSource!=noDataSources.end(); itDataSource++ )
        delete ( *itDataSource ).second;*/

    std::map<std::string, Level*>::iterator iLevel;
    for ( iLevel=levels.begin(); iLevel!=levels.end(); iLevel++ )
        delete ( *iLevel ).second;
}

