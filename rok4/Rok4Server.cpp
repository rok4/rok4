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

/**
 * \file Rok4Server.cpp
 * \~french
 * \brief Implémentation de la classe Rok4Server et du programme principal
 * \~english
 * \brief Implement the Rok4Server class, handling the event loop
 */

#include "Image.h"

#include "Rok4Server.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <map>
#include <fstream>
#include <cstring>
#include <proj_api.h>
#include <csignal>

#include "config.h"
#include "intl.h"
#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"
#include "BilEncoder.h"
#include "Format.h"
#include "Message.h"
#include "StyledImage.h"
#include "Logger.h"
#include "TileMatrixSet.h"
#include "Layer.h"
#include "ServiceException.h"
#include "fcgiapp.h"
#include "PaletteDataSource.h"
#include "EstompageImage.h"
#include "MergeImage.h"

void* Rok4Server::thread_loop ( void* arg ) {
    Rok4Server* server = ( Rok4Server* ) ( arg );
    FCGX_Request fcgxRequest;
    if ( FCGX_InitRequest ( &fcgxRequest, server->sock, FCGI_FAIL_ACCEPT_ON_INTR ) !=0 ) {
        LOGGER_FATAL ( _ ( "Le listener FCGI ne peut etre initialise" ) );
    }

    while ( server->isRunning() ) {
        std::string content;
        bool postRequest;

        int rc;
        if ( ( rc=FCGX_Accept_r ( &fcgxRequest ) ) < 0 ) {
            if ( rc != -4 ) { // Cas différent du redémarrage
                LOGGER_ERROR ( _ ( "FCGX_InitRequest renvoie le code d'erreur" ) << rc );
            }
            //std::cerr <<"FCGX_InitRequest renvoie le code d'erreur" << rc << std::endl;
            break;
        }
        //DEBUG: La boucle suivante permet de lister les valeurs dans fcgxRequest.envp
        /*char **p;
        for (p = fcgxRequest.envp; *p; ++p) {
            LOGGER_DEBUG((char*)*p);
        }*/

        Request* request;



        postRequest = ( server->servicesConf.isPostEnabled() ?strcmp ( FCGX_GetParam ( "REQUEST_METHOD",fcgxRequest.envp ),"POST" ) ==0:false );

        if ( postRequest ) { // Post Request
            char* contentBuffer = ( char* ) malloc ( sizeof ( char ) *200 );
            while ( FCGX_GetLine ( contentBuffer,200,fcgxRequest.in ) ) {
                content.append ( contentBuffer );
            }
            free ( contentBuffer );
            contentBuffer= NULL;
            LOGGER_DEBUG ( _ ( "Request Content :" ) << std::endl << content );
            request = new Request ( FCGX_GetParam ( "QUERY_STRING", fcgxRequest.envp ),
                                    FCGX_GetParam ( "HTTP_HOST", fcgxRequest.envp ),
                                    FCGX_GetParam ( "SCRIPT_NAME", fcgxRequest.envp ),
                                    FCGX_GetParam ( "HTTPS", fcgxRequest.envp ),
                                    content );



        } else { // Get Request

            /* On espère récupérer le nom du host tel qu'il est exprimé dans la requete avec HTTP_HOST.
             * De même, on espère récupérer le path tel qu'exprimé dans la requête avec SCRIPT_NAME.
             */

            request = new Request ( FCGX_GetParam ( "QUERY_STRING", fcgxRequest.envp ),
                                    FCGX_GetParam ( "HTTP_HOST", fcgxRequest.envp ),
                                    FCGX_GetParam ( "SCRIPT_NAME", fcgxRequest.envp ),
                                    FCGX_GetParam ( "HTTPS", fcgxRequest.envp )
                                  );
        }
        server->processRequest ( request, fcgxRequest );
        delete request;

        FCGX_Finish_r ( &fcgxRequest );
        FCGX_Free ( &fcgxRequest,1 );
    }
    LOGGER_DEBUG ( _ ( "Extinction du thread" ) );
    Logger::stopLogger();
    return 0;
}

