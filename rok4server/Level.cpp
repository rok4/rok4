/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
#include <cmath>
#include "Logger.h"
#include "Kernel.h"
#include <vector>
#include "Pyramid.h"
#include "Context.h"
#include "FileContext.h"
#include "PaletteDataSource.h"
#include "Format.h"
#include "intl.h"
#include "config.h"
#include <cstddef>
#include <sys/stat.h>

// GREG
#include "Message.h"
#include "Rok4Image.h"
// GREG


#define EPS 1./256. // FIXME: La valeur 256 est liée au nombre de niveau de valeur d'un canal
//        Il faudra la changer lorsqu'on aura des images non 8bits.


Level::Level ( LevelXML* l, PyramidXML* p ) {
    tm = l->tm;
    format = p->getFormat();

    baseDir = l->baseDir;
    pathDepth = l->pathDepth;
    context = l->context;
    prefix = l->prefix;

    tilesPerWidth = l->tilesPerWidth;
    tilesPerHeight = l->tilesPerHeight;

    maxTileRow = l->maxTileRow;
    minTileRow = l->minTileRow;
    maxTileCol = l->maxTileCol;
    minTileCol = l->minTileCol;


    if (Rok4Format::isRaster(format)) {
        onDemand = l->onDemand;
        onFly = l->onFly;
        sSources = l->sSources;
        channels = p->getChannels();
        maxTileSize = tm->getTileH() * tm->getTileW() * channels * Rok4Format::getChannelSize(format) * 2;
        nodataValue = p->getNoDataValues();
    } else {
        onDemand = false;
        onFly = false;
        channels = 0;
        maxTileSize = 0;
        tables = l->tables;
    }
    

}

Level::Level ( Level* obj, ServerXML* sxml, TileMatrixSet* tms) {
    // On met bien l'adresse du nouveau TileMatrix, et pas celui dans le Level cloné (issu de l'ancienne liste de TMS)
    tm = tms->getTm(obj->tm->getId());

    if (tm == NULL) {
        LOGGER_ERROR ( "Un niveau de pyramide cloné reference un niveau de TMS [" << obj->tm->getId() << "] qui n'existe plus." );
        return;
    }

    channels = obj->channels;
    baseDir = obj->baseDir;

    // On clone bien toutes les sources
    for ( int i = 0; i < obj->sSources.size(); i++ ) {
        if (obj->sSources.at(i)->getType() == PYRAMID) {
            Pyramid* pOrig = reinterpret_cast<Pyramid*>(obj->sSources.at(i));
            Pyramid* pS = new Pyramid(pOrig, sxml);
            if (pS->getTms() == NULL) {
                LOGGER_ERROR("Impossible de cloner la pyramide source pour ce niveau");
                tm == NULL;
                // Tester la nullité du TM en sortie pour faire remonter l'erreur
                return;
            }

            // On récupère bien le pointeur vers le nouveau style (celui de la nouvelle liste)
            Style* pSt = sxml->getStyle(pOrig->getStyle()->getId());
            if ( pSt == NULL ) {
                LOGGER_ERROR ( "Une pyramide source clonée reference un style [" << pOrig->getStyle()->getId() <<  "] qui n'existe plus." );
                pSt = sxml->getStyle("normal");
            }
            pS->setStyle(pSt);

            sSources.push_back(pS);
        } else if (obj->sSources.at(i)->getType() == WEBSERVICE) {
            WebService* pS = new WebService(reinterpret_cast<WebService*>(obj->sSources.at(i)));
            sSources.push_back(pS);
        }
    }

    tilesPerWidth = obj->tilesPerWidth;
    tilesPerHeight = obj->tilesPerHeight;

    maxTileRow = obj->maxTileRow;
    minTileRow = obj->minTileRow;
    maxTileCol = obj->maxTileCol;
    minTileCol = obj->minTileCol;
    onDemand = obj->onDemand;
    onFly = obj->onFly;


    pathDepth = obj->pathDepth;
    format = obj->format;

    context = NULL;

    if (obj->context != NULL) {
        switch ( obj->context->getType() ) {
            case FILECONTEXT :
                context = new FileContext("");
                if (! context->connection() ) {
                    LOGGER_ERROR("Impossible de se connecter aux donnees.");
                    tm == NULL;
                    return;
                }
                break;
#if BUILD_OBJECT
            case CEPHCONTEXT :
                if (sxml->getCephContextBook() != NULL) {
                    context = sxml->getCephContextBook()->addContext(obj->context->getTray());
                } else {
                    LOGGER_ERROR ( "L'utilisation d'un cephContext necessite de preciser les informations de connexions dans le server.conf");
                    tm == NULL;
                    return;
                }
                break;
            case S3CONTEXT :
                if (sxml->getS3ContextBook() != NULL) {
                    context = sxml->getS3ContextBook()->addContext(obj->context->getTray());
                } else {
                    LOGGER_ERROR ( "L'utilisation d'un s3Context necessite de preciser les informations de connexions dans le server.conf");
                    tm == NULL;
                    return;
                }
                break;
            case SWIFTCONTEXT :
                if (sxml->getSwiftContextBook() != NULL) {
                    context = sxml->getSwiftContextBook()->addContext(obj->context->getTray());
                } else {
                    LOGGER_ERROR ( "L'utilisation d'un swiftContext necessite de preciser les informations de connexions dans le server.conf");
                    tm == NULL;
                    return;
                }
                break;
#endif
        }
    }

    prefix = obj->prefix;

    if (Rok4Format::isRaster(format)) {
        maxTileSize = obj->maxTileSize;
        nodataValue = new int[channels];
        for (int i = 0; i < channels; ++i) {
            nodataValue[i] = obj->nodataValue[i];
        }
    } else {
        tables = obj->tables;
    }

}


