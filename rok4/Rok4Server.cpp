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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

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
#include "ProcessFactory.h"
#include "Rok4Image.h"
#include "LoggerSpecific.h"

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

        server->parallelProcess->checkAllPid();
    }
    LOGGER_DEBUG ( _ ( "Extinction du thread" ) );
    Logger::stopLogger();
    return 0;
}

Rok4Server::Rok4Server ( int nbThread, ServicesConf& servicesConf, std::map<std::string,Layer*> &layerList,
                         std::map<std::string,TileMatrixSet*> &tmsList, std::map<std::string,Style*> &styleList,
                         std::string socket, int backlog, bool supportWMTS, bool supportWMS, int nbProcess ) :
    sock ( 0 ), servicesConf ( servicesConf ), layerList ( layerList ), tmsList ( tmsList ),
    styleList ( styleList ), threads ( nbThread ), socket ( socket ), backlog ( backlog ),
    running ( false ), notFoundError ( NULL ), supportWMTS ( supportWMTS ), supportWMS ( supportWMS ) {

    if ( supportWMS ) {
        LOGGER_DEBUG ( _ ( "Build WMS Capabilities 1.3.0" ) );
        buildWMS130Capabilities();
        //---- WMS 1.1.1
        LOGGER_DEBUG ( _ ( "Build WMS Capabilities 1.1.1" ) );
        buildWMS111Capabilities();
        //----
    }
    if ( supportWMTS ) {
        LOGGER_DEBUG ( _ ( "Build WMTS Capabilities" ) );
        buildWMTSCapabilities();
    }
    //initialize processFactory
    if (nbProcess > MAX_NB_PROCESS) {
        nbProcess = MAX_NB_PROCESS;
    }
    if (nbProcess < 0) {
        nbProcess = DEFAULT_NB_PROCESS;
    }
    parallelProcess = new ProcessFactory(nbProcess);
}

Rok4Server::~Rok4Server() {
    if ( notFoundError ) {
        delete notFoundError;
        notFoundError = NULL;
    }
    delete parallelProcess;
    parallelProcess = NULL;
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

void Rok4Server::run(sig_atomic_t signal_pending) {
    running = true;

    for ( int i = 0; i < threads.size(); i++ ) {
        pthread_create ( & ( threads[i] ), NULL, Rok4Server::thread_loop, ( void* ) this );
    }
    
    if (signal_pending != 0 ) {
	raise( signal_pending );
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
    std::string capa;
    std::vector<std::string> capaFrag;
    std::map<std::string,std::vector<std::string> >::iterator it = wmsCapaFrag.find(version);
    capaFrag = it->second;
    capa = capaFrag[0] + request->scheme + request->hostName;
    for ( int i=1; i < capaFrag.size()-1; i++ ) {
        capa = capa + capaFrag[i] + request->scheme + request->hostName + request->path + "?";
    }
    capa = capa + capaFrag.back();

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

        if (layers.at(i)->getWMSAuthorized()) {

            Image* curImage = layers.at ( i )->getbbox ( servicesConf, bbox, width, height, crs, error );
            Rok4Format::eformat_data pyrType = layers.at ( i )->getDataPyramid()->getFormat();
            Style* style = styles.at(i);
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

            Image *image = styleImage(curImage, pyrType, style, format, layers.size());

            images.push_back ( image );
        } else {

            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +layers.at(i)->getTitle()+_ ( " unknown " ),"wms" ) );

        }
    }


    //Use background image format.
    Rok4Format::eformat_data pyrType = layers.at ( 0 )->getDataPyramid()->getFormat();
    Style* style = styles.at(0);

    image = mergeImages(images, pyrType, style, crs, bbox);

    DataStream * stream = formatImage(image, format, pyrType, format_option, layers.size(), style);

    return stream;
}

