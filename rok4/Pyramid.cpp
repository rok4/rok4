/*
 * Copyright © (2011-2013) Institut national de l'information
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
#include "Format.h"
#include "EmptyImage.h"
#include "TiffEncoder.h"
#include "Level.h"
#include <cfloat>
#include "intl.h"
#include "config.h"
#include "EmptyImage.h"

Pyramid::Pyramid (std::map<std::string, Level*> &levels, TileMatrixSet tms, Rok4Format::eformat_data format,
                   int channels, bool onDemand, bool onFly)
    : levels ( levels ), tms ( tms ), format ( format ), channels ( channels ),
      onDemand ( onDemand ), onFly (onFly) {

    std::map<std::string, Level*>::iterator itLevel;
    double minRes= DBL_MAX;
    double maxRes= DBL_MIN;
    for ( itLevel=levels.begin(); itLevel!=levels.end(); itLevel++ ) {
        //Empty Source as fallback
        DataSource* noDataSource;
        DataStream* nodatastream;

        if ( format==Rok4Format::TIFF_JPG_INT8 ) {
            nodatastream = new JPEGEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ) );
        } else if ( format==Rok4Format::TIFF_PNG_INT8 ) {
            nodatastream = new PNGEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ) );
        } else if ( format==Rok4Format::TIFF_RAW_FLOAT32 ) {
            nodatastream = new BilEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ) );
        } else {
            nodatastream = TiffEncoder::getTiffEncoder ( new ImageDecoder ( 0, itLevel->second->getTm().getTileW(), itLevel->second->getTm().getTileH(), channels ), format );
        }
        if ( nodatastream ) {
            noDataSource = new BufferedDataSource ( *nodatastream );
            delete nodatastream;
            nodatastream = NULL;
        } else {
            LOGGER_ERROR ( "Format non pris en charge : "<< Rok4Format::toString ( format ) );
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
    int i = 0;

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

std::string Pyramid::best_level ( double resolution_x, double resolution_y, bool onDemand ) {

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

    if (onDemand) {

        if (best <= 1.8 && best >= 0.8) {
            return best_h;
        } else {
            return "";
        }

    } else {
        return best_h;
    }

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

    LOGGER_DEBUG ( "source tms.getCRS() is " << tms.getCrs().getProj4Code() << " and destination dst_crs is " << dst_crs.getProj4Code() );

    if ( (tms.getCrs() == dst_crs) || (are_the_two_CRS_equal( tms.getCrs().getProj4Code(), dst_crs.getProj4Code(), servicesConf.getListOfEqualsCRS() ) ) ) {
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

    std::string l = best_level ( resolution_x, resolution_y, false );
    LOGGER_DEBUG ( _ ( "best_level=" ) << l << _ ( " resolution requete=" ) << resolution_x << " " << resolution_y );

    if ( (tms.getCrs() == dst_crs) || (are_the_two_CRS_equal( tms.getCrs().getProj4Code(), dst_crs.getProj4Code(), servicesConf.getListOfEqualsCRS() ) ) ) {
        return levels[l]->getbbox ( servicesConf, bbox, width, height, interpolation, error );
    } else {
        return createReprojectedImage(l, bbox, dst_crs, servicesConf, width, height, interpolation, error);
    }

}

Image * Pyramid::createReprojectedImage(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesConf& servicesConf, int width, int height, Interpolation::KernelType interpolation, int error) {

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
            int newWidth = lround(width * ratio_x);
            int newHeigth = lround(height * ratio_y);


    //Avec lround, taille en pixel et cropBBox ne sont plus cohérents.
    //On ajoute la différence de l'arrondi dans la cropBBox et on ajoute un pixel en plus tout autour.

    //Calcul de l'erreur d'arrondi converti en coordonnées
            double delta_h = double (newHeigth) - double(height) * ratio_y ;
            double delta_w = double (newWidth) - double(width) * ratio_x ;

            double res_y = ( cropBBox.ymax - cropBBox.ymin ) / double(height * ratio_y) ;
            double res_x = ( cropBBox.xmax - cropBBox.xmin ) / double(width * ratio_x) ;

            double delta_y = res_y * delta_h ;
            double delta_x = res_x * delta_w ;

    //Ajout de l'erreur d'arrondi et le pixel en plus
            cropBBox.ymax += delta_y +res_y;
            cropBBox.ymin -= res_y;

            cropBBox.xmax += delta_x +res_x;
            cropBBox.xmin -= res_x;

            newHeigth += 2;
            newWidth += 2;

            LOGGER_DEBUG ( _ ( "New Width = " ) << newWidth << " " << _ ( "New Height = " ) << newHeigth );
            LOGGER_DEBUG ( _ ( "ratio_x = " ) << ratio_x << " " << _ ( "ratio_y = " ) << ratio_y );


            Image* tmp = 0;
            int cropError = 0;
            if ( (1/ratio_x > 5 && newWidth < 3) || (newHeigth < 3 && 1/ratio_y > 5) ){ //Too small BBox
                LOGGER_DEBUG ( _ ( "BBox decoupe incorrect" ) );
                tmp = 0;
            } else if ( newWidth > 0 && newHeigth > 0 ) {
                tmp = levels[l]->getbbox ( servicesConf, cropBBox, newWidth, newHeigth, tms.getCrs(), dst_crs, interpolation, cropError );
            }
            if ( tmp != 0 ) {
                LOGGER_DEBUG ( _ ( "Image decoupe valide" ) );
                images.push_back ( tmp );
            }
        }

        int ndvalue[this->channels];
        memset(ndvalue,0,this->channels*sizeof(int));
        levels[l]->getNoDataValue(ndvalue);

        if ( images.empty() ) {
            EmptyImage* fond = new EmptyImage(width, height, channels, ndvalue);
            fond->setBbox(bbox);
            return fond;
        }

        return facto.createExtendedCompoundImage ( width,height,channels,bbox,images,ndvalue,0 );
    }

}

Image *Pyramid::NoDataOnDemand(std::string bLevel, BoundingBox<double> bbox) {

    return levels[bLevel]->getNoDataTile(bbox);

}

Pyramid::~Pyramid() {
    std::map<std::string, DataSource*>::iterator itDataSource;
    /*for ( itDataSource=noDataSources.begin(); itDataSource!=noDataSources.end(); itDataSource++ )
        delete ( *itDataSource ).second;*/

    std::map<std::string, Level*>::iterator iLevel;
    for ( iLevel=levels.begin(); iLevel!=levels.end(); iLevel++ )
        delete ( *iLevel ).second;

    //If it is a pyramid on demand
    //So we need to delete pointers to other pyramids
    if (onDemand) {
        for (int i=0;i<basedPyramids.size();i++) {
            basedPyramids.at(i)->~Pyramid();
            delete basedPyramids.at(i);
            basedPyramids.at(i) = NULL;
        }
    }
}