Level::~Level() {

    // les contextes sont dans des contextBooks
    // ce sont les contextBooks qui se chargent de détruire les contextes
    // mais ce n'est pas le cas des FILECONTEXT
    if (context) {
        if (context->getType() == FILECONTEXT) delete context;
    }

    for ( int i = 0; i < sSources.size(); i++ ) {
        Source* pS = sSources.at(i);
        delete pS;
    }

    if (Rok4Format::isRaster(format)) delete[] nodataValue;
}


/*
 * A REFAIRE
 */
Image* Level::getbbox ( ServicesXML* servicesConf, BoundingBox< double > bbox, int width, int height, CRS src_crs, CRS dst_crs, Interpolation::KernelType interpolation, int& error ) {

    Grid* grid = new Grid ( width, height, bbox );

    grid->bbox.print();

    if ( ! ( grid->reproject ( dst_crs.getProj4Code(), src_crs.getProj4Code() ) ) ) {
        LOGGER_DEBUG("Impossible de reprojeter la grid");
        error = 1; // BBox invalid
        delete grid;
        return 0;
    }

    //la reprojection peut marcher alors que la bbox contient des NaN
    //cela arrive notamment lors que la bbox envoyée par l'utilisateur n'est pas dans le crs specifié par ce dernier
    if (grid->bbox.xmin != grid->bbox.xmin || grid->bbox.xmax != grid->bbox.xmax || grid->bbox.ymin != grid->bbox.ymin || grid->bbox.ymax != grid->bbox.ymax ) {
        LOGGER_DEBUG("Bbox de la grid contenant des NaN");
        error = 1;
        delete grid;
        return 0;
    }

    grid->bbox.print();

    // Calcul de la taille du noyau
    //Maintain previous Lanczos behaviour : Lanczos_2 for resampling and reprojecting
    if ( interpolation >= Interpolation::LANCZOS_2 ) interpolation= Interpolation::LANCZOS_2;

    const Kernel& kk = Kernel::getInstance ( interpolation ); // Lanczos_2
    double ratio_x = ( grid->bbox.xmax - grid->bbox.xmin ) / ( tm->getRes() *double ( width ) );
    double ratio_y = ( grid->bbox.ymax - grid->bbox.ymin ) / ( tm->getRes() *double ( height ) );
    double bufx=kk.size ( ratio_x );
    double bufy=kk.size ( ratio_y );

    bufx<50?bufx=50:0;
    bufy<50?bufy=50:0; // Pour etre sur de ne pas regresser
    BoundingBox<int64_t> bbox_int ( floor ( ( grid->bbox.xmin - tm->getX0() ) /tm->getRes() - bufx ),
                                    floor ( ( tm->getY0() - grid->bbox.ymax ) /tm->getRes() - bufy ),
                                    ceil ( ( grid->bbox.xmax - tm->getX0() ) /tm->getRes() + bufx ),
                                    ceil ( ( tm->getY0() - grid->bbox.ymin ) /tm->getRes() + bufy ) );

    Image* image = getwindow ( servicesConf, bbox_int, error );
    if ( !image ) {
        LOGGER_DEBUG ( _ ( "Image invalid !" ) );
        return 0;
    }

    image->setBbox ( BoundingBox<double> ( tm->getX0() + tm->getRes() * bbox_int.xmin, tm->getY0() - tm->getRes() * bbox_int.ymax, tm->getX0() + tm->getRes() * bbox_int.xmax, tm->getY0() - tm->getRes() * bbox_int.ymin ) );

    grid->affine_transform ( 1./image->getResX(), -image->getBbox().xmin/image->getResX() - 0.5,
                             -1./image->getResY(), image->getBbox().ymax/image->getResY() - 0.5 );

    return new ReprojectedImage ( image, bbox, grid, interpolation );
}