Image *Rok4Server::styleImage(Image *curImage, Rok4Format::eformat_data pyrType,
                            Style *style, std::string format, int size) {



    if ( servicesConf.isFullStyleCapable() ) {
        if ( style->isEstompage() ) {
            LOGGER_DEBUG ( _ ( "Estompage" ) );
            curImage = new EstompageImage ( curImage,style->getAngle(),style->getExaggeration(), style->getCenter() );
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

        if ( style && curImage->channels == 1 && ! ( style->getPalette()->getColoursMap()->empty() ) ) {
            if ( format == "image/png" && size == 1 ) {
                switch ( pyrType ) {

                case Rok4Format::TIFF_RAW_FLOAT32 :
                case Rok4Format::TIFF_ZIP_FLOAT32 :
                case Rok4Format::TIFF_LZW_FLOAT32 :
                case Rok4Format::TIFF_PKB_FLOAT32 :
                    curImage = new StyledImage ( curImage, style->getPalette()->isNoAlpha()?3:4 , style->getPalette() );
                default:
                    break;
                }
            } else {
                curImage = new StyledImage ( curImage, style->getPalette()->isNoAlpha()?3:4, style->getPalette() );
            }
        }

    }

    return curImage;

}

Image * Rok4Server::mergeImages(std::vector<Image*> images, Rok4Format::eformat_data &pyrType,
                                Style *style, CRS crs, BoundingBox<double> bbox) {


    Image *image = images.at ( 0 );
    //if ( images.size() > 1  || (styles.at( 0 ) && (styles.at( 0 )->isEstompage() || !styles.at( 0 )->getPalette()->getColoursMap()->empty()) ) ) {
    if ( (style && (style->isEstompage() || !style->getPalette()->getColoursMap()->empty()) ) ) {

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
            return NULL;
        }
    }

    image->setCRS(crs);
    image->setBbox(bbox);

    return image;

}

