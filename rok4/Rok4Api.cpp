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
* \file Rok4Api.cpp
* \brief Implementation de l'API de ROK4
*/

#include "Rok4Api.h"
#include "config.h"
#include <proj_api.h>
#include "ConfLoader.h"
#include "Message.h"
#include "Request.h"
#include "RawImage.h"
#include "TiffEncoder.h"
#include "TiffHeaderDataSource.h"
#include "Palette.h"
#include <cstdlib>
#include <PNGEncoder.h>
#include <Decoder.h>

/**
* @brief Initialisation d'une reponse a partir d'une source
* @brief Les donnees source sont copiees dans la reponse
*/

static bool loggerInitialised = false;

HttpResponse* initResponseFromSource ( DataSource* source ) {
    HttpResponse* response=new HttpResponse;
    response->status=source->getHttpStatus();
    response->type=new char[source->getType().length() +1];
    strcpy ( response->type,source->getType().c_str() );
    size_t buffer_size;
    const uint8_t *buffer = source->getData ( buffer_size );
    // TODO : tester sans copie memoire (attention, la source devrait etre supprimee plus tard)
    response->content=new uint8_t[buffer_size];
    memcpy ( response->content, ( uint8_t* ) buffer,buffer_size );
    response->contentSize=buffer_size;
    return response;
}

/**
* @brief Initialisation du serveur ROK4
* @param serverConfigFile : nom du fichier de configuration des parametres techniques
* @return : pointeur sur le serveur ROK4, NULL en cas d'erreur (forcement fatale)
*/

