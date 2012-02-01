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

/*
* @fn std::string extern getMimeType(std::string format)
* @return Le type MIME correspindant au format passe en argument
*/

/*std::string getMimeType(std::string format){
        if (format.compare("TIFF_RAW_INT8")==0)
                return "image/tiff";
        else if (format.compare("TIFF_JPG_INT8")==0)
                return "image/jpeg";
        else if (format.compare("TIFF_PNG_INT8")==0)
                return "image/png";
        else if (format.compare("TIFF_RAW_FLOAT32")==0)
                return "image/x-bil;bits=32";
        return "text/plain";
}*/

Pyramid::Pyramid ( std::map<std::string, Level*> &levels, TileMatrixSet tms, eformat_data format, int channels ) : levels ( levels ), tms ( tms ), format ( format ), channels ( channels ) {

    std::map<std::string, TileMatrix>::iterator itTm;
    for ( itTm=tms.getTmList()->begin();itTm!=tms.getTmList()->end();itTm++ ) {
        //Empty Source as fallback
        DataSource* noDataSource;
        if ( format==TIFF_RAW_INT8 ) {
            TiffEncoder dataStream ( new ImageDecoder ( 0, itTm->second.getTileW(), itTm->second.getTileH(), channels ) );
            noDataSource = new BufferedDataSource ( dataStream );
        } else if ( format==TIFF_JPG_INT8 ) {
            JPEGEncoder dataStream ( new ImageDecoder ( 0, itTm->second.getTileW(), itTm->second.getTileH(), channels ) );
            noDataSource = new BufferedDataSource ( dataStream );
        } else if ( format==TIFF_PNG_INT8 ) {
            PNGEncoder dataStream ( new ImageDecoder ( 0, itTm->second.getTileW(), itTm->second.getTileH(), channels ) );
            noDataSource = new BufferedDataSource ( dataStream );
        } else if ( format==TIFF_RAW_FLOAT32 ) {
            BilEncoder dataStream ( new ImageDecoder ( 0, itTm->second.getTileW(), itTm->second.getTileH(), channels ) );
            noDataSource = new BufferedDataSource ( dataStream );
        } else
            // Cas normalement filtre avant l'appel au constructeur
            LOGGER_ERROR ( "Format non pris en charge : "<< format::toString ( format ) );
        noDataSources.insert ( std::pair<std::string, DataSource*> ( itTm->second.getId(), noDataSource ) );

        std::map<std::string, Level*>::const_iterator itLevel=levels.find ( itTm->second.getId() );
        if ( itLevel!=levels.end() )
            itLevel->second->setNoDataSource ( noDataSource );
    }
}

DataSource* Pyramid::getTile ( int x, int y, std::string tmId ) {

    std::map<std::string, Level*>::const_iterator itLevel=levels.find ( tmId );
    if ( itLevel==levels.end() ) {
        std::map<std::string, DataSource*>::const_iterator itNoDataSource=noDataSources.find ( tmId );
        if ( itNoDataSource!=noDataSources.end() )
            return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( itNoDataSource->second ) );
        else {
            LOGGER_ERROR ( "pas de nodata disponible pour le TM "<<tmId );
            return 0;
        }
    }
    return itLevel->second->getTile ( x, y );
}

std::string Pyramid::best_level ( double resolution_x, double resolution_y ) {

    // TODO: A REFAIRE !!!!
    // res_level/resx ou resy ne doit pas exceder une certaine valeur
    double resolution = sqrt ( resolution_x * resolution_y );

    std::map<std::string, Level*>::iterator it ( levels.begin() ), itend ( levels.end() );
    std::string best_h = it->first;
    double best = resolution_x / it->second->getRes();
    ++it;
    for ( ;it!=itend;++it ) {
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


Image* Pyramid::getbbox ( BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error ) {
    // On calcule la résolution de la requete dans le crs source selon une diagonale de l'image
    double resolution_x, resolution_y;
    if ( getTms().getCrs() ==dst_crs ) {
        resolution_x = ( bbox.xmax - bbox.xmin ) / width;
        resolution_y = ( bbox.ymax - bbox.ymin ) / height;
    } else {
        Grid* grid = new Grid ( width, height, bbox );


        LOGGER_DEBUG ( "debut pyramide" );
        if ( !grid->reproject ( dst_crs.getProj4Code(),getTms().getCrs().getProj4Code() ) ) {
            // BBOX invalide
            error=1;
            return 0;
        }
        LOGGER_DEBUG ( "fin pyramide" );

        resolution_x = ( grid->bbox.xmax - grid->bbox.xmin ) / width;
        resolution_y = ( grid->bbox.ymax - grid->bbox.ymin ) / height;
        delete grid;
    }
    std::string l = best_level ( resolution_x, resolution_y );
    LOGGER_DEBUG ( "best_level=" << l << " resolution requete=" << resolution_x << " " << resolution_y );

    if ( getTms().getCrs() ==dst_crs )
        return levels[l]->getbbox ( bbox, width, height );
    else
        return levels[l]->getbbox ( bbox, width, height, getTms().getCrs(), dst_crs );

}

Pyramid::~Pyramid() {
    std::map<std::string, DataSource*>::iterator itDataSource;
    for ( itDataSource=noDataSources.begin();itDataSource!=noDataSources.end();itDataSource++ )
        delete ( *itDataSource ).second;
}