Rok4Server::Rok4Server ( int nbThread, ServicesConf& servicesConf, std::map<std::string,Layer*> &layerList,
                         std::map<std::string,TileMatrixSet*> &tmsList, std::map<std::string,Style*> &styleList,
                         std::string socket, int backlog, bool supportWMTS, bool supportWMS ) :
    sock ( 0 ), servicesConf ( servicesConf ), layerList ( layerList ), tmsList ( tmsList ),
    styleList ( styleList ), threads ( nbThread ), socket ( socket ), backlog ( backlog ),
    running ( false ), notFoundError ( NULL ), supportWMTS ( supportWMTS ), supportWMS ( supportWMS ) {

    if ( supportWMS ) {
        LOGGER_DEBUG ( _ ( "Build WMS Capabilities" ) );
        buildWMSCapabilities();
    }
    if ( supportWMTS ) {
        LOGGER_DEBUG ( _ ( "Build WMTS Capabilities" ) );
        buildWMTSCapabilities();
    }
}

Rok4Server::~Rok4Server() {
    if ( notFoundError ) {
        delete notFoundError;
        notFoundError = NULL;
    }
}

void Rok4Server::initFCGI() {
    int init=FCGX_Init();
    if ( !socket.empty() ) {
        LOGGER_INFO ( _ ( "Listening on " ) << socket );
        sock = FCGX_OpenSocket ( socket.c_str(), backlog );
    }
}

void Rok4Server::killFCGI() {
    FCGX_CloseSocket ( sock );
    FCGX_Close();
}

void Rok4Server::run() {
    running = true;

    for ( int i = 0; i < threads.size(); i++ ) {
        pthread_create ( & ( threads[i] ), NULL, Rok4Server::thread_loop, ( void* ) this );
    }
    for ( int i = 0; i < threads.size(); i++ )
        pthread_join ( threads[i], NULL );
}

void Rok4Server::terminate() {
    running = false;
    //FCGX_ShutdownPending();
    // Terminate FCGI Thread
    for ( int i = 0; i < threads.size(); i++ ) {
        pthread_kill ( threads[i], SIGPIPE );
    }


}

bool Rok4Server::hasParam ( std::map<std::string, std::string>& option, std::string paramName ) {
    std::map<std::string, std::string>::iterator it = option.find ( paramName );
    if ( it == option.end() ) {
        return false;
    }
    return true;
}

std::string Rok4Server::getParam ( std::map<std::string, std::string>& option, std::string paramName ) {
    std::map<std::string, std::string>::iterator it = option.find ( paramName );
    if ( it == option.end() ) {
        return "";
    }
    return it->second;
}

DataStream* Rok4Server::WMSGetCapabilities ( Request* request ) {
    if ( !supportWMS ) {
        // Return Error
    }
    std::string version;
    DataStream* errorResp = request->getCapWMSParam ( servicesConf,version );
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getCapabilities" ) );
        return errorResp;
    }

    /* concaténation des fragments invariant de capabilities en intercalant les
     * parties variables dépendantes de la requête */

    std::string capa = wmsCapaFrag[0] + request->scheme + request->hostName;
    for ( int i=1; i < wmsCapaFrag.size()-1; i++ ) {
        capa = capa + wmsCapaFrag[i] + request->scheme + request->hostName + request->path + "?";
    }
    capa = capa + wmsCapaFrag.back();

    return new MessageDataStream ( capa,"text/xml" );
}

DataStream* Rok4Server::WMTSGetCapabilities ( Request* request ) {
    if ( !supportWMTS ) {
        // Return Error
    }
    std::string version;
    DataStream* errorResp = request->getCapWMTSParam ( servicesConf,version );
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getCapabilities" ) );
        return errorResp;
    }

    /* concaténation des fragments invariant de capabilities en intercalant les
      * parties variables dépendantes de la requête */
    std::string capa = "";
    for ( int i=0; i < wmtsCapaFrag.size()-1; i++ ) {
        capa = capa + wmtsCapaFrag[i] + request->scheme + request->hostName + request->path +"?";
    }
    capa = capa + wmtsCapaFrag.back();

    return new MessageDataStream ( capa,"application/xml" );
}

