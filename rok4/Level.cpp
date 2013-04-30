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

#include "Level.h"
#include "Interpolation.h"
#include "FileDataSource.h"
#include "CompoundImage.h"
#include "ResampledImage.h"
#include "ReprojectedImage.h"
#include "RawImage.h"
#include "Decoder.h"
#include "TiffEncoder.h"
#include "TiffHeaderDataSource.h"
#include <cmath>
#include "Logger.h"
#include "Kernel.h"
#include <vector>
#include "Pyramid.h"
#include "PaletteDataSource.h"
#include "Format.h"
#include "intl.h"
#include "config.h"


#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.



Level::Level ( TileMatrix tm, int channels, std::string baseDir, int tilesPerWidth,
               int tilesPerHeight, uint32_t maxTileRow, uint32_t minTileRow,
               uint32_t maxTileCol, uint32_t minTileCol, int pathDepth,
               Format::eformat_data format, std::string noDataFile ) :
    tm ( tm ), channels ( channels ), baseDir ( baseDir ),
    tilesPerWidth ( tilesPerWidth ), tilesPerHeight ( tilesPerHeight ),
    maxTileRow ( maxTileRow ), minTileRow ( minTileRow ), maxTileCol ( maxTileCol ),
    minTileCol ( minTileCol ), pathDepth ( pathDepth ), format ( format ),noDataFile ( noDataFile ), noDataSource ( NULL ) {
    noDataTileSource = new FileDataSource ( noDataFile.c_str(),2048,2048+4, Format::toMimeType ( format ) );
    noDataSourceProxy = noDataTileSource;
}

Level::~Level() {

    delete noDataSourceProxy;
    if ( noDataSource )
        delete noDataSource;

}

void Level::setNoData ( const std::string& file ) {
    noDataFile=file;
    DataSource* tmpDataSource = new FileDataSource ( noDataFile.c_str(),2048,2048+4, Format::toMimeType ( format ) );
    if ( noDataTileSource ) {
        delete noDataTileSource;
    }

    if ( noDataSource ) {
        delete noDataSourceProxy;
        noDataSourceProxy = new DataSourceProxy ( tmpDataSource, *noDataSource );
    } else {
        noDataSourceProxy = tmpDataSource;
    }

    noDataTileSource= tmpDataSource;
}


void Level::setNoDataSource ( DataSource* source ) {
    if ( noDataSource ) {
        delete noDataSourceProxy;
        noDataTileSource = new FileDataSource ( noDataFile.c_str(),2048,2048+4, Format::toMimeType ( format ) );
    }
    noDataSource=source;
    noDataSourceProxy = new DataSourceProxy ( noDataTileSource, *noDataSource );
}


Image* Level::getnodatabbox ( ServicesConf& servicesConf, BoundingBox< double > bbox, int width, int height, Interpolation::KernelType interpolation, int& error ) {
//     Image* imageout = getNoDataTile (bbox);
    // On convertit les coordonnées en nombre de pixels depuis l'origine X0,Y0
    bbox.xmin = ( bbox.xmin - tm.getX0() ) /tm.getRes();
    bbox.xmax = ( bbox.xmax - tm.getX0() ) /tm.getRes();
    double tmp = bbox.ymin;
    bbox.ymin = ( tm.getY0() - bbox.ymax ) /tm.getRes();
    bbox.ymax = ( tm.getY0() - tmp ) /tm.getRes();


    //A VERIFIER !!!!
    BoundingBox<int64_t> bbox_int ( floor ( bbox.xmin + EPS ),
                                    floor ( bbox.ymin + EPS ),
                                    ceil ( bbox.xmax - EPS ),
                                    ceil ( bbox.ymax - EPS ) );

    // Rappel : les coordonnees de la bbox sont ici en pixels
    double res_x = ( bbox.xmax - bbox.xmin ) / width;
    double res_y = ( bbox.ymax - bbox.ymin ) / height;

    //Most efficient
    interpolation = Interpolation::LINEAR;
    const Kernel& kk = Kernel::getInstance ( interpolation );

    bbox_int.xmin = floor ( bbox.xmin - kk.size ( res_x ) );
    bbox_int.xmax = ceil ( bbox.xmax + kk.size ( res_x ) );
    bbox_int.ymin = floor ( bbox.ymin - kk.size ( res_y ) );
    bbox_int.ymax = ceil ( bbox.ymax + kk.size ( res_y ) );

    Image* imageout = getNoDataTile ( bbox );
    if ( !imageout ) {
        LOGGER_DEBUG ( _ ( "Image invalid !" ) );
        return 0;
    }

    LOGGER_DEBUG ( "Top 1" );
    return new ResampledImage ( imageout, width, height, res_x, res_y, bbox, false, interpolation);
}


