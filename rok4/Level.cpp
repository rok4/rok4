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
#include "StoreDataSource.h"
#include "CompoundImage.h"
#include "ResampledImage.h"
#include "ReprojectedImage.h"
#include "RawImage.h"
#include "Decoder.h"
#include "TiffEncoder.h"
#include "TiffHeaderDataSource.h"
#include "EmptyDataSource.h"
#include <cmath>
#include "Logger.h"
#include "Kernel.h"
#include <vector>
#include "Pyramid.h"
#include "Context.h"
#include "PaletteDataSource.h"
#include "Format.h"
#include "intl.h"
#include "config.h"
#include <cstddef>
#include <sys/stat.h>

// GREG
#include "Message.h"
// GREG


#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.


Level::Level ( LevelXML* l ) {
    tm = l->tm;
    channels = l->channels;
    baseDir = l->baseDir;
    sSources = l->sSources;

    tilesPerWidth = l->tilesPerWidth;
    tilesPerHeight = l->tilesPerHeight;

    maxTileRow = l->maxTileRow;
    minTileRow = l->minTileRow;
    maxTileCol = l->maxTileCol;
    minTileCol = l->minTileCol;

    pathDepth = l->pathDepth;
    format = l->format;
    noDataFile = l->noDataFilePath;
    noDataSource = NULL;
    context = l->context;
    prefix = l->prefix;

    StoreDataSourceFactory SDSF;
    noDataTileSource = SDSF.createStoreDataSource (
        noDataFile.c_str(), 2048, 2048+4, Rok4Format::toMimeType ( format ), context;
        Rok4Format::toEncoding ( format )
    );

    noDataSourceProxy = noDataTileSource;
}

Level::~Level() {

    delete noDataSourceProxy;
    noDataSourceProxy = NULL;
    if ( noDataSource ) {
        delete noDataSource;
        noDataSource = NULL;
    }
    // les contextes sont dans des contextBooks
    // ce sont les contextBooks qui se chargent de détruire les contexts
    // mais ce n'est pas le cas des FILECONTEXT
    if (context) {
        if (context->getType() == FILECONTEXT) {
            delete context;
        }
    }

}

void Level::setNoData ( const std::string& file ) {
    noDataFile=file;
    StoreDataSourceFactory SDSF;
    DataSource* tmpDataSource = SDSF.createStoreDataSource ( noDataFile.c_str(),2048,2048+4, Rok4Format::toMimeType ( format ), context, Rok4Format::toEncoding ( format ) );
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
        StoreDataSourceFactory SDSF;
        noDataTileSource = SDSF.createStoreDataSource ( noDataFile.c_str(),2048,2048+4, Rok4Format::toMimeType ( format ), context, Rok4Format::toEncoding ( format ) );
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
    return new ResampledImage ( imageout, width, height, res_x, res_y, bbox, interpolation, false );
}


/*
 * A REFAIRE
 */