Rok4Server* rok4InitServer ( const char* serverConfigFile ) {
    // Initialisation des parametres techniques
    LogOutput logOutput;
    int nbThread,logFilePeriod;
    LogLevel logLevel;
    bool reprojectionCapability;
    std::string strServerConfigFile=serverConfigFile,strLogFileprefix,strServicesConfigFile,strLayerDir,strTmsDir,strStyleDir;
    if ( !ConfLoader::getTechnicalParam ( strServerConfigFile, logOutput, strLogFileprefix, logFilePeriod, logLevel, nbThread, reprojectionCapability, strServicesConfigFile, strLayerDir, strTmsDir, strStyleDir ) ) {
        std::cerr<<"ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur "<<strServerConfigFile<<std::endl;
        return false;
    }
    if ( !loggerInitialised ) {
        Logger::setOutput ( logOutput );
        // Initialisation du logger
        Accumulator *acc=0;
        if ( logOutput==ROLLING_FILE ) {
            acc = new RollingFileAccumulator ( strLogFileprefix,logFilePeriod );
        } else if ( logOutput==STANDARD_OUTPUT_STREAM_FOR_ERRORS ) {
            acc = new StreamAccumulator();
        }
        // Attention : la fonction Logger::setAccumulator n'est pas threadsafe
        for ( int i=0;i<=logLevel;i++ )
            Logger::setAccumulator ( ( LogLevel ) i, acc );
        std::ostream &log = LOGGER ( DEBUG );
        log.precision ( 8 );
        log.setf ( std::ios::fixed,std::ios::floatfield );

        std::cout<<"Envoi des messages dans la sortie du logger"<< std::endl;
        LOGGER_INFO ( "*** DEBUT DU FONCTIONNEMENT DU LOGGER ***" );
        loggerInitialised=true;
    } else {
        LOGGER_INFO ( "*** NOUVEAU CLIENT DU LOGGER ***" );
    }

    // Construction des parametres de service
    ServicesConf* servicesConf=ConfLoader::buildServicesConf ( strServicesConfigFile );
    if ( servicesConf==NULL ) {
        LOGGER_FATAL ( "Impossible d'interpreter le fichier de conf "<<strServicesConfigFile );
        LOGGER_FATAL ( "Extinction du serveur ROK4" );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }
    // Chargement des TMS
    std::map<std::string,TileMatrixSet*> tmsList;
    if ( !ConfLoader::buildTMSList ( strTmsDir,tmsList ) ) {
        LOGGER_FATAL ( "Impossible de charger la conf des TileMatrix" );
        LOGGER_FATAL ( "Extinction du serveur ROK4" );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }
    //Chargement des styles
    std::map<std::string, Style*> styleList;
    if ( !ConfLoader::buildStylesList ( strStyleDir,styleList, servicesConf->isInspire() ) ) {
        LOGGER_FATAL ( "Impossible de charger la conf des Styles" );
        LOGGER_FATAL ( "Extinction du serveur ROK4" );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Chargement des layers
    std::map<std::string, Layer*> layerList;
    if ( !ConfLoader::buildLayersList ( strLayerDir,tmsList, styleList,layerList,reprojectionCapability,servicesConf->isInspire() ) ) {
        LOGGER_FATAL ( "Impossible de charger la conf des Layers/pyramides" );
        LOGGER_FATAL ( "Extinction du serveur ROK4" );
        sleep ( 1 );    // Pour laisser le temps au logger pour se vider
        return NULL;
    }

    // Instanciation du serveur
    return new Rok4Server ( nbThread, *servicesConf, layerList, tmsList );
}

/**
* @fn HttpRequest* rok4InitRequest(const char* queryString, const char* hostName, const char* scriptName)
* @brief Initialisation d'une requete
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @return Requete (memebres alloues ici, doivent etre desalloues ensuite)
* Requete HTTP, basee sur la terminologie des variables d'environnement Apache et completee par le type d'operation (au sens WMS/WMTS) de la requete
* Exemple :
* http://localhost/target/bin/rok4?SERVICE=WMTS&REQUEST=GetTile&tileCol=6424&tileRow=50233&tileMatrix=19&LAYER=ORTHO_RAW_IGNF_LAMB93&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0
* queryString="SERVICE=WMTS&REQUEST=GetTile&tileCol=6424&tileRow=50233&tileMatrix=19&LAYER=ORTHO_RAW_IGNF_LAMB93&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0"
* hostName="localhost"
* scriptName="/target/bin/rok4"
* service="wmts" (en minuscules)
* operationType="Gettile" (en minuscules)
*/

HttpRequest* rok4InitRequest ( const char* queryString, const char* hostName, const char* scriptName, const char* https ) {
    std::string strQuery=queryString;
    HttpRequest* request=new HttpRequest;
    request->queryString=new char[strQuery.length() +1];
    strcpy ( request->queryString,strQuery.c_str() );
    request->hostName=new char[strlen ( hostName ) +1];
    strcpy ( request->hostName,hostName );
    request->scriptName=new char[strlen ( scriptName ) +1];
    strcpy ( request->scriptName,scriptName );
    Request* rok4Request=new Request ( ( char* ) strQuery.c_str(), ( char* ) hostName, ( char* ) scriptName, ( char* ) https );
    request->service=new char[rok4Request->service.length() +1];
    strcpy ( request->service,rok4Request->service.c_str() );
    request->operationType=new char[rok4Request->request.length() +1];
    strcpy ( request->operationType,rok4Request->request.c_str() );
    delete rok4Request;
    return request;
}

/**
* @brief Implementation de l'operation GetCapabilities pour le WMTS
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @return Reponse (allouee ici, doit etre desallouee ensuite)
*/

HttpResponse* rok4GetWMTSCapabilities ( const char* queryString, const char* hostName, const char* scriptName,const char* https ,Rok4Server* server ) {
    std::string strQuery=queryString;
    Request* request=new Request ( ( char* ) strQuery.c_str(), ( char* ) hostName, ( char* ) scriptName, ( char* ) https );
    DataStream* stream=server->WMTSGetCapabilities ( request );
    DataSource* source= new BufferedDataSource ( *stream );
    HttpResponse* response=initResponseFromSource ( /*new BufferedDataSource(*stream)*/source );
    delete request;
    delete stream;
    return response;
}

/**
* @brief Implementation de l'operation GetTile
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @return Reponse (allouee ici, doit etre desallouee ensuite)
*/

HttpResponse* rok4GetTile ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server ) {
    std::string strQuery=queryString;
    Request* request=new Request ( ( char* ) strQuery.c_str(), ( char* ) hostName, ( char* ) scriptName, ( char* ) https );
    DataSource* source=server->getTile ( request );
    HttpResponse* response=initResponseFromSource ( source );
    delete request;
    delete source;
    return response;
}

/**
* @brief Implementation de l'operation GetTile modifiee
* @brief La tuile n'est pas lue, les elements recuperes sont les references de la tuile : le fichier dans lequel elle est stockee et les positions d'enregistrement(sur 4 octets) dans ce fichier de l'index du premier octet de la tuile et de sa taille
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @param[out] tileRef : reference de la tuile (la variable filename est allouee ici et doit etre desallouee ensuite)
* @param[out] palette : palette à ajouter, NULL sinon.
* @return Reponse en cas d'exception, NULL sinon
*/