/*
 * A REFAIRE
 */
Image* Level::getbbox ( ServicesConf& servicesConf, BoundingBox< double > bbox, int width, int height, CRS src_crs, CRS dst_crs, Interpolation::KernelType interpolation, int& error ) {
    Grid* grid = new Grid ( width, height, bbox );

    grid->bbox.print();

    if ( ! ( grid->reproject ( dst_crs.getProj4Code(), src_crs.getProj4Code() ) ) ) {
        error = 1; // BBox invalid
        return 0;
    }

    grid->bbox.print();

    // Calcul de la taille du noyau
    //Maintain previous Lanczos behaviour : Lanczos_2 for resampling and reprojecting
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_2;

    const Kernel& kk = Kernel::getInstance ( interpolation ); // Lanczos_2
    double ratio_x = ( grid->bbox.xmax - grid->bbox.xmin ) / ( tm.getRes() *double ( width ) );
    double ratio_y = ( grid->bbox.ymax - grid->bbox.ymin ) / ( tm.getRes() *double ( height ) );
    double bufx=kk.size ( ratio_x );
    double bufy=kk.size ( ratio_y );

    bufx<50?bufx=50:0;
    bufy<50?bufy=50:0; // Pour etre sur de ne pas regresser
    BoundingBox<int64_t> bbox_int ( floor ( ( grid->bbox.xmin - tm.getX0() ) /tm.getRes() - bufx ),
                                    floor ( ( tm.getY0() - grid->bbox.ymax ) /tm.getRes() - bufy ),
                                    ceil ( ( grid->bbox.xmax - tm.getX0() ) /tm.getRes() + bufx ),
                                    ceil ( ( tm.getY0() - grid->bbox.ymin ) /tm.getRes() + bufy ) );

    Image* image = getwindow ( servicesConf, bbox_int, error );
    if ( !image ) {
        LOGGER_DEBUG ( _ ( "Image invalid !" ) );
        return 0;
    }
    image->setBbox ( BoundingBox<double> ( tm.getX0() + tm.getRes() * bbox_int.xmin, tm.getY0() - tm.getRes() * bbox_int.ymax, tm.getX0() + tm.getRes() * bbox_int.xmax, tm.getY0() - tm.getRes() * bbox_int.ymin ) );

    grid->affine_transform ( 1./image->getResX(), -image->getBbox().xmin/image->getResX() - 0.5,
                             -1./image->getResY(), image->getBbox().ymax/image->getResY() - 0.5 );

    return new ReprojectedImage ( image, bbox, grid, interpolation );
}