DataStream * Rok4Server::formatImage(Image *image, std::string format, Rok4Format::eformat_data pyrType,
                                     std::map <std::string, std::string > format_option,
                                     int size, Style *style) {

    if ( format=="image/png" ) {
        if ( size == 1 ) {
            return new PNGEncoder ( image,style->getPalette() );
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


    //Si le WMTS n'est pas authorisé pour ce layer, on renvoit une erreur
    if (!(L->getWMTSAuthorized())) {
        std::string Title = L->getId();
        delete L;
        L = NULL;
        delete style;
        style = NULL;
        return new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Layer " ) +Title+_ ( " unknown " ),"wmts" ) );
    }


    DataSource* tileSource;

    if (L->getDataPyramid()->getOnDemand()) {
        //Si la pyramide est à la demande, on doit créer la tuile
        if (L->getDataPyramid()->getOnFly()) {
            tileSource = getTileOnFly(L, tileMatrix, tileCol, tileRow, style, format, errorResp);
        } else {
            tileSource = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
        }

    } else {
        //GetTile normal, on renvoit la tuile
        tileSource = getTileUsual(L, format, tileCol, tileRow, tileMatrix, errorResp, style) ;
    }

    return tileSource;

}



DataSource *Rok4Server::getTileUsual(Layer* L,std::string format, int tileCol, int tileRow, std::string tileMatrix, DataSource *errorResp, Style *style) {

    DataSource* tileSource;
    // Avoid using unnecessary palette
    if ( format == "image/png" ) {
        tileSource= new PaletteDataSource ( L->gettile ( tileCol, tileRow, tileMatrix, errorResp ),style->getPalette() );
    } else {
        tileSource= L->gettile ( tileCol, tileRow, tileMatrix , errorResp );
    }
    return tileSource;

}

DataSource *Rok4Server::getTileOnDemand(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format) {
    //On va créer la tuile sur demande

    //Variables
    std::vector<Image*> images;
    Image *curImage;
    Image *image;
    Image *mergeImage;
    std::string bLevel;
    double Col, Row, xmin, ymin, xmax, ymax, xo, yo, resolution;
    int tileH, tileW, width, height, error;
    Rok4Format::eformat_data pyrType;
    Style * bStyle;
    std::map <std::string, std::string > format_option;
    std::vector<Pyramid*> bPyr;
    bool specific = false;

    //la correspondance est assurée par les vérifications qui ont eu lieu dans getTile()
    std::string level = tileMatrix;

    //Calcul des paramètres nécessaires
    Pyramid * pyr = L->getDataPyramid();

    //on regarde si le level demandé est spécifique ou pas
    if (L->getDataPyramid()->getSPyramids().size() !=0) {
        std::map<std::string,std::vector<Pyramid*> > spyr = L->getDataPyramid()->getSPyramids();
        std::map<std::string,std::vector<Pyramid*> >::iterator sp = spyr.find(level);
        if (sp != spyr.end()) {
            specific = true;
        }
    }

    if (!specific) {
      bPyr = L->getDataPyramid()->getBPyramids();
    } else {
        bPyr = L->getDataPyramid()->getSPyramids().find(level)->second;
    }

    CRS dst_crs = pyr->getTms().getCrs();
    error = 0;


    Interpolation::KernelType interpolation = L->getResampling();

    //bbox
    //Récupération du TileMatrix demandé
    std::map<std::string, Level*>::iterator lv = pyr->getLevels().find(level);

    if (lv == pyr->getLevels().end()) {
        return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( pyr->getLowestLevel()->getEncodedNoDataTile() ) );
    } else {

        TileMatrix tm = lv->second->getTm();

        //Récupération des paramètres associés
        resolution = tm.getRes();
        xo = tm.getX0();
        yo = tm.getY0();
        tileH = tm.getTileH();
        tileW = tm.getTileW();
        width = tileW;
        height = tileH;

        Row = double(tileRow);
        Col = double(tileCol);

        if (tileRow >= lv->second->getMinTileRow() && tileRow <= lv->second->getMaxTileRow()
                && tileCol >= lv->second->getMinTileCol() && tileCol <= lv->second->getMaxTileCol()) {

            //calcul de la bbox
            xmin = Col * double(tileW) * resolution + xo;
            ymax = yo - Row * double(tileH) * resolution;
            xmax = xmin + double(tileW) * resolution;
            ymin = ymax - double(tileH) * resolution;

            BoundingBox<double> bbox(xmin,ymin,xmax,ymax) ;

            //Récupérationd du tableau à double entrée représentant les associations de levels
            std::map<std::string, std::map<std::string, std::string> > aLevels;
            if (!specific) {
                aLevels = pyr->getALevel();
            }



            //pour chaque pyramide de base, on récupère une image
            for (int i = 0; i < bPyr.size(); i++) {


                pyrType = bPyr.at(i)->getFormat();
                bStyle = bPyr.at(i)->getStyle();

                if (specific) {
                    bLevel = bPyr.at(i)->getLevels().begin()->second->getId();
                } else {
                    //on récupère le bLevel associé à level
                    std::ostringstream oss;
                    oss << i;
                    std::map<std::string,std::string> aLevel = aLevels.find(level)->second;
                    bLevel = aLevel.find(oss.str())->second;
                }


                //on transforme la bbox
                BoundingBox<double> motherBbox = bbox;
                BoundingBox<double> childBBox = bPyr.at(i)->getTms().getCrs().getCrsDefinitionArea();
                if (motherBbox.reproject(pyr->getTms().getCrs().getProj4Code(),bPyr.at(i)->getTms().getCrs().getProj4Code()) ==0 &&
                childBBox.reproject("epsg:4326",bPyr.at(i)->getTms().getCrs().getProj4Code()) == 0) {
                    //on récupère l'image si on a pu reprojeter les bbox
                    //  cela a un double objectif:
                    //      on peut voir s'il y a de la donnée
                    //      et si on ne peut pas reprojeter,on ne pourra pas le faire plus tard non plus
                    //          donc il sera impossible de créer une image

                    if (childBBox.containsInside(motherBbox) || motherBbox.containsInside(childBBox) || motherBbox.intersects(childBBox)) {

                        curImage = bPyr.at(i)->createReprojectedImage(bLevel, bbox, dst_crs, servicesConf, width, height, interpolation, error);

                        if (curImage != NULL) {
                            //On applique un style à l'image
                            image = styleImage(curImage, pyrType, bStyle, format, bPyr.size());
                            images.push_back ( image );
                        } else {
                            LOGGER_ERROR("Impossible de générer la tuile car l'une des basedPyramid du layer "+L->getTitle()+" ne renvoit pas de tuile");
                            return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
                        }

                    } else {

                        LOGGER_DEBUG("Incohérence des bbox: Impossible de générer une tuile issue d'une basedPyramid du layer "+L->getTitle());

                    }

                } else {
                    //on n'a pas pu reprojeter les bbox

                    LOGGER_DEBUG("Reprojection impossible: Impossible de générer une tuile issue d'une basedPyramid du layer "+L->getTitle());

                }


            }

            pyrType = bPyr.at ( 0 )->getFormat();


            //On merge les images récupérés dans chacune des basedPyramid
            if (images.size() != 0) {

                mergeImage = mergeImages(images, pyrType, style, dst_crs, bbox);

                if (mergeImage == NULL) {
                    LOGGER_ERROR("Impossible de générer la tuile car l'opération de merge n'a pas fonctionné");
                    return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
                }

            } else {

                LOGGER_ERROR("Aucune image n'a été récupérée");
                return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( pyr->getLowestLevel()->getEncodedNoDataTile() ) );

            }


        } else {
            return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( pyr->getLowestLevel()->getEncodedNoDataTile() ) );
        }

    }



    //De cette image mergée, on lui applique un format pour la renvoyer au client
    DataStream *tileSource = formatImage(mergeImage, format, pyrType, format_option, bPyr.size(), style);
    DataSource *tile;

    if (tileSource == NULL) {
        LOGGER_ERROR("Impossible de générer la tuile car l'opération de formattage n'a pas fonctionné");
        return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
    } else {
        tile = new BufferedDataSource(*tileSource);
        delete tileSource;

    }

    return tile;

}