Image* Level::getbbox ( ServicesConf& servicesConf, BoundingBox< double > bbox, int width, int height, CRS src_crs, CRS dst_crs, Interpolation::KernelType interpolation, int& error ) {
    Grid* grid = new Grid ( width, height, bbox );

    grid->bbox.print();

    if ( ! ( grid->reproject ( dst_crs.getProj4Code(), src_crs.getProj4Code() ) ) ) {
        error = 1; // BBox invalid
        delete grid;
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
    if (! imageout->setDimensions ( bbox_int.xmax - bbox_int.xmin, bbox_int.ymax - bbox_int.ymin, BoundingBox<double> ( bbox_int ), 1.0, 1.0 ) ) {
        LOGGER_DEBUG ( _ ( "Dimensions invalid !" ) );
        return 0;
    }

    return new ResampledImage ( imageout, width, height, ratio_x, ratio_y, bbox, interpolation, false );
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
 * Recuperation du nom de la dalle du cache en fonction de son indice
 */
std::string Level::getPath ( int tilex, int tiley, int tilesPerW, int tilesPerH ) {
    // Cas normalement filtré en amont (exception WMS/WMTS)
    if ( tilex < 0 || tiley < 0 ) {
        LOGGER_ERROR ( _ ( "Indice de tuile negatif" ) );
        return "";
    }

    std::ostringstream convert;
    int x,y,pos;

    x = tilex / tilesPerW;
    y = tiley / tilesPerH;


    switch (context->getType()) {
        case FILECONTEXT:

            char path[32];
            path[sizeof ( path ) - 5] = '.';
            path[sizeof ( path ) - 4] = 't';
            path[sizeof ( path ) - 3] = 'i';
            path[sizeof ( path ) - 2] = 'f';
            path[sizeof ( path ) - 1] = 0;
            pos = sizeof ( path ) - 6;

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
            break;
        case CEPHCONTEXT:
            convert << "_" << x << "_" << y;
            return prefix + convert.str();
            break;
        case SWIFTCONTEXT:
            convert << "_" << x << "_" << y;
            return prefix + convert.str();
            break;
        default:
            return "";

    }
}

std::string Level::getDirPath ( int tilex, int tiley ) {

    if (context->getType() == FILECONTEXT) {
        std::string file = getPath(tilex,tiley, tilesPerWidth, tilesPerHeight);
        return file.substr(0,file.find_last_of("/"));        
    } else {
        LOGGER_ERROR ( _ ( "getDirPath n'a pas de sens dans le cas d'un contexte non fichier" ) );
        return "";
    }

    return "";
}

/*
 * Creation du dossier indiqué par path
 */
int Level::createDirPath(std::string path) {

    int success = -1;
    int curDirCreated;
    std::size_t found = path.find_first_of("/");
    std::string currentDir = path.substr(0,found)+"/";
    std::string endOfPath = path.substr(found+1);

    while (found != std::string::npos) {
        found = endOfPath.find_first_of("/");
        currentDir += endOfPath.substr(0,found)+"/";
        endOfPath = endOfPath.substr(found+1);
        curDirCreated = mkdir(currentDir.c_str(),ACCESSPERMS);
        if (curDirCreated) {
            success = 0;
        }
    }

    return success;

}


/*
 * @return la tuile d'indice (x,y) du niveau
 */

DataSource* Level::getEncodedTile ( int x, int y ) {
    // TODO: return 0 sur des cas d'erreur..
    if (tilesPerWidth == 0 && tilesPerHeight == 0) {

        //on stocke une tuile et non une dalle
        std::string path=getPath ( x, y, 1, 1 );
        LOGGER_DEBUG ( path );
        StoreDataSourceFactory SDSF;
        return SDSF.createStoreDataSource( path.c_str(),maxTileSize,Rok4Format::toMimeType ( format ), context, Rok4Format::toEncoding( format ) );

    } else {

        //on stocke une dalle
        // Index de la tuile (cf. ordre de rangement des tuiles)
        int n= ( y%tilesPerHeight ) *tilesPerWidth + ( x%tilesPerWidth );
        // Les index sont stockés à partir de l'octet 2048
        uint32_t posoff=2048+4*n, possize=2048+4*n +tilesPerWidth*tilesPerHeight*4;
        std::string path=getPath ( x, y, tilesPerWidth, tilesPerHeight);
        LOGGER_DEBUG ( path );
        StoreDataSourceFactory SDSF;
        return SDSF.createStoreDataSource( path.c_str(),posoff,possize,Rok4Format::toMimeType ( format ), context, Rok4Format::toEncoding( format ) );

    }

}

DataSource* Level::getDecodedTile ( int x, int y ) {
    DataSource* encData = new DataSourceProxy ( getEncodedTile ( x, y ),*getEncodedNoDataTile() );
    if ( format==Rok4Format::TIFF_RAW_INT8 || format==Rok4Format::TIFF_RAW_FLOAT32 )
        return encData;
    else if ( format==Rok4Format::TIFF_JPG_INT8 )
        return new DataSourceDecoder<JpegDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_PNG_INT8 )
        return new DataSourceDecoder<PngDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_LZW_INT8 || format == Rok4Format::TIFF_LZW_FLOAT32 )
        return new DataSourceDecoder<LzwDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_ZIP_FLOAT32 )
        return new DataSourceDecoder<DeflateDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_PKB_INT8 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        return new DataSourceDecoder<PackBitsDecoder> ( encData );
    LOGGER_ERROR ( _ ( "Type d'encodage inconnu : " ) <<format );
    return 0;
}