Image* Level::getbbox ( ServicesConf& servicesConf, BoundingBox< double > bbox, int width, int height, Interpolation::KernelType interpolation, int& error ) {
    
    // On convertit les coordonnées en nombre de pixels depuis l'origine X0,Y0
    bbox.xmin = ( bbox.xmin - tm.getX0() ) /tm.getRes();
    bbox.xmax = ( bbox.xmax - tm.getX0() ) /tm.getRes();
    double tmp = bbox.ymin;
    bbox.ymin = ( tm.getY0() - bbox.ymax ) /tm.getRes();
    bbox.ymax = ( tm.getY0() - tmp ) /tm.getRes();

    //A VERIFIER !!!!
    BoundingBox<int64_t> bbox_int ( floor ( bbox.xmin + EPS ),
                                    floor ( bbox.ymin + EPS ),
                                    ceil ( bbox.xmax - EPS ),
                                    ceil ( bbox.ymax - EPS ) );

    if ( bbox_int.xmax - bbox_int.xmin == width && bbox_int.ymax - bbox_int.ymin == height &&
         bbox.xmin - bbox_int.xmin < EPS && bbox_int.xmax - bbox.xmax < EPS &&
         bbox.ymin - bbox_int.ymin < EPS && bbox_int.ymax - bbox.ymax < EPS ) {
        /* L'image demandée est en phase et à les mêmes résolutions que les images du niveau
         *   => pas besoin de réechantillonnage */
        return getwindow ( servicesConf, bbox_int, error );
    }

    // Rappel : les coordonnees de la bbox sont ici en pixels
    double ratio_x = ( bbox.xmax - bbox.xmin ) / width;
    double ratio_y = ( bbox.ymax - bbox.ymin ) / height;

    //Maintain previous Lanczos behaviour : Lanczos_3 for resampling only
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_3;
    const Kernel& kk = Kernel::getInstance ( interpolation ); // Lanczos_3

    // On en prend un peu plus pour ne pas avoir d'effet de bord lors du réechantillonnage
    bbox_int.xmin = floor ( bbox.xmin - kk.size ( ratio_x ) );
    bbox_int.xmax = ceil ( bbox.xmax + kk.size ( ratio_x ) );
    bbox_int.ymin = floor ( bbox.ymin - kk.size ( ratio_y ) );
    bbox_int.ymax = ceil ( bbox.ymax + kk.size ( ratio_y ) );

    Image* imageout = getwindow ( servicesConf, bbox_int, error );
    if ( !imageout ) {
        LOGGER_DEBUG ( _ ( "Image invalid !" ) );
        return 0;
    }
    
    LOGGER_DEBUG ( "Top 2" );
    // On affecte la bonne bbox à l'image source afin que la classe de réechantillonnage calcule les bonnes valeurs d'offset
    imageout->setBbox(BoundingBox<double>(bbox_int));
    
    return new ResampledImage ( imageout, width, height, ratio_x, ratio_y, bbox, false, interpolation );
}

int euclideanDivisionQuotient ( int64_t i, int n ) {
    int q=i/n;  // Division tronquee
    if ( q<0 ) q-=1;
    if ( q==0 && i<0 ) q=-1;
    return q;
}

int euclideanDivisionRemainder ( int64_t i, int n ) {
    int r=i%n;
    if ( r<0 ) r+=n;
    return r;
}

Image* Level::getwindow ( ServicesConf& servicesConf, BoundingBox< int64_t > bbox, int& error ) {
    int tile_xmin=euclideanDivisionQuotient ( bbox.xmin,tm.getTileW() );
    int tile_xmax=euclideanDivisionQuotient ( bbox.xmax -1,tm.getTileW() );
    int nbx = tile_xmax - tile_xmin + 1;
    if ( nbx >= servicesConf.getMaxTileX() ) {
        LOGGER_INFO ( _ ( "Too Much Tile on X axis" ) );
        error=2;
        return 0;
    }
    int tile_ymin=euclideanDivisionQuotient ( bbox.ymin,tm.getTileH() );
    int tile_ymax = euclideanDivisionQuotient ( bbox.ymax-1,tm.getTileH() );
    int nby = tile_ymax - tile_ymin + 1;
    if ( nby >= servicesConf.getMaxTileY() ) {
        LOGGER_INFO ( _ ( "Too Much Tile on Y axis" ) );
        error=2;
        return 0;
    }

    int left[nbx];
    memset ( left,   0, nbx*sizeof ( int ) );
    left[0]=euclideanDivisionRemainder ( bbox.xmin,tm.getTileW() );
    int top[nby];
    memset ( top,    0, nby*sizeof ( int ) );
    top[0]=euclideanDivisionRemainder ( bbox.ymin,tm.getTileH() );
    int right[nbx];
    memset ( right,  0, nbx*sizeof ( int ) );
    right[nbx - 1] = tm.getTileW() - euclideanDivisionRemainder ( bbox.xmax -1,tm.getTileW() ) -1;
    int bottom[nby];
    memset ( bottom, 0, nby*sizeof ( int ) );
    bottom[nby- 1] = tm.getTileH() - euclideanDivisionRemainder ( bbox.ymax -1,tm.getTileH() ) - 1;

    std::vector<std::vector<Image*> > T ( nby, std::vector<Image*> ( nbx ) );
    for ( int y = 0; y < nby; y++ )
        for ( int x = 0; x < nbx; x++ ) {
            T[y][x] = getTile ( tile_xmin + x, tile_ymin + y, left[x], top[y], right[x], bottom[y] );
        }

    if ( nbx == 1 && nby == 1 ) return T[0][0];
    else return new CompoundImage ( T );
}