HttpResponse* rok4GetTileReferences ( const char* queryString, const char* hostName, const char* scriptName, const char* https, Rok4Server* server, TileRef* tileRef, TilePalette* palette ) {
    // Initialisation
    std::string strQuery=queryString;

    Request* request=new Request ( ( char* ) strQuery.c_str(), ( char* ) hostName, ( char* ) scriptName, ( char* ) https );
    Layer* layer;
        std::string tmId,mimeType,format;
    int x,y;
    Style* style =0;
    // Analyse de la requete
        DataSource* errorResp = request->getTileParam(server->getServicesConf(), server->getTmsList(), server->getLayerList(), layer, tmId, x, y, mimeType, style);
    // Exception
    if ( errorResp ) {
        LOGGER_ERROR ( "Probleme dans les parametres de la requete getTile" );
        HttpResponse* error=initResponseFromSource ( errorResp );
        delete errorResp;
        return error;
    }

    // References de la tuile
    std::map<std::string, Level*>::iterator itLevel=layer->getDataPyramid()->getLevels().find ( tmId );
    if ( itLevel==layer->getDataPyramid()->getLevels().end() )
        return 0;
    Level* level=layer->getDataPyramid()->getLevels().find ( tmId )->second;
    int n= ( y%level->getTilesPerHeight() ) *level->getTilesPerWidth() + ( x%level->getTilesPerWidth() );

    tileRef->posoff=2048+4*n;
    tileRef->possize=2048+4*n +level->getTilesPerWidth() *level->getTilesPerHeight() *4;

    std::string imageFilePath=level->getFilePath ( x, y );
    tileRef->filename=new char[imageFilePath.length() +1];
    strcpy ( tileRef->filename,imageFilePath.c_str() );

	tileRef->type=new char[mimeType.length()+1];
	strcpy(tileRef->type,mimeType.c_str());

    tileRef->width=level->getTm().getTileW();
    tileRef->height=level->getTm().getTileH();
    tileRef->channels=level->getChannels();

	format = format::toString(layer->getDataPyramid()->getFormat());
	tileRef->format= new char[format.length()+1];
	strcpy(tileRef->format, format.c_str());
	
    //Palette uniquement PNG pour le moment
	if (mimeType == "image/png"){
        palette->size = style->getPalette()->getPalettePNGSize();
        palette->data = style->getPalette()->getPalettePNG();
    } else {
        palette->size = 0;
        palette->data = NULL;
    }

    delete request;
    return 0;
}

/**
* @brief Implementation de l'operation GetNoDataTile
* @brief La tuile n'est pas lue, les elements recuperes sont les references de la tuile : le fichier dans lequel elle est stockee et les positions d'enregistrement(sur 4 octets) dans ce fichier de l'index du premier octet de la tuile et de sa taille
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @param[out] tileRef : reference de la tuile (la variable filename est allouee ici et doit etre desallouee ensuite)
* @param[out] palette : palette à ajouter, NULL sinon.
* @return Reponse en cas d'exception, NULL sinon
*/
HttpResponse* rok4GetNoDataTileReferences ( const char* queryString, const char* hostName, const char* scriptName, const char* https, Rok4Server* server, TileRef* tileRef, TilePalette* palette ) {
// Initialisation
    std::string strQuery=queryString;

    Request* request=new Request ( ( char* ) strQuery.c_str(), ( char* ) hostName, ( char* ) scriptName, ( char* ) https );
    Layer* layer;
    std::string tmId,format;
    int x,y;
    Style* style =0;
    // Analyse de la requete
    DataSource* errorResp = request->getTileParam ( server->getServicesConf(), server->getTmsList(), server->getLayerList(), layer, tmId, x, y, format, style );
    // Exception
    if ( errorResp ) {
        LOGGER_ERROR ( "Probleme dans les parametres de la requete getTile" );
        HttpResponse* error=initResponseFromSource ( errorResp );
        delete errorResp;
        return error;
    }

    // References de la tuile
    std::map<std::string, Level*>::iterator itLevel=layer->getDataPyramid()->getLevels().find ( tmId );
    if ( itLevel==layer->getDataPyramid()->getLevels().end() )
        return 0;
    Level* level=layer->getDataPyramid()->getLevels().find ( tmId )->second;

    tileRef->posoff=2048;
    tileRef->possize=2048+4;

    std::string imageFilePath=level->getNoDataFilePath();
    tileRef->filename=new char[imageFilePath.length() +1];
    strcpy ( tileRef->filename,imageFilePath.c_str() );

    tileRef->type=new char[format.length() +1];
    strcpy ( tileRef->type,format.c_str() );

    tileRef->width=level->getTm().getTileW();
    tileRef->height=level->getTm().getTileH();
    tileRef->channels=level->getChannels();

    //Palette uniquement PNG pour le moment
    if ( format == "image/png" ) {
        palette->size = style->getPalette()->getPalettePNGSize();
        palette->data = style->getPalette()->getPalettePNG();
    } else {
        palette->size = 0;
        palette->data = NULL;
    }

    delete request;
    return 0;
}