DataStream* Rok4Server::getMap ( Request* request ) {
    std::vector<Layer*> layers;
    BoundingBox<double> bbox ( 0.0, 0.0, 0.0, 0.0 );
    int width, height;
    CRS crs;
    std::string format;
    std::vector<Style*> styles;
    std::map <std::string, std::string > format_option;
    std::vector<Image*> images;


    // Récupération des paramètres
    DataStream* errorResp = request->getMapParam ( servicesConf, layerList, layers, bbox, width, height, crs, format ,styles, format_option );
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getMap" ) );
        return errorResp;
    }

    int error;
    Image* image;
    for ( int i = 0 ; i < layers.size(); i ++ ) {
        Image* curImage = layers.at ( i )->getbbox ( servicesConf, bbox, width, height, crs, error );

        LOGGER_DEBUG ( _ ( "GetMap de Style : " ) << styles.at ( i )->getId() << _ ( " pal size : " ) <<styles.at ( i )->getPalette()->getPalettePNGSize() );

        if ( curImage == 0 ) {
            switch ( error ) {

            case 1: {
                return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "bbox invalide" ),"wms" ) );
            }
            case 2: {
                return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "bbox trop grande" ),"wms" ) );
            }
            default : {
                return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wms" ) );
            }
            }
        }

        Rok4Format::eformat_data pyrType = layers.at ( i )->getDataPyramid()->getFormat();

        if ( servicesConf.isFullStyleCapable() ) {
            if ( styles.at ( i )->isEstompage() ) {
                LOGGER_DEBUG ( _ ( "Estompage" ) );
                curImage = new EstompageImage ( curImage,styles.at ( i )->getAngle(),styles.at ( i )->getExaggeration(), styles.at ( i )->getCenter() );
                switch ( pyrType ) {
                    //Only use int8 output whith estompage
                case Rok4Format::TIFF_RAW_FLOAT32 :
                    pyrType = Rok4Format::TIFF_RAW_INT8;
                    break;
                case Rok4Format::TIFF_ZIP_FLOAT32 :
                    pyrType = Rok4Format::TIFF_ZIP_INT8;
                    break;
                case Rok4Format::TIFF_LZW_FLOAT32 :
                    pyrType = Rok4Format::TIFF_LZW_INT8;
                    break;
                case Rok4Format::TIFF_PKB_FLOAT32 :
                    pyrType = Rok4Format::TIFF_PKB_INT8;
                    break;
                default:
                    break;
                }
            }

            if ( styles.at ( i ) && curImage->channels == 1 && ! ( styles.at ( i )->getPalette()->getColoursMap()->empty() ) ) {
                if ( format == "image/png" && layers.size() == 1 ) {
                    switch ( pyrType ) {

                    case Rok4Format::TIFF_RAW_FLOAT32 :
                    case Rok4Format::TIFF_ZIP_FLOAT32 :
                    case Rok4Format::TIFF_LZW_FLOAT32 :
                    case Rok4Format::TIFF_PKB_FLOAT32 :
                        curImage = new StyledImage ( curImage, styles.at ( i )->getPalette()->isNoAlpha()?3:4 , styles.at ( i )->getPalette() );
                    default:
                        break;
                    }
                } else {
                    curImage = new StyledImage ( curImage, styles.at ( i )->getPalette()->isNoAlpha()?3:4, styles.at ( i )->getPalette() );
                }
            }

        }

        images.push_back ( curImage );
    }

    //Use background image format.
    Rok4Format::eformat_data pyrType = layers.at ( 0 )->getDataPyramid()->getFormat();
    image = images.at ( 0 );
    //if ( images.size() > 1  || (styles.at( 0 ) && (styles.at( 0 )->isEstompage() || !styles.at( 0 )->getPalette()->getColoursMap()->empty()) ) ) {
    if ( (styles.at( 0 ) && (styles.at( 0 )->isEstompage() || !styles.at( 0 )->getPalette()->getColoursMap()->empty()) ) ) {

        switch ( pyrType ) {
            //Only use int8 output with estompage
        case Rok4Format::TIFF_RAW_FLOAT32 :
            pyrType = Rok4Format::TIFF_RAW_INT8;
            break;
        case Rok4Format::TIFF_ZIP_FLOAT32 :
            pyrType = Rok4Format::TIFF_ZIP_INT8;
            break;
        case Rok4Format::TIFF_LZW_FLOAT32 :
            pyrType = Rok4Format::TIFF_LZW_INT8;
            break;
        case Rok4Format::TIFF_PKB_FLOAT32 :
            pyrType = Rok4Format::TIFF_PKB_INT8;
            break;
        default:
            break;
        }
    }
    if (images.size() > 1 ){

        MergeImageFactory MIF;
        int spp = images.at ( 0 )->channels;
	int bg[spp];
	int transparentColor[spp];
	
	switch (pyrType) {
	  case Rok4Format::TIFF_RAW_FLOAT32 :
	  case Rok4Format::TIFF_ZIP_FLOAT32 :
	  case Rok4Format::TIFF_LZW_FLOAT32 :
	  case Rok4Format::TIFF_PKB_FLOAT32 :
	    switch(spp) {
	      case 1:
		  bg[0] = -99999.0;
		  break;
	      case 2:
		  bg[0] = -99999.0; bg[1] = 0;
		  break;
	      case 3:
		  bg[0] = -99999.0; bg[1] = -99999.0; bg[2] = -99999.0;
		  break;
	      case 4:
		  bg[0] = -99999.0; bg[1] = -99999.0; bg[2] = -99999.0; bg[4] = 0;
		  break;
	      default:
		  memset(bg, 0, sizeof(int) * spp);
		  break;                
	    }
	    memccpy(transparentColor, bg, spp, sizeof(int));
	    break;
	  case Rok4Format::TIFF_RAW_INT8 :
	  case Rok4Format::TIFF_ZIP_INT8 :
	  case Rok4Format::TIFF_LZW_INT8 :
	  case Rok4Format::TIFF_PKB_INT8 :
	  default :
	    switch(spp) {
	      case 1:
		  bg[0] = 255;
		  break;
	      case 2:
		  bg[0] = 255; bg[1] = 0;
		  break;
	      case 3:
		  bg[0] = 255; bg[1] = 255; bg[2] = 255;
		  break;
	      case 4:
		  bg[0] = 255; bg[1] = 255; bg[2] = 255; bg[4] = 0;
		  break;
	      default:
		  memset(bg, 0, sizeof(uint8_t) * spp);
		  break;                
	    }
	    break;
	}
	
	image = MIF.createMergeImage(images,spp,bg,transparentColor,Merge::ALPHATOP);

        if ( image == NULL ) {
            LOGGER_ERROR ( "Impossible de fusionner les images des differentes couches" );
            return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wms" ) );
        }
    }
    
    image->setCRS(crs);
    image->setBbox(bbox);

    if ( format=="image/png" ) {
        if ( layers.size() == 1 ) {
            return new PNGEncoder ( image,styles.at ( 0 )->getPalette() );
        } else {
            return new PNGEncoder ( image,NULL );
        }

    } else if ( format == "image/tiff" || format == "image/geotiff" ) { // Handle compression option
	bool isGeoTiff = (format == "image/geotiff");

        switch ( pyrType ) {

        case Rok4Format::TIFF_RAW_FLOAT32 :
        case Rok4Format::TIFF_ZIP_FLOAT32 :
        case Rok4Format::TIFF_LZW_FLOAT32 :
        case Rok4Format::TIFF_PKB_FLOAT32 :
            if ( getParam ( format_option,"compression" ).compare ( "lzw" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_LZW_FLOAT32, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "deflate" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_ZIP_FLOAT32, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "raw" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_RAW_FLOAT32, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "packbits" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_PKB_FLOAT32, isGeoTiff );
            }
            return TiffEncoder::getTiffEncoder ( image, pyrType, isGeoTiff );
        case Rok4Format::TIFF_RAW_INT8 :
        case Rok4Format::TIFF_ZIP_INT8 :
        case Rok4Format::TIFF_LZW_INT8 :
        case Rok4Format::TIFF_PKB_INT8 :
            if ( getParam ( format_option,"compression" ).compare ( "lzw" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_LZW_INT8, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "deflate" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_ZIP_INT8, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "raw" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_RAW_INT8, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "packbits" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_PKB_INT8, isGeoTiff );
            }
            return TiffEncoder::getTiffEncoder ( image, pyrType, isGeoTiff );
        default:
            if ( getParam ( format_option,"compression" ).compare ( "lzw" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_LZW_INT8, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "deflate" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_ZIP_INT8, isGeoTiff );
            }
            if ( getParam ( format_option,"compression" ).compare ( "packbits" ) ==0 ) {
                return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_PKB_INT8, isGeoTiff );
            }
            return TiffEncoder::getTiffEncoder ( image, Rok4Format::TIFF_RAW_INT8, isGeoTiff );
        }
    } else if ( format == "image/jpeg" ) {
        return new JPEGEncoder ( image );
    } else if ( format == "image/x-bil;bits=32" ) {
        return new BilEncoder ( image );
    }

    LOGGER_ERROR ( "Le format "<<format<<" ne peut etre traite" );

    return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Le format " ) +format+_ ( " ne peut etre traite" ),"wms" ) );
}