/*
 * Tableau statique des caractères Base36 (pour systeme de fichier non case-sensitive)
 */
// static const char* Base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-";
static const char* Base36 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

/*
 * Recuperation du nom de fichier de la dalle du cache en fonction de son indice
 */
std::string Level::getFilePath ( int tilex, int tiley ) {
    // Cas normalement filtré en amont (exception WMS/WMTS)
    if ( tilex < 0 || tiley < 0 ) {
        LOGGER_ERROR ( _ ( "Indice de tuile negatif" ) );
        return "";
    }

    int x = tilex / tilesPerWidth;
    int y = tiley / tilesPerHeight;

    char path[32];
    path[sizeof ( path ) - 5] = '.';
    path[sizeof ( path ) - 4] = 't';
    path[sizeof ( path ) - 3] = 'i';
    path[sizeof ( path ) - 2] = 'f';
    path[sizeof ( path ) - 1] = 0;
    int pos = sizeof ( path ) - 6;

    for ( int d = 0; d < pathDepth; d++ ) {
        ;
        path[pos--] = Base36[y % 36];
        path[pos--] = Base36[x % 36];
        path[pos--] = '/';
        x = x / 36;
        y = y / 36;
    }
    do {
        path[pos--] = Base36[y % 36];
        path[pos--] = Base36[x % 36];
        x = x / 36;
        y = y / 36;
    } while ( x || y );
    path[pos] = '/';

    return baseDir + ( path + pos );
}

/*
 * @return la tuile d'indice (x,y) du niveau
 */

DataSource* Level::getEncodedTile ( int x, int y ) {
    // TODO: return 0 sur des cas d'erreur..
    // Index de la tuile (cf. ordre de rangement des tuiles)
    int n= ( y%tilesPerHeight ) *tilesPerWidth + ( x%tilesPerWidth );
    // Les index sont stockés à partir de l'octet 2048
    uint32_t posoff=2048+4*n, possize=2048+4*n +tilesPerWidth*tilesPerHeight*4;
    std::string path=getFilePath ( x, y );
    LOGGER_DEBUG ( path );
    return new FileDataSource ( path.c_str(),posoff,possize,Format::toMimeType ( format ) );
}

DataSource* Level::getDecodedTile ( int x, int y ) {
    DataSource* encData = new DataSourceProxy ( getEncodedTile ( x, y ),*getEncodedNoDataTile() );
    if ( format==Format::TIFF_RAW_INT8 || format==Format::TIFF_RAW_FLOAT32 )
        return encData;
    else if ( format==Format::TIFF_JPG_INT8 )
        return new DataSourceDecoder<JpegDecoder> ( encData );
    else if ( format==Format::TIFF_PNG_INT8 )
        return new DataSourceDecoder<PngDecoder> ( encData );
    else if ( format==Format::TIFF_LZW_INT8 || format == Format::TIFF_LZW_FLOAT32 )
        return new DataSourceDecoder<LzwDecoder> ( encData );
    else if ( format==Format::TIFF_ZIP_INT8 || format == Format::TIFF_ZIP_FLOAT32 )
        return new DataSourceDecoder<DeflateDecoder> ( encData );
    else if ( format==Format::TIFF_PKB_INT8 || format == Format::TIFF_PKB_FLOAT32 )
        return new DataSourceDecoder<PackBitsDecoder> ( encData );
    LOGGER_ERROR ( _ ( "Type d'encodage inconnu : " ) <<format );
    return 0;
}

DataSource* Level::getDecodedNoDataTile() {
    DataSource* encData = new DataSourceProxy ( new FileDataSource ( "",0,0,"" ),*getEncodedNoDataTile() );
    if ( format==Format::TIFF_RAW_INT8 || format==Format::TIFF_RAW_FLOAT32 )
        return encData;
    else if ( format==Format::TIFF_JPG_INT8 )
        return new DataSourceDecoder<JpegDecoder> ( encData );
    else if ( format==Format::TIFF_PNG_INT8 )
        return new DataSourceDecoder<PngDecoder> ( encData );
    else if ( format==Format::TIFF_LZW_INT8 || format == Format::TIFF_LZW_FLOAT32 )
        return new DataSourceDecoder<LzwDecoder> ( encData );
    else if ( format==Format::TIFF_ZIP_INT8 || format == Format::TIFF_ZIP_FLOAT32 )
        return new DataSourceDecoder<DeflateDecoder> ( encData );
    else if ( format==Format::TIFF_PKB_INT8 || format == Format::TIFF_PKB_FLOAT32 )
        return new DataSourceDecoder<PackBitsDecoder> ( encData );
    LOGGER_ERROR ( _ ( "Type d'encodage inconnu : " ) <<format );
    return 0;
}