// Check if two CRS are equivalent
//   A list of equivalent CRS was created during server initialization
// TODO: return false if servicesconf tells we don't check the equality
bool Pyramid::are_the_two_CRS_equal( std::string crs1, std::string crs2, std::vector<std::string> listofequalsCRS ) {
    // Could have issues with lowercase name -> we put the CRS in upercase
    transform(crs1.begin(), crs1.end(), crs1.begin(), toupper);
    transform(crs2.begin(), crs2.end(), crs2.begin(), toupper);
    for (int line_number = 0 ; line_number < listofequalsCRS.size() ; line_number++) {
        std::string line = listofequalsCRS.at(line_number);
        // We check if the two CRS are on the same line inside the file. If yes then they are equivalent.
        std::size_t found1 = line.find(crs1);
        if ( found1 != std::string::npos  )  {
            std::size_t found2 = line.find(crs2);
            if ( found2 != std::string::npos  )  {
                LOGGER_DEBUG ( "The two CRS (source and destination) are equals and were found on line  " << line );
                return true;
            }
        }
    }
    return false; // The 2 CRS were not found on the same line inside the list
}

Compression::eCompression Pyramid::getSampleCompression() {
    Compression::eCompression cpn = Compression::UNKNOWN;
    if (format == Rok4Format::UNKNOWN) {
        cpn = Compression::UNKNOWN;
    }
    if (format == Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_RAW_FLOAT32) {
        cpn = Compression::NONE;
    }
    if (format == Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_ZIP_FLOAT32) {
        cpn = Compression::DEFLATE;
    }
    if (format == Rok4Format::TIFF_JPG_INT8) {
        cpn = Compression::JPEG;
    }
    if (format == Rok4Format::TIFF_PNG_INT8) {
        cpn = Compression::PNG;
    }
    if (format == Rok4Format::TIFF_LZW_INT8 || format == Rok4Format::TIFF_LZW_FLOAT32) {
        cpn = Compression::LZW;
    }
    if (format == Rok4Format::TIFF_PKB_INT8 || format == Rok4Format::TIFF_PKB_FLOAT32) {
        cpn = Compression::PACKBITS;
    }
    return cpn;
}

SampleFormat::eSampleFormat Pyramid::getSampleFormat() {
    SampleFormat::eSampleFormat sft = SampleFormat::UNKNOWN;
    if (format == Rok4Format::UNKNOWN) {
        sft = SampleFormat::UNKNOWN;
    }
    if (format == Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_JPG_INT8
            || format == Rok4Format::TIFF_PNG_INT8 || format == Rok4Format::TIFF_LZW_INT8
            || format == Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_PKB_INT8) {
        sft = SampleFormat::UINT;
    }
    if (format == Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32
            || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32) {
        sft = SampleFormat::FLOAT;
    }

    return sft;

}

int Pyramid::getBitsPerSample() {
    int bits = 8;
    if (format == Rok4Format::UNKNOWN) {
        bits = 8;
    }
    if (format == Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_JPG_INT8
            || format == Rok4Format::TIFF_PNG_INT8 || format == Rok4Format::TIFF_LZW_INT8
            || format == Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_PKB_INT8) {
        bits = 8;
    }
    if (format == Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32
            || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32) {
        bits = 32;
    }

    return bits;

}

bool Pyramid::isThisLevelSpecific(std::string lv) {

    bool specific = false;

    if (getSPyramids().size() !=0) {
        std::map<std::string,std::vector<Pyramid*> > spyr = getSPyramids();
        std::map<std::string,std::vector<Pyramid*> >::iterator sp = spyr.find(lv);
        if (sp != spyr.end()) {
            specific = true;
        }
    }

    return specific;


}

std::vector<Pyramid*> Pyramid::getSourcePyramid ( std::string lv,bool sp ) {

    std::vector<Pyramid*> bPyr;

    if (!sp) {
      bPyr = getBPyramids();
    } else {
        bPyr = getSPyramids().find(lv)->second;
    }

    return bPyr;
}