Image* Level::getbbox ( ServicesXML* servicesConf, BoundingBox< double > bbox, int width, int height, Interpolation::KernelType interpolation, int& error ) {

    // On convertit les coordonnées en nombre de pixels depuis l'origine X0,Y0
    bbox.xmin = ( bbox.xmin - tm->getX0() ) /tm->getRes();
    bbox.xmax = ( bbox.xmax - tm->getX0() ) /tm->getRes();
    double tmp = bbox.ymin;
    bbox.ymin = ( tm->getY0() - bbox.ymax ) /tm->getRes();
    bbox.ymax = ( tm->getY0() - tmp ) /tm->getRes();

    //A VERIFIER !!!!
    BoundingBox<int64_t> bbox_int ( floor ( bbox.xmin + EPS ),
                                    floor ( bbox.ymin + EPS ),
                                    ceil ( bbox.xmax - EPS ),
                                    ceil ( bbox.ymax - EPS ) );

    if ( bbox_int.xmax - bbox_int.xmin == width && bbox_int.ymax - bbox_int.ymin == height &&
            bbox.xmin - bbox_int.xmin < EPS && bbox_int.xmax - bbox.xmax < EPS &&
            bbox.ymin - bbox_int.ymin < EPS && bbox_int.ymax - bbox.ymax < EPS ) {
        /* L'image demandée est en phase et a les mêmes résolutions que les images du niveau
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

Image* Level::getwindow ( ServicesXML* servicesConf, BoundingBox< int64_t > bbox, int& error ) { 
    int tile_xmin=euclideanDivisionQuotient ( bbox.xmin,tm->getTileW() );
    int tile_xmax=euclideanDivisionQuotient ( bbox.xmax -1,tm->getTileW() );
    int nbx = tile_xmax - tile_xmin + 1;
    if ( nbx >= servicesConf->getMaxTileX() ) {
        LOGGER_INFO ( _ ( "Too Much Tile on X axis" ) );
        error=2;
        return 0;
    }
    if (nbx == 0) {
        LOGGER_INFO("nbx = 0");
        error=1;
        return 0;
    }

    int tile_ymin=euclideanDivisionQuotient ( bbox.ymin,tm->getTileH() );
    int tile_ymax = euclideanDivisionQuotient ( bbox.ymax-1,tm->getTileH() );
    int nby = tile_ymax - tile_ymin + 1;
    if ( nby >= servicesConf->getMaxTileY() ) {
        LOGGER_INFO ( _ ( "Too Much Tile on Y axis" ) );
        error=2;
        return 0;
    }
    if (nby == 0) {
        LOGGER_INFO("nby = 0");
        error=1;
        return 0;
    }

    int left[nbx];
    memset ( left,   0, nbx*sizeof ( int ) );
    left[0]=euclideanDivisionRemainder ( bbox.xmin,tm->getTileW() );
    int top[nby];
    memset ( top,    0, nby*sizeof ( int ) );
    top[0]=euclideanDivisionRemainder ( bbox.ymin,tm->getTileH() );
    int right[nbx];
    memset ( right,  0, nbx*sizeof ( int ) );
    right[nbx - 1] = tm->getTileW() - euclideanDivisionRemainder ( bbox.xmax -1,tm->getTileW() ) -1;
    int bottom[nby];
    memset ( bottom, 0, nby*sizeof ( int ) );
    bottom[nby- 1] = tm->getTileH() - euclideanDivisionRemainder ( bbox.ymax -1,tm->getTileH() ) - 1;

    std::vector<std::vector<Image*> > T ( nby, std::vector<Image*> ( nbx ) );
    for ( int y = 0; y < nby; y++ ) {
        for ( int x = 0; x < nbx; x++ ) {
            T[y][x] = getTile ( tile_xmin + x, tile_ymin + y, left[x], top[y], right[x], bottom[y] );
        }
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
        case S3CONTEXT:
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
int Level::createDirPath (std::string path) {

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
DataSource* Level::getEncodedTile ( int x, int y ) { // TODO: return 0 sur des cas d'erreur..

    //on stocke une dalle
    // Index de la tuile (cf. ordre de rangement des tuiles)
    int n= ( y%tilesPerHeight ) *tilesPerWidth + ( x%tilesPerWidth );
    // Les index sont stockés à partir de l'octet ROK4_IMAGE_HEADER_SIZE
    uint32_t posoff=ROK4_IMAGE_HEADER_SIZE+4*n, possize=ROK4_IMAGE_HEADER_SIZE+tilesPerWidth*tilesPerHeight*4+4*n;
    std::string path=getPath ( x, y, tilesPerWidth, tilesPerHeight);
    LOGGER_DEBUG ( path );
    return new StoreDataSource ( path, posoff, possize, ROK4_IMAGE_HEADER_SIZE + 2*4*tilesPerWidth*tilesPerHeight, Rok4Format::toMimeType ( format ), context, Rok4Format::toEncoding( format ) );
}

DataSource* Level::getDecodedTile ( int x, int y ) {

    DataSource* encData = getEncodedTile ( x, y );
    if (encData == NULL) return 0;

    size_t size;
    if (encData->getData ( size ) == NULL) {
        delete encData;
        return 0;
    }

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


DataSource* Level::getTile (int x, int y) {

    DataSource* source = getEncodedTile ( x, y );
    if (source == NULL) return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND, _ ( "No data found" ), "wmts" ) );

    size_t size;
    if (source->getData ( size ) == NULL) return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND, _ ( "No data found" ), "wmts" ) );

    if ( format == Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_LZW_INT8 ||
         format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_INT8 ||
         format == Rok4Format::TIFF_PKB_FLOAT32 || format == Rok4Format::TIFF_PKB_INT8
        )
    {
        LOGGER_DEBUG ( _ ( "GetTile Tiff" ) );
        TiffHeaderDataSource* fullTiffDS = new TiffHeaderDataSource ( source,format,channels,tm->getTileW(), tm->getTileH() );
        return fullTiffDS;
    }

    return source;
}

Image* Level::getTile ( int x, int y, int left, int top, int right, int bottom ) {
    int pixel_size=1;
    LOGGER_DEBUG ( _ ( "GetTile Image" ) );
    if ( format==Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32 || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32 )
        pixel_size=4;

    DataSource* ds = getDecodedTile ( x,y );

    BoundingBox<double> bb ( 
        tm->getX0() + x * tm->getTileW() * tm->getRes() + left * tm->getRes(),
        tm->getY0() - ( y+1 ) * tm->getTileH() * tm->getRes() + bottom * tm->getRes(),
        tm->getX0() + ( x+1 ) * tm->getTileW() * tm->getRes() - right * tm->getRes(),
        tm->getY0() - y * tm->getTileH() * tm->getRes() - top * tm->getRes()
    );

    if (ds == 0) {
        // On crée une image monochrome (valeur fournie dans la pyramide) de la taille qu'aurait du avoir la tuile demandée
        EmptyImage* ei = new EmptyImage(
            tm->getTileW() - left - right, // width
            tm->getTileH() - top - bottom, // height
            channels,
            nodataValue
        );
        ei->setBbox(bb);
        return ei;
    } else {
        return new ImageDecoder (
            ds, tm->getTileW(), tm->getTileH(), channels, bb,
            left, top, right, bottom, pixel_size
        );
    }
}


BoundingBox<double> Level::tileIndicesToSlabBbox (int tileCol, int tileRow) {

    //Variables utilisees
    double Col, Row, xmin, ymin, xmax, ymax, xo, yo, resolution;
    int tileH, tileW,TilePerWidth, TilePerHeight;

    //Récupération des paramètres associés
    resolution = tm->getRes();
    xo = tm->getX0();
    yo = tm->getY0();
    tileH = tm->getTileH();
    tileW = tm->getTileW();
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

BoundingBox<double> Level::tileIndicesToTileBbox (int tileCol, int tileRow) {

    //Variables utilisees
    double Col, Row, xmin, ymin, xmax, ymax, xo, yo, resolution;
    int tileH, tileW;

    //Récupération des paramètres associés
    resolution = tm->getRes();
    xo = tm->getX0();
    yo = tm->getY0();
    tileH = tm->getTileH();
    tileW = tm->getTileW();

    Row = double(tileRow);
    Col = double(tileCol);
    xmin = Col * double(tileW) * resolution + xo;
    ymax = yo - Row * double(tileH) * resolution;
    xmax = xmin + double(tileW) * resolution;
    ymin = ymax - double(tileH) * resolution;

    BoundingBox<double> bbox(xmin,ymin,xmax,ymax) ;
    return bbox;

}

int Level::getSlabWidth () {

    int TilePerWidth = getTilesPerWidth();
    int width = tm->getTileW();

    return width*TilePerWidth;

}

int Level::getSlabHeight () {

    int TilePerHeight = getTilesPerHeight();
    int height = tm->getTileH();

    return height*TilePerHeight;

}

BoundingBox<double> Level::TMLimitsToBbox () {

    int bPMinCol,bPMaxCol,bPMinRow,bPMaxRow;
    double xo,yo,res,tileW,tileH,xmin,xmax,ymin,ymax;

    bPMinCol = getMinTileCol();
    bPMaxCol = getMaxTileCol();
    bPMinRow = getMinTileRow();
    bPMaxRow = getMaxTileRow();

    //On récupère d'autres informations sur le TM
    xo = tm->getX0();
    yo = tm->getY0();
    res = tm->getRes();
    tileW = tm->getTileW();
    tileH = tm->getTileH();

    //On transforme en bbox
    xmin = bPMinCol * tileW * res + xo;
    ymax = yo - bPMinRow * tileH * res;
    xmax = xo + (bPMaxCol+1) * tileW * res;
    ymin = ymax - (bPMaxRow - bPMinRow + 1) * tileH * res;

    return BoundingBox<double> (xmin,ymin,xmax,ymax);


}

TileMatrix* Level::getTm () { return tm; }
Rok4Format::eformat_data Level::getFormat () { return format; }
int Level::getChannels () { return channels; }
uint32_t Level::getMaxTileRow () { return maxTileRow; }
uint32_t Level::getMinTileRow () { return minTileRow; }
uint32_t Level::getMaxTileCol () { return maxTileCol; }
uint32_t Level::getMinTileCol () { return minTileCol; }
void Level::setMaxTileRow (uint32_t mm ) { maxTileRow = mm; }
void Level::setMinTileRow (uint32_t mm ) { minTileRow = mm; }
void Level::setMaxTileCol (uint32_t mm ) { maxTileCol = mm; }
void Level::setMinTileCol (uint32_t mm ) { minTileCol = mm; }
double Level::getRes () { return tm->getRes(); }
std::string Level::getId () { return tm->getId(); }
uint32_t Level::getTilesPerWidth () { return tilesPerWidth; }
uint32_t Level::getTilesPerHeight () { return tilesPerHeight; }
Context* Level::getContext () { return context; }
bool Level::isOnDemand() { return onDemand; }
bool Level::isOnFly() { return onFly; }
std::vector<Table>* Level::getTables() { return &tables; }
std::vector<Source*> Level::getSources() { return sSources; }