DataSource *Rok4Server::getTileOnFly(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format, DataSource *errorResp) {
    //On va créer la tuile sur demande et stocker la dalle qui la contient

    //variables
    std::string Spath, SpathTmp, SpathErr;
    DataSource *tile;
    Pyramid * pyr = L->getDataPyramid();
    struct stat bufferS;
    struct stat bufferT;
    struct stat bufferE;

    //on récupère l'emplacement théorique de la dalle
    std::map<std::string, Level*>::iterator lv = pyr->getLevels().find(tileMatrix);
    if (lv == pyr->getLevels().end()) {
        return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( pyr->getLowestLevel()->getEncodedNoDataTile() ) );
    } else {

        Spath = lv->second->getFilePath(tileCol,tileRow);
        SpathTmp = Spath + ".tmp";
        SpathErr = Spath + ".err";

        if (stat (Spath.c_str(), &bufferS) == 0 && stat (SpathTmp.c_str(), &bufferT) == -1) {
            //la dalle existe donc on fait une requete normale
            tile = getTileUsual(L, format, tileCol, tileRow, tileMatrix, errorResp, style);
        } else {
            //la dalle n'existe pas

            if (stat (SpathTmp.c_str(), &bufferT) == 0 || stat (SpathErr.c_str(), &bufferE) == 0) {
                //la dalle est en cours de creation ou on a deja essaye de la creer et ça n'a pas marché
                //donc on a un fichier d'erreur
                tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);

            } else {

                //la dalle n'est pas en cours de creation
                //on cree un processus qui va creer la dalle en parallele
                if (parallelProcess->createProcess()) {

                    //---------------------------------------------------------------------------------------------------

                    if (parallelProcess->getLastPid() == 0) {
                        //PROCESSUS FILS
                        // on va créer un fichier tmp, générer la dalle et supprimer le fichier tmp

                        //on cree un fichier temporaire pour indiquer que la dalle va etre creer
                        int fileTmp = open(SpathTmp.c_str(),O_CREAT|O_EXCL);
                        if (fileTmp != -1) {
                            //on a pu creer un fichier temporaire
                            close(fileTmp);
                        } else {
                            //impossible de creer un fichier temporaire
                            exit(0);
                        }

                        //on cree un logger qui ne rapporte que les erreurs
                        // il sera écrit dans SpathErr et supprimé si la dalle a été généré
                        LoggerSpecific *logErr = new LoggerSpecific(STATIC_FILE_SYNC,ERROR_SYNC,SpathErr,std::cerr);

                        //on cree la dalle
                        int state = createSlabOnFly(L, tileMatrix, tileCol, tileRow, style, format, Spath, logErr);
                        if (!state) {
                            //la generation s'est bien déroulé
                            //on supprime le logger qui ne contient en théorie pas ou peu
                            //d'erreurs. Du moins, aucune ayant empéchée la génération
                            int fileErr = remove(SpathErr.c_str());
                            if (fileErr != 0) {
                                //Impossible de supprimer le fichier erreur
                            }
                        }

                        //on nettoie
                        fileTmp = remove(SpathTmp.c_str());
                        if (fileTmp != 0) {
                            //Impossible de supprimer le fichier temporaire
                        }
                        delete logErr;

                        //on arrete le processus
                        exit(0);

                    } else {
                        //PROCESSUS PERE
                        //on va répondre a la requête
                        tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
                    }

                    //-------------------------------------------------------------------------------------------------


                } else {
                    LOGGER_WARN("Impossible de créer un processus parallele donc pas de génération de dalle");
                    tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
                }

            }

        }

    }

    return tile;

}