DataSource* Rok4Server::getTile ( Request* request ) {
    Layer* L;
    std::string tileMatrix,format;
    int tileCol,tileRow;
    bool noDataError;
    Style* style=0;

    // Récupération des parametres de la requete
    DataSource* errorResp = request->getTileParam ( servicesConf, tmsList, layerList, L, tileMatrix, tileCol, tileRow, format, style, noDataError );

    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getTile" ) );
        return errorResp;
    }
    errorResp = NULL;
    if ( noDataError ) {
        if ( !notFoundError ) {
            notFoundError = new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND, _ ( "No data found" ), "wmts" ) );
        }
        errorResp = notFoundError;
    }

    DataSource* tileSource;
    // Avoid using unnecessary palette
    if ( format == "image/png" ) {
        tileSource= new PaletteDataSource ( L->gettile ( tileCol, tileRow, tileMatrix, errorResp ),style->getPalette() );
    } else {
        tileSource= L->gettile ( tileCol, tileRow, tileMatrix , errorResp );
    }


    return tileSource;
}

void Rok4Server::processWMTS ( Request* request, FCGX_Request&  fcgxRequest ) {
    if ( request->request == "getcapabilities" ) {
        S.sendresponse ( WMTSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == "gettile" ) {
        S.sendresponse ( getTile ( request ), &fcgxRequest );
    } else if ( request->request == "getversion" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->request+_ ( " n'est pas prise en charge par ce serveur." ) + ROK4_INFO,"wmts" ) ),&fcgxRequest );
    } else if ( request->request == "" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre REQUEST n'est pas renseigne." ) ,"wmts" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,_ ( "L'operation " ) +request->request+_ ( " n'est pas prise en charge par ce serveur." ),"wmts" ) ),&fcgxRequest );
    }
}