DataSource* Level::getEncodedNoDataTile() {
    LOGGER_DEBUG ( _ ( "Tile : " ) << noDataFile );
    return noDataSourceProxy;
}

DataSource* Level::getTile ( int x, int y , DataSource* errorDataSource ) {
    DataSource* source=getEncodedTile ( x, y );
    DataSource* ndSource = ( errorDataSource?errorDataSource:noDataSourceProxy );
    size_t size;

    if ( ( format==Format::TIFF_RAW_INT8 || format == Format::TIFF_LZW_INT8 || format==Format::TIFF_LZW_FLOAT32 || format==Format::TIFF_ZIP_INT8 || format == Format::TIFF_ZIP_FLOAT32 || format==Format::TIFF_PKB_FLOAT32 || format==Format::TIFF_PKB_INT8 ) && source!=0 && source->getData ( size ) !=0 ) {
        LOGGER_DEBUG ( _ ( "GetTile Tiff" ) );
        TiffHeaderDataSource* fullTiffDS = new TiffHeaderDataSource ( source,format,channels,tm.getTileW(), tm.getTileH() );
        return new DataSourceProxy ( fullTiffDS,*ndSource );
    }

    return new DataSourceProxy ( source, *ndSource );
}

Image* Level::getTile ( int x, int y, int left, int top, int right, int bottom ) {
    int pixel_size=1;
    LOGGER_DEBUG ( _ ( "GetTile Image" ) );
    if ( format==Format::TIFF_RAW_FLOAT32 || format == Format::TIFF_LZW_FLOAT32 || format == Format::TIFF_ZIP_FLOAT32 || format == Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;
    return new ImageDecoder ( getDecodedTile ( x,y ), tm.getTileW(), tm.getTileH(), channels,
                              BoundingBox<double> ( tm.getX0() + x * tm.getTileW() * tm.getRes(),
                                      tm.getY0() - ( y+1 ) * tm.getTileH() * tm.getRes(),
                                      tm.getX0() + ( x+1 ) * tm.getTileW() * tm.getRes(),
                                      tm.getY0() - y * tm.getTileH() * tm.getRes() ),
                              left, top, right, bottom, pixel_size );
}

Image* Level::getNoDataTile ( BoundingBox<double> bbox ) {
    int pixel_size=1;
    LOGGER_DEBUG ( _ ( "GetTile Image" ) );
    if ( format==Format::TIFF_RAW_FLOAT32 || format == Format::TIFF_LZW_FLOAT32 || format == Format::TIFF_ZIP_FLOAT32 || format == Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;
    return new ImageDecoder ( getDecodedNoDataTile() , tm.getTileW(), tm.getTileH(), channels,
                              bbox, 0, 0, 0, 0, pixel_size );
}

int* Level::getNoDataValue ( int* nodatavalue ) {
    DataSource *nd =  getDecodedNoDataTile();

    size_t size;
    const uint8_t * buffer = nd->getData(size);
    if ( buffer ) { 
        if ( format == Format::TIFF_RAW_FLOAT32 || format == Format::TIFF_LZW_FLOAT32 || format == Format::TIFF_ZIP_FLOAT32 || format == Format::TIFF_PKB_FLOAT32) {
            const float* fbuf =  (const float*) buffer;
            for (int pixel = 0; pixel < this->channels; pixel++) {
                *(nodatavalue + pixel)  = (int) *(fbuf + pixel);
            }
        } else {
            for (int pixel = 0; pixel < this->channels; pixel++) {
                *(nodatavalue + pixel)  = *(buffer + pixel);
            }
        }
    } else {
            for (int pixel = 0; pixel < this->channels; pixel++) {
                *(nodatavalue + pixel)  = 0;
            }
    }
    delete nd;
    return nodatavalue;
}

SampleType Level::getSampleType() {
    return Format::toSampleType ( format );
}