int Rok4Server::createSlabOnFly(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format, std::string path, LoggerSpecific *logErr) {

    //Variables utilisees
    std::vector<Image*> images;
    Image *curImage;
    Image *image;
    Image *mergeImage;
    std::string bLevel;
    double Col, Row, xmin, ymin, xmax, ymax, xo, yo, resolution;
    int tileH, tileW, width, height, error, TilePerWidth, TilePerHeight;
    Rok4Format::eformat_data pyrType;
    Style * bStyle;
    std::vector<Pyramid*> bPyr;
    bool specific = false;

    int state = 0;

    //On cree la dalle sous forme d'image

    //la correspondance est assurée par les vérifications qui ont eu lieu dans getTile()
    std::string level = tileMatrix;

    //Calcul des paramètres nécessaires
    Pyramid * pyr = L->getDataPyramid();

    //on regarde si le level demandé est spécifique ou pas
    if (L->getDataPyramid()->getSPyramids().size() !=0) {
        std::map<std::string,std::vector<Pyramid*> > spyr = L->getDataPyramid()->getSPyramids();
        std::map<std::string,std::vector<Pyramid*> >::iterator sp = spyr.find(level);
        if (sp != spyr.end()) {
            specific = true;
        }
    }

    if (!specific) {
      bPyr = L->getDataPyramid()->getBPyramids();
    } else {
        bPyr = L->getDataPyramid()->getSPyramids().find(level)->second;
    }

    CRS dst_crs = pyr->getTms().getCrs();
    error = 0;


    Interpolation::KernelType interpolation = L->getResampling();

    //bbox
    //Récupération du TileMatrix demandé
    std::map<std::string, Level*>::iterator lv = pyr->getLevels().find(level);
    TileMatrix tm = lv->second->getTm();

    //Récupération des paramètres associés
    resolution = tm.getRes();
    xo = tm.getX0();
    yo = tm.getY0();
    tileH = tm.getTileH();
    tileW = tm.getTileW();
    TilePerWidth = lv->second->getTilesPerWidth();
    TilePerHeight = lv->second->getTilesPerHeight();

    //width and height of a slab, not a tile
    width = tileW * TilePerWidth;
    height = tileH * TilePerHeight;

    Row = floor(double(tileRow) / TilePerHeight ) * TilePerHeight;
    Col = floor(double(tileCol) / TilePerWidth) * TilePerWidth;


    //calcul de la bbox de la dalle et non de la tuile
    xmin = Col * double(tileW) * resolution  * TilePerWidth + xo;
    ymax = yo - Row * double(tileH) * resolution * TilePerHeight;
    xmax = xmin + double(tileW) * resolution * TilePerWidth;
    ymin = ymax - double(tileH) * resolution * TilePerHeight;

    if (pyr->getTms().getCrs().getMetersPerUnit() != 1) {
        xmin = (int)xmin % 360;
        xmax = (int)xmax % 360;
        ymin = (int)ymin % 360;
        ymax = (int)ymax % 360;
    }

    BoundingBox<double> bbox(xmin,ymin,xmax,ymax) ;

    //Récupérationd du tableau à double entrée représentant les associations de levels
    std::map<std::string, std::map<std::string, std::string> > aLevels;
    if (!specific) {
        aLevels = pyr->getALevel();
    }

    //pour chaque pyramide de base, on récupère une image
    for (int i = 0; i < bPyr.size(); i++) {

        pyrType = bPyr.at(i)->getFormat();
        bStyle = bPyr.at(i)->getStyle();

        if (specific) {
            bLevel = bPyr.at(i)->getLevels().begin()->second->getId();
        } else {
            //on récupère le bLevel associé à level
            std::ostringstream oss;
            oss << i;
            std::map<std::string,std::string> aLevel = aLevels.find(level)->second;
            bLevel = aLevel.find(oss.str())->second;
        }

        curImage = bPyr.at(i)->createReprojectedImage(bLevel, bbox, dst_crs, servicesConf, width, height, interpolation, error);

        if (curImage != NULL) {
            //On applique un style à l'image
            image = styleImage(curImage, pyrType, bStyle, format, bPyr.size());
            images.push_back ( image );
        } else {
            //LOGGER_ERROR("Impossible de générer la tuile car l'une des basedPyramid du layer "+L->getTitle()+" ne renvoit pas de tuile");
            //return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
        }

    }

    pyrType = bPyr.at ( 0 )->getFormat();


    //On merge les images récupérés dans chacune des basedPyramid
    if (images.size() != 0) {

        mergeImage = mergeImages(images, pyrType, style, dst_crs, bbox);

        if (mergeImage == NULL) {
//            LOGGER_ERROR("Impossible de générer la tuile car l'opération de merge n'a pas fonctionné");
//            return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
        }

    } else {

//        LOGGER_ERROR("Aucune image n'a été récupérée");
//        return new DataSourceProxy ( new FileDataSource ( "",0,0,"" ), * ( pyr->getLowestLevel()->getEncodedNoDataTile() ) );

    }


    //on transforme la dalle en image Rok4 que l'on stocke

    Rok4ImageFactory R4IF;
    //à cause d'un problème de typage...
    char * pathToWrite = (char *)path.c_str();

    Rok4Image * finalImage = R4IF.createRok4ImageToWrite(pathToWrite,bbox,mergeImage->getResX(),mergeImage->getResY(),
                                                         mergeImage->getWidth(),mergeImage->getHeight(),pyr->getChannels(),
                                                         pyr->getSampleFormat(),pyr->getBitsPerSample(),
                                                         pyr->getPhotometry(),pyr->getSampleCompression(),tm.getTileW(),
                                                         tm.getTileH());

    if (finalImage != NULL) {
        //error("Cannot create the ROK4 image to write", -1);
        //LOGGER_DEBUG ( "Write" );
        if (finalImage->writeImage(mergeImage) < 0) {
            //error("Cannot write ROK4 image", -1);
        }
    }

    return state;

}