DataSource* Level::getDecodedNoDataTile() {
    StoreDataSourceFactory SDSF;
    DataSource* encData = new DataSourceProxy ( SDSF.createStoreDataSource ( "",0,0,"",context ),*getEncodedNoDataTile() );
    if ( format==Rok4Format::TIFF_RAW_INT8 || format==Rok4Format::TIFF_RAW_FLOAT32 )
        return encData;
    else if ( format==Rok4Format::TIFF_JPG_INT8 )
        return new DataSourceDecoder<JpegDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_PNG_INT8 )
        return new DataSourceDecoder<PngDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_LZW_INT8 || format == Rok4Format::TIFF_LZW_FLOAT32 )
        return new DataSourceDecoder<LzwDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_ZIP_FLOAT32 )
        return new DataSourceDecoder<DeflateDecoder> ( encData );
    else if ( format==Rok4Format::TIFF_PKB_INT8 || format == Rok4Format::TIFF_PKB_FLOAT32 )
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

    if ( ( format==Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_LZW_INT8 || format==Rok4Format::TIFF_LZW_FLOAT32 || format==Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format==Rok4Format::TIFF_PKB_FLOAT32 || format==Rok4Format::TIFF_PKB_INT8 ) && source!=0 && source->getData ( size ) !=0 ) {
        LOGGER_DEBUG ( _ ( "GetTile Tiff" ) );
        TiffHeaderDataSource* fullTiffDS = new TiffHeaderDataSource ( source,format,channels,tm.getTileW(), tm.getTileH() );
        return new DataSourceProxy ( fullTiffDS,*ndSource );
    }

    return new DataSourceProxy ( source, *ndSource );
}