void Rok4Server::processWMS ( Request* request, FCGX_Request&  fcgxRequest ) {
    if ( request->request == "getcapabilities" ) {
        S.sendresponse ( WMSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == "getmap" ) {
        S.sendresponse ( getMap ( request ), &fcgxRequest );
    } else if ( request->request == "getversion" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->request+_ ( " n'est pas prise en charge par ce serveur." ) + ROK4_INFO,"wms" ) ),&fcgxRequest );
    } else if ( request->request == "" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre REQUEST n'est pas renseigne." ) ,"wms" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->request+_ ( " n'est pas prise en charge par ce serveur." ),"wms" ) ),&fcgxRequest );
    }
}

void Rok4Server::processRequest ( Request * request, FCGX_Request&  fcgxRequest ) {
    if ( supportWMTS && request->service == "wmts" ) {
        processWMTS ( request, fcgxRequest );
        //Service is not mandatory in GetMap request in WMS 1.3.0 and GetFeatureInfo
    } else if ( supportWMS && ( request->service=="wms" || request->request == "getmap" ) ) {
        processWMS ( request, fcgxRequest );
    } else if ( request->service == "" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre SERVICE n'est pas renseigne." ) ,"xxx" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le service " ) +request->service+_ ( " est inconnu pour ce serveur." ),"wmts" ) ),&fcgxRequest );
    }
}