void Rok4Server::processWMTS ( Request* request, FCGX_Request&  fcgxRequest ) {
    if ( request->request == "getcapabilities" ) {
        S.sendresponse ( WMTSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == "gettile" ) {
        S.sendresponse ( getTile ( request ),&fcgxRequest );
    } else if ( request->request == "getversion" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->request+_ ( " n'est pas prise en charge par ce serveur." ) + ROK4_INFO,"wmts" ) ),&fcgxRequest );
    } else if ( request->request == "" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre REQUEST n'est pas renseigne." ) ,"wmts" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,_ ( "L'operation " ) +request->request+_ ( " n'est pas prise en charge par ce serveur." ),"wmts" ) ),&fcgxRequest );
    }
}

void Rok4Server::processWMS ( Request* request, FCGX_Request&  fcgxRequest ) {
    //le capabilities est présent pour une compatibilité avec le WMS 1.1.1
    if ( request->request == "getcapabilities" || request->request == "capabilities") {
        S.sendresponse ( WMSGetCapabilities ( request ),&fcgxRequest );
        //le map est présent pour une compatibilité avec le WMS 1.1.1
    } else if ( request->request == "getmap" || request->request == "map") {
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
        //le map est présent pour une compatibilité avec le WMS 1.1.1
    } else if ( supportWMS && ( request->service=="wms" || request->request == "getmap" || request->request == "map") ) {
        processWMS ( request, fcgxRequest );
    } else if ( request->service == "" ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre SERVICE n'est pas renseigne." ) ,"xxx" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le service " ) +request->service+_ ( " est inconnu pour ce serveur." ),"wmts" ) ),&fcgxRequest );
    }
}