/**
* @brief Construction d'un en-tete TIFF
* @deprecated
*/

TiffHeader* rok4GetTiffHeader ( int width, int height, int channels ) {
    TiffHeader* header = new TiffHeader;
    RawImage* rawImage=new RawImage ( width,height,channels,0 );
    TiffEncoder tiffStream ( rawImage );
    tiffStream.read ( header->data,128 );
    return header;
}

/**
* @brief Construction d'un en-tete TIFF
*/

TiffHeader* rok4GetTiffHeaderFormat(int width, int height, int channels, char* format, uint32_t possize)
{
	TiffHeader* header = new TiffHeader;
	size_t tiffHeaderSize;
	const uint8_t* tiffHeader;
	TiffHeaderDataSource* fullTiffDS = new TiffHeaderDataSource(0,format::fromString(format),channels,width,height,possize);
	tiffHeader = fullTiffDS->getData(tiffHeaderSize);
	memcpy(header->data,tiffHeader,tiffHeaderSize); 
}





/**
* @brief Construction d'un en-tete PNG avec Palette
*/

PngPaletteHeader* rok4GetPngPaletteHeader ( int width, int height, TilePalette* palette ) {
    PngPaletteHeader* header = new PngPaletteHeader;
    //RawImage* rawImage=new RawImage(width,height,1,0);
    Palette rok4Palette = Palette ( palette->size,palette->data );
    PNGEncoder pngStream ( new ImageDecoder ( 0,width,height,1 ),&rok4Palette );
    header->size = 33 + palette->size;
    header->data = ( uint8_t* ) malloc ( header->size+1 );
    pngStream.read ( header->data,header->size );
    return header;
}


/**
* @brief Renvoi d'une exception pour une operation non prise en charge
*/

HttpResponse* rok4GetOperationNotSupportedException ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server ) {

    std::string strQuery=queryString;
    Request* request=new Request ( ( char* ) strQuery.c_str(), ( char* ) hostName, ( char* ) scriptName, ( char* ) https );
    DataSource* source=new SERDataSource ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,"L'operation "+request->request+" n'est pas prise en charge par ce serveur.","wmts" ) );
    HttpResponse* response=initResponseFromSource ( source );
    delete request;
    delete source;
    return response;
}

/**
* @brief Suppression d'une requete
*/

void rok4DeleteRequest ( HttpRequest* request ) {
    delete[] request->queryString;
    delete[] request->hostName;
    delete[] request->scriptName;
    delete[] request->service;
    delete[] request->operationType;
    delete request;
}

/**
* @brief Suppression d'une reponse
*/

void rok4DeleteResponse ( HttpResponse* response ) {
    delete[] response->type;
    delete[] response->content;
    delete response;
}

/**
* @brief Suppression des champs d'une reference de tuile
* La reference n est pas supprimee
*/

void rok4FlushTileRef ( TileRef* tileRef ) {
    delete[] tileRef->filename;
    delete[] tileRef->type;
	delete[] tileRef->format;
}

/**
* @brief Suppression d'un en-tete TIFF
*/

void rok4DeleteTiffHeader ( TiffHeader* header ) {
    delete header;
}

/**
* @brief Suppression d'un en-tete Png avec Palette
*/

void rok4DeletePngPaletteHeader ( PngPaletteHeader* header ) {
    delete header;
}

/**
* @brief Suppression d'une Palette
*/

void rok4DeleteTilePalette ( TilePalette* palette ) {
    delete palette;
}


/**
* @brief Extinction du serveur
*/

void rok4KillServer ( Rok4Server* server ) {
    LOGGER_INFO ( "Extinction du serveur ROK4" );

    std::map<std::string,TileMatrixSet*>::iterator iTms;
    for ( iTms=server->getTmsList().begin();iTms!=server->getTmsList().end();iTms++ )
        delete ( *iTms ).second;

    std::map<std::string, Layer*>::iterator iLayer;
    for ( iLayer=server->getLayerList().begin();iLayer!=server->getLayerList().end();iLayer++ )
        delete ( *iLayer ).second;

    // TODO Supprimer le logger
}