Image* Level::getTile ( int x, int y, int left, int top, int right, int bottom ) {
    int pixel_size=1;
    LOGGER_DEBUG ( _ ( "GetTile Image" ) );
    if ( format==Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;
    return new ImageDecoder ( getDecodedTile ( x,y ), tm.getTileW(), tm.getTileH(), channels,
                              BoundingBox<double> ( tm.getX0() + x * tm.getTileW() * tm.getRes() + left * tm.getRes(),
                                      tm.getY0() - ( y+1 ) * tm.getTileH() * tm.getRes() + bottom * tm.getRes(),
                                      tm.getX0() + ( x+1 ) * tm.getTileW() * tm.getRes() - right * tm.getRes(),
                                      tm.getY0() - y * tm.getTileH() * tm.getRes() - top * tm.getRes() ),
                              left, top, right, bottom, pixel_size );
}

Image* Level::getNoDataTile ( BoundingBox<double> bbox ) {
    int pixel_size=1;
    LOGGER_DEBUG ( _ ( "GetTile Image" ) );
    if ( format==Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;
    return new ImageDecoder ( getDecodedNoDataTile() , tm.getTileW(), tm.getTileH(), channels,
                              bbox, 0, 0, 0, 0, pixel_size );
}

int* Level::getNoDataValue ( int* nodatavalue ) {
    DataSource *nd =  getDecodedNoDataTile();

    size_t size;
    const uint8_t * buffer = nd->getData ( size );
    if ( buffer ) {
        if ( format == Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32 ) {
            const float* fbuf = ( const float* ) buffer;
            for ( int pixel = 0; pixel < this->channels; pixel++ ) {
                * ( nodatavalue + pixel )  = ( int ) * ( fbuf + pixel );
            }
        } else {
            for ( int pixel = 0; pixel < this->channels; pixel++ ) {
                * ( nodatavalue + pixel )  = * ( buffer + pixel );
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

BoundingBox<double> Level::tileIndicesToSlabBbox(int tileCol, int tileRow) {

    //Variables utilisees
    double Col, Row, xmin, ymin, xmax, ymax, xo, yo, resolution;
    int tileH, tileW,TilePerWidth, TilePerHeight;

    //Récupération du TileMatrix demandé
    TileMatrix tm = getTm();

    //Récupération des paramètres associés
    resolution = tm.getRes();
    xo = tm.getX0();
    yo = tm.getY0();
    tileH = tm.getTileH();
    tileW = tm.getTileW();
    TilePerWidth = getTilesPerWidth();
    TilePerHeight = getTilesPerHeight();

    Row = floor(double(tileRow) / double(TilePerHeight) ) * double(TilePerHeight);
    Col = floor(double(tileCol) / double(TilePerWidth) ) * double(TilePerWidth);

    //calcul de la bbox de la dalle et non de la tuile
    xmin = Col * double(tileW) * resolution  + xo;
    ymax = yo - Row * double(tileH) * resolution;
    xmax = xmin + double(tileW) * resolution * TilePerWidth;
    ymin = ymax - double(tileH) * resolution * TilePerHeight;

    BoundingBox<double> bbox(xmin,ymin,xmax,ymax) ;
    return bbox;

}

BoundingBox<double> Level::tileIndicesToTileBbox(int tileCol, int tileRow) {

    //Variables utilisees
    double Col, Row, xmin, ymin, xmax, ymax, xo, yo, resolution;
    int tileH, tileW;

    //calcul de la bbox
    TileMatrix tm = getTm();

    //Récupération des paramètres associés
    resolution = tm.getRes();
    xo = tm.getX0();
    yo = tm.getY0();
    tileH = tm.getTileH();
    tileW = tm.getTileW();

    Row = double(tileRow);
    Col = double(tileCol);
    xmin = Col * double(tileW) * resolution + xo;
    ymax = yo - Row * double(tileH) * resolution;
    xmax = xmin + double(tileW) * resolution;
    ymin = ymax - double(tileH) * resolution;

    BoundingBox<double> bbox(xmin,ymin,xmax,ymax) ;
    return bbox;

}

int Level::getSlabWidth() {

    int TilePerWidth = getTilesPerWidth();
    TileMatrix tm = getTm();
    int width = tm.getTileW();

    return width*TilePerWidth;

}

int Level::getSlabHeight() {

    int TilePerHeight = getTilesPerHeight();
    TileMatrix tm = getTm();
    int height = tm.getTileH();

    return height*TilePerHeight;

}

BoundingBox<double> Level::TMLimitsToBbox() {

    int bPMinCol,bPMaxCol,bPMinRow,bPMaxRow;
    double xo,yo,res,tileW,tileH,xmin,xmax,ymin,ymax;

    bPMinCol = getMinTileCol();
    bPMaxCol = getMaxTileCol();
    bPMinRow = getMinTileRow();
    bPMaxRow = getMaxTileRow();

    //On récupère d'autres informations sur le TM
    xo = getTm().getX0();
    yo = getTm().getY0();
    res = getTm().getRes();
    tileW = getTm().getTileW();
    tileH = getTm().getTileH();

    //On transforme en bbox
    xmin = bPMinCol * tileW * res + xo;
    ymax = yo - bPMinRow * tileH * res;
    xmax = xo + (bPMaxCol+1) * tileW * res;
    ymin = ymax - (bPMaxRow - bPMinRow + 1) * tileH * res;

    return BoundingBox<double> (xmin,ymin,xmax,ymax);


}

void Level::updateNoDataTile(std::vector<int> noDataValues) {

    if (noDataTileSource) {
        delete noDataTileSource;
        noDataTileSource = NULL;
    }

    noDataTileSource = new EmptyDataSource(channels,noDataValues,tm.getTileW(),tm.getTileH(),format);
    noDataSourceProxy = noDataTileSource;

}
