/*
 * Copyright © (2011-2013) Institut national de l'information
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
#include "curl/curl.h"
#include "WebService.h"
#include <cmath>
#include <errno.h>
#include <time.h>
#include "config.h"
#include "intl.h"
#include "TiffEncoder.h"
#include "CurlPool.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"
#include "BilEncoder.h"
#include "AscEncoder.h"
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
#include "EmptyImage.h"
#include "FileContext.h"
#include "PenteImage.h"
#include "Pente.h"
#include "AspectImage.h"
#include "Aspect.h"
#include "ConvertedChannelsImage.h"
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::second

void hangleSIGALARM(int id) {
    if(id==SIGALRM) {
         exit(0) ; /* exit on receiving SIGALRM signal */
    }
    signal(SIGALRM, hangleSIGALARM) ;
}

void* Rok4Server::thread_loop ( void* arg ) {
    Rok4Server* server = ( Rok4Server* ) ( arg );
    FCGX_Request fcgxRequest;
    if ( FCGX_InitRequest ( &fcgxRequest, server->sock, FCGI_FAIL_ACCEPT_ON_INTR ) != 0 ) {
        LOGGER_FATAL ( _ ( "Le listener FCGI ne peut etre initialise" ) );
    }

    while ( server->isRunning() ) {
        std::string content;

        int rc;
        if ( ( rc=FCGX_Accept_r ( &fcgxRequest ) ) < 0 ) {
            if ( rc == -4 ) { // Cas du redémarrage
                LOGGER_DEBUG ( _ ( "Redémarrage : FCGX_InitRequest renvoie le code d'erreur " ) << rc );
            } else {
                LOGGER_ERROR ( _ ( "FCGX_InitRequest renvoie le code d'erreur " ) << rc );
                std::cerr <<"FCGX_InitRequest renvoie le code d'erreur " << rc << std::endl;
            }
            
            break;
        }

        LOGGER_DEBUG("Thread " << pthread_self() << " traite une requete");

        bool postRequest = false;
        if (server->servicesConf->isPostEnabled() && strcmp ( FCGX_GetParam ( "REQUEST_METHOD",fcgxRequest.envp ),"POST" ) == 0) {
            postRequest = true;
        }

        Request* request;
        if ( postRequest ) { // Post Request
            char* contentBuffer = ( char* ) malloc ( sizeof ( char ) *200 );
            while ( FCGX_GetLine ( contentBuffer,200,fcgxRequest.in ) ) {
                content.append ( contentBuffer );
            }
            free ( contentBuffer );
            contentBuffer= NULL;
            LOGGER_DEBUG ( _ ( "Request Content :" ) << std::endl << content );
            request = new Request (
                FCGX_GetParam ( "QUERY_STRING", fcgxRequest.envp ),
                FCGX_GetParam ( "HTTP_HOST", fcgxRequest.envp ),
                FCGX_GetParam ( "SCRIPT_NAME", fcgxRequest.envp ),
                FCGX_GetParam ( "HTTPS", fcgxRequest.envp ),
                content
            );
        } else { // Get Request

            /* On espère récupérer le nom du host tel qu'il est exprimé dans la requete avec HTTP_HOST.
             * De même, on espère récupérer le path tel qu'exprimé dans la requête avec SCRIPT_NAME.
             */

            request = new Request ( 
                FCGX_GetParam ( "QUERY_STRING", fcgxRequest.envp ),
                FCGX_GetParam ( "HTTP_HOST", fcgxRequest.envp ),
                FCGX_GetParam ( "SCRIPT_NAME", fcgxRequest.envp ),
                FCGX_GetParam ( "HTTPS", fcgxRequest.envp )
            );
        }

        server->processRequest ( request, fcgxRequest );
        delete request;

        FCGX_Finish_r ( &fcgxRequest );
        FCGX_Free ( &fcgxRequest,1 );

        LOGGER_DEBUG("Thread " << pthread_self() << " en a fini avec la requete");

        server->parallelProcess->checkCurrentPid();

    }

    LOGGER_DEBUG ( _ ( "Extinction du thread" ) );
    Logger::stopLogger();
    return 0;
}

Rok4Server::Rok4Server (  ServerXML* serverXML, ServicesXML* servicesXML) {
    

    sock = 0;
    servicesConf = servicesXML;
    serverConf = serverXML;

    threads = std::vector<pthread_t>(serverConf->getNbThreads());

    running = false;

    if ( serverConf->supportWMS ) {
        LOGGER_DEBUG ( _ ( "Build WMS Capabilities 1.3.0" ) );
        buildWMS130Capabilities();
        //---- WMS 1.1.1
        LOGGER_DEBUG ( _ ( "Build WMS Capabilities 1.1.1" ) );
        buildWMS111Capabilities();
        //----
    }
    if ( serverConf->supportWMTS ) {
        LOGGER_DEBUG ( _ ( "Build WMTS Capabilities" ) );
        buildWMTSCapabilities();
    }
    if ( serverConf->supportTMS ) {
        LOGGER_DEBUG ( _ ( "Build TMS Capabilities" ) );
        buildTMSCapabilities();
    }
    //initialize processFactory
    if (serverConf->nbProcess > MAX_NB_PROCESS) {
        serverConf->nbProcess = MAX_NB_PROCESS;
    }
    if (serverConf->nbProcess < 0) {
        serverConf->nbProcess = DEFAULT_NB_PROCESS;
    }
    parallelProcess = new ProcessFactory(serverConf->nbProcess, "", serverConf->timeKill);
}

Rok4Server::~Rok4Server() {

    delete serverConf;
    delete servicesConf;

    delete parallelProcess;
    parallelProcess = NULL;
}

void Rok4Server::initFCGI() {
    int init=FCGX_Init();
    if ( ! serverConf->socket.empty() ) {
        LOGGER_INFO ( _ ( "Listening on " ) << serverConf->socket );
        sock = FCGX_OpenSocket ( serverConf->socket.c_str(), serverConf->backlog );
    }
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

    // Terminate FCGI Thread
    for ( int i = 0; i < threads.size(); i++ ) {
        pthread_kill ( threads[i], SIGQUIT );
    }

    CurlPool::cleanCurlPool();
}


DataStream* Rok4Server::getMap ( Request* request ) {
    std::vector<Layer*> layers;
    BoundingBox<double> bbox ( 0.0, 0.0, 0.0, 0.0 );
    int width, height, dpi;
    CRS crs;
    std::string format;
    std::vector<Style*> styles;
    std::map <std::string, std::string > format_option;
    std::vector<Image*> images;


    // Récupération des paramètres
    DataStream* errorResp = getMapParamWMS ( request, layers, bbox, width, height, crs, format ,styles, format_option, dpi );
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getMap" ) );
        return errorResp;
    }

    int error;
    Image* image;
    for ( int i = 0 ; i < layers.size(); i ++ ) {

            Image* curImage = layers.at ( i )->getbbox ( servicesConf, bbox, width, height, crs, dpi, error );

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

            curImage->setBbox(bbox);
            curImage->setCRS(crs);
            Rok4Format::eformat_data pyrType = layers.at ( i )->getDataPyramid()->getFormat();
            Style* style = styles.at(i);
            LOGGER_DEBUG ( _ ( "GetMap de Style : " ) << styles.at ( i )->getId() << _ ( " pal size : " ) <<styles.at ( i )->getPalette()->getPalettePNGSize() );


            Image *image = styleImage(curImage, pyrType, style, format, layers.size(), layers.at(i)->getDataPyramid());

            if (image == 0) {
                return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wms" ) );
            }

            images.push_back ( image );
    }


    //Use background image format.
    Rok4Format::eformat_data pyrType = layers.at ( 0 )->getDataPyramid()->getFormat();
    Style* style = styles.at(0);

    image = mergeImages(images, pyrType, style, crs, bbox);

    DataStream * stream = formatImage(image, format, pyrType, format_option, layers.size(), style);

    return stream;
}

Image *Rok4Server::styleImage(Image *curImage, Rok4Format::eformat_data pyrType, Style *style, std::string format, int size, Pyramid* pyr) {

    Image * expandedImage = curImage;

    if ( servicesConf->isFullStyleCapable() ) {
        if ( style->isEstompage() ) {
            LOGGER_DEBUG ( _ ( "Estompage" ) );

            int error=0;
            BoundingBox<double> expandedBbox = curImage->getBbox().expand(curImage->getResX(),curImage->getResY(),1);
            expandedImage = pyr->getbbox(servicesConf,expandedBbox,curImage->getWidth()+2,curImage->getHeight()+2,curImage->getCRS(),style->getInterpolationOfEstompage(),0,error);

            if (expandedImage == 0) {
                LOGGER_ERROR("expanded Image is NULL");
                delete curImage;
                return NULL;
            }

            expandedImage->setBbox(expandedBbox);
            expandedImage->setCRS(curImage->getCRS());

            expandedImage = new EstompageImage ( curImage->getWidth(), curImage->getHeight(), curImage->getChannels(),
                                                 curImage->getBbox(), expandedImage, style->getZenith(), style->getAzimuth(),
                                                 style->getZFactor(), expandedImage->getResXmeter(),expandedImage->getResYmeter() );
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

            delete curImage;
        }

        if (expandedImage->getChannels() == 1 && style->isPente()){

            int error=0;
            BoundingBox<double> expandedBbox = curImage->getBbox().expand(curImage->getResX(),curImage->getResY(),1);
            expandedImage = pyr->getbbox(servicesConf,expandedBbox,curImage->getWidth()+2,curImage->getHeight()+2,curImage->getCRS(),style->getInterpolationOfPente(),0,error);

            if (expandedImage == 0) {
                LOGGER_ERROR("expanded Image is NULL");
                delete curImage;
                return NULL;
            }

            expandedImage->setBbox(expandedBbox);
            expandedImage->setCRS(curImage->getCRS());

			if ( format == "image/png" && size == 1 ) {
				switch ( pyrType ) {
                case Rok4Format::TIFF_RAW_FLOAT32 :
                case Rok4Format::TIFF_ZIP_FLOAT32 :
                case Rok4Format::TIFF_LZW_FLOAT32 :
                case Rok4Format::TIFF_PKB_FLOAT32 :
                    expandedImage = new PenteImage ( curImage->getWidth(), curImage->getHeight(), curImage->getChannels(),
                                                                         curImage->getBbox(),expandedImage, expandedImage->getResXmeter(),
                                                                         expandedImage->getResYmeter(), style->getAlgoOfPente(), style->getUnitOfPente(),
                                                                         style->getSlopeNoDataOfPente(),pyr->getFirstNdValues(),
                                                                         style->getMaxSlopeOfPente());
                default:
					break;
				}
			} else {
                expandedImage = new PenteImage ( curImage->getWidth(), curImage->getHeight(), curImage->getChannels(),
                                                                     curImage->getBbox(),expandedImage, expandedImage->getResXmeter(),
                                                                     expandedImage->getResYmeter(), style->getAlgoOfPente(), style->getUnitOfPente(),
                                                                     style->getSlopeNoDataOfPente(),pyr->getFirstNdValues(),
                                                                     style->getMaxSlopeOfPente());
            }
            delete curImage;
		}

        if (expandedImage->getChannels() == 1 && style->isAspect()){

            int error=0;
            BoundingBox<double> expandedBbox = curImage->getBbox().expand(curImage->getResX(),curImage->getResY(),1);
            expandedImage = pyr->getbbox(servicesConf,expandedBbox,curImage->getWidth()+2,curImage->getHeight()+2,curImage->getCRS(),Interpolation::LINEAR,0,error);

            if (expandedImage == 0) {
                LOGGER_ERROR("expanded Image is NULL");
                delete curImage;
                return NULL;
            }

            expandedImage->setBbox(expandedBbox);
            expandedImage->setCRS(curImage->getCRS());

            if ( format == "image/png" && size == 1 ) {
                switch ( pyrType ) {
                case Rok4Format::TIFF_RAW_FLOAT32 :
                case Rok4Format::TIFF_ZIP_FLOAT32 :
                case Rok4Format::TIFF_LZW_FLOAT32 :
                case Rok4Format::TIFF_PKB_FLOAT32 :
                    expandedImage = new AspectImage ( curImage->getWidth(), curImage->getHeight(), curImage->getChannels(), curImage->getBbox(),expandedImage, expandedImage->computeMeanResolution(),  style->getAlgoOfAspect(), style->getMinSlopeOfAspect());
                default:
                    break;
                }
            } else {
                expandedImage = new AspectImage ( curImage->getWidth(), curImage->getHeight(), curImage->getChannels(), curImage->getBbox(),expandedImage, expandedImage->computeMeanResolution(),  style->getAlgoOfAspect(), style->getMinSlopeOfAspect());
            }
            delete curImage;
        }

        if ( style && expandedImage->getChannels() == 1 && ! ( style->getPalette()->getColoursMap()->empty() ) ) {
            if ( format == "image/png" && size == 1 ) {
                switch ( pyrType ) {

                case Rok4Format::TIFF_RAW_FLOAT32 :
                case Rok4Format::TIFF_ZIP_FLOAT32 :
                case Rok4Format::TIFF_LZW_FLOAT32 :
                case Rok4Format::TIFF_PKB_FLOAT32 :
                    expandedImage = new StyledImage ( expandedImage, style->getPalette()->isNoAlpha()?3:4 , style->getPalette() );
                default:
                    break;
                }
            } else {
                expandedImage = new StyledImage ( expandedImage, style->getPalette()->isNoAlpha()?3:4, style->getPalette() );
            }
        }

    }

    return expandedImage;

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
        int spp = images.at ( 0 )->getChannels();
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
    } else if ( format == "text/asc" ) {
        // On ne traite le format asc que sur les image à un seul channel
        if (image->getChannels() != 1){
            LOGGER_ERROR ( "Le format "<<format<<" ne concerne que les images à 1 canal" );
        }else{
            return new AscEncoder ( image );
        }
    }

    LOGGER_ERROR ( "Le format "<<format<<" ne peut etre traite" );

    return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,_ ( "Le format " ) +format+_ ( " ne peut etre traite" ),"wms" ) );

}

DataSource* Rok4Server::getTile ( Request* request ) {
    Layer* L;
    std::string tileMatrix,format;
    int tileCol,tileRow;
    Style* style=0;

    // Récupération des parametres de la requete
    DataSource* errorResp;
    if (request->service == ServiceType::WMTS) {
        errorResp = getTileParamWMTS ( request, L, tileMatrix, tileCol, tileRow, format, style );
    } else {
        // TMS
        errorResp = getTileParamTMS ( request, L, tileMatrix, tileCol, tileRow, format, style );
    }

    if ( errorResp ) {
        return errorResp;
    }
    errorResp = NULL;

    Level* level = L->getDataPyramid()->getLevel(tileMatrix);
    if (level == NULL) {
        // On est hors niveau -> erreur
        return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND, _ ( "No data found" ), "wmts" ) );
    }

    DataSource* tileSource;

    if (tileRow < level->getMinTileRow() || tileRow > level->getMaxTileRow()
            || tileCol < level->getMinTileCol() || tileCol > level->getMaxTileCol()) {
        // On est hors tuiles -> erreur
        return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND, _ ( "No data found" ), "wmts" ) );
    }

    if (level->isOnFly()) {
        tileSource = getTileOnFly(L, tileMatrix, tileCol, tileRow, style, format);
    }
    else if (level->isOnDemand()) {
        tileSource = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
    }
    else {
        tileSource = getTileUsual(L, tileMatrix, tileCol, tileRow, style, format) ;
    }

    return tileSource;

}



DataSource *Rok4Server::getTileUsual(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style,std::string format) {

    DataSource* tileSource;
    // Avoid using unnecessary palette
    if ( format == "image/png" ) {
        tileSource = new PaletteDataSource ( L->getDataPyramid()->getLevel(tileMatrix)->getTile ( tileCol, tileRow ), style->getPalette() );
    } else {
        tileSource = L->getDataPyramid()->getLevel(tileMatrix)->getTile ( tileCol, tileRow );
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
    int width, height, error;
    Rok4Format::eformat_data pyrType = Rok4Format::UNKNOWN;
    Style * bStyle;
    std::map <std::string, std::string > format_option;
    int bSize = 0;
    std::vector <Source*> bSources;

    LOGGER_INFO("GetTileOnDemand");

    //Calcul des paramètres nécessaires
    LOGGER_DEBUG("Compute parameters");
    Pyramid * pyr = L->getDataPyramid();
    CRS dst_crs = pyr->getTms()->getCrs();
    error = 0;
    Interpolation::KernelType interpolation = L->getResampling();
    Level* lev = pyr->getLevel(tileMatrix);

    //--------------------------------------------------------------------------------------------------------
    //Suite du calcul des paramètres nécessaires
    //calcul de la bbox
    LOGGER_DEBUG("Compute BBOX");
    BoundingBox<double> bbox = lev->tileIndicesToTileBbox(tileCol,tileRow) ;
    bbox.print();
    //width and height and channels
    width = lev->getTm()->getTileW();
    height = lev->getTm()->getTileH();

    //--------------------------------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------------------------------
    //CREATION DE L'IMAGE
    LOGGER_DEBUG("Create Image");

    bSources = lev->getSources();
    bSize = bSources.size();

    for(int i = bSize-1; i >= 0; i-- ) {

        eSourceType type = bSources.at(i)->getType();

        if (type == PYRAMID) {

            //----on recupère la Pyramide Source
            Pyramid* bPyr = reinterpret_cast<Pyramid*>(bSources.at(i));

            pyrType = bPyr->getFormat();
            bStyle = bPyr->getStyle();
            bLevel = bPyr->getUniqueLevel()->getId();

            //on transforme la bbox
            BoundingBox<double> motherBbox = bbox;
            BoundingBox<double> childBBox = bPyr->getTms()->getCrs().getCrsDefinitionArea();
            if (motherBbox.reproject(pyr->getTms()->getCrs().getProj4Code(),bPyr->getTms()->getCrs().getProj4Code()) ==0 &&
            childBBox.reproject("epsg:4326",bPyr->getTms()->getCrs().getProj4Code()) == 0) {
                // on récupère l'image si on a pu reprojeter les bbox. Cela a un double objectif : on peut voir
                // s'il y a de la donnée et si on ne peut pas reprojeter,on ne pourra pas le faire plus tard non
                // plus donc il sera impossible de créer une image

                if (childBBox.containsInside(motherBbox) || motherBbox.containsInside(childBBox) || motherBbox.intersects(childBBox)) {

                    curImage = bPyr->createReprojectedImage(bLevel, bbox, dst_crs, servicesConf, width, height, interpolation, error);

                    if (curImage != NULL) {
                        //On applique un style à l'image
                        image = styleImage(curImage, pyrType, bStyle, format, bSize, bPyr);
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

        } else {

            if (type == WEBSERVICE) {

                //----on recupère le WebService Source
                WebMapService *wms = reinterpret_cast<WebMapService*>(bSources.at(i));

                //----traitement de la requete
                image = wms->createImageFromRequest(width,height,bbox);

                if (image) {
                    images.push_back(image);
                } else {
                    LOGGER_ERROR("Impossible de generer la tuile car l'un des WebServices du layer "+L->getTitle()+" ne renvoit pas de tuile");
                    return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
                }

            }

        }

    }


    //On merge les images récupérés dans chacune des basedPyramid ou/et des WebServices
    if (images.size() != 0) {
        mergeImage = mergeImages(images, pyrType, style, dst_crs, bbox);

        if (mergeImage == NULL) {
            LOGGER_ERROR("Impossible de générer la tuile car l'opération de merge n'a pas fonctionné");
            return new SERDataSource( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ),"wmts" ) );
        }

    } else {
        LOGGER_ERROR("Aucune image n'a été récupérée");
        return new SERDataSource ( new ServiceException ( "", HTTP_NOT_FOUND, _ ( "No data found" ), "wmts" ) );
    }



    //De cette image mergée, on lui applique un format pour la renvoyer au client
    DataStream *tileSource = formatImage(mergeImage, format, pyrType, format_option, bSize, style);
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

DataSource *Rok4Server::getTileOnFly(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format) {
    //On va créer la tuile sur demande et stocker la dalle qui la contient

    //variables
    std::string Spath, SpathTmp, SpathErr, SpathDir;
    DataSource *tile;
    Pyramid * pyr = L->getDataPyramid();
    struct stat bufferS;
    struct stat bufferT;
    struct stat bufferE;

    LOGGER_INFO("GetTileOnFly");

    //---- on verifie certains paramètres pour ne pas effectuer des calculs inutiles
    Level* lev = pyr->getLevel(tileMatrix);

    Spath = lev->getPath(tileCol, tileRow);
    SpathDir = lev->getDirPath(tileCol,tileRow);
    SpathTmp = Spath + ".tmp";
    SpathErr = Spath + ".err";

    if (stat (Spath.c_str(), &bufferS) == 0) {
        //la dalle existe ou est en cours de création
        if (stat (SpathTmp.c_str(), &bufferT) == -1) {
            // il n'y a pas de tmp donc la dalle existe mais une erreur peut subsister
            if (stat (SpathErr.c_str(), &bufferE) == -1) {
                // il n'y a pas d'erreurs donc la dalle existe
                //Usual
                tile = getTileUsual(L, tileMatrix, tileCol, tileRow, style, format);
            } else {
                //il y a une erreur
                //onDemand
                tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
            }
        } else {
            //il y a un fichier tmp donc la dalle est en cours de génération
            //onDemand
            tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
        }
    } else {
        //la dalle n'existe pas mais peut être en cours de création
        if (stat (SpathTmp.c_str(), &bufferT) == 0) {
            //la dalle est en cours de création
            //onDemand
            tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
        } else {
            //la dalle n'est pas en cours de création, il peut y avoir une erreur
            if (stat (SpathErr.c_str(), &bufferE) == 0) {
                //il y a une erreur
                //onDemand
                tile = getTileOnDemand(L, tileMatrix, tileCol, tileRow, style, format);
            } else {
                //il n'y a pas d'erreurs, on peut générer la dalle

                //on cree un processus qui va creer la dalle en parallele
                if (parallelProcess->createProcess()) {

                    //---------------------------------------------------------------------------------------------------

                    if (parallelProcess->getLastPid() == 0) {
                        //PROCESSUS FILS

                        // on va créer un fichier tmp, générer la dalle et supprimer le fichier tmp
                        //on met en place une alarme qui va eteindre le processus au bout d'un certain temps
                        signal(SIGALRM, hangleSIGALARM);
                        alarm(parallelProcess->getTimeBeforeAutoKill());

                        //on cree un fichier temporaire pour indiquer que la dalle va etre creer
                        //on commence par créer le dossier
                        int directory = lev->createDirPath(SpathDir.c_str());
                        if (directory == -1) {
                            if (errno != EEXIST) {
                                std::cerr << "Impossible de creer le dossier contenant la dalle " << SpathDir.c_str() << std::endl;
                                std::cerr << "errno: " << errno << " " << strerror(errno) << std::endl;
                                exit(0);
                            }
                        }

                        //on vient de creer le dossier donc on essaye de creer le fichier tmp
                        int fileTmp = open(SpathTmp.c_str(),O_CREAT|O_EXCL,S_IWRITE);
                        if (fileTmp != -1) {
                            //on a pu creer un fichier temporaire
                            close(fileTmp);
                        } else {
                            //impossible de creer le fichier tmp, il est potentiellement déjà existant
                            exit(0);
                        }

                        //on cree un logger et supprime l'ancien pour ne pas mélanger les sorties
                        // il sera écrit dans SpathErr et supprimé si la dalle a été généré correctement
                        parallelProcess->initializeLogger(SpathErr);

                        //on cree la dalle
                        int state = createSlabOnFly(L, tileMatrix, tileCol, tileRow, style, format, Spath);
                        if (!state) {
                            //la generation s'est bien déroulé
                            //on supprime le logger qui ne contient en théorie pas ou peu
                            //d'erreurs. Du moins, aucune ayant empéchée la génération

                            //vide du logger
                            sleep(1);

                            int fileErr = remove(SpathErr.c_str());
                            if (fileErr != 0) {
                                //Impossible de supprimer le fichier erreur
                                //std::cerr << "Impossible de supprimer le fichier de log .err " << SpathErr.c_str() << std::endl;
                                //std::cerr << "errno: " << errno << " " << strerror(errno) << std::endl;
                            }
                        } else {
                            //la generation n'a pas fonctionne
                            //on essaye de supprimer le fichier de dalle potentiellement existant
                            //mais contenant des erreurs

                            if (stat (Spath.c_str(), &bufferS) == 0) {
                                //le fichier existe mais il faut le supprimer
                                int file = remove(Spath.c_str());
                                if (file != 0) {
                                    //std::cerr << "Impossible de supprimer la dalle contenant des erreurs " << Spath.c_str() << std::endl;
                                    //std::cerr << "errno: " << errno << " " << strerror(errno) << std::endl;
                                }
                            }
                        }

                        //on nettoie
                        fileTmp = remove(SpathTmp.c_str());
                        if (fileTmp != 0) {
                            //Impossible de supprimer le fichier temporaire
                            //std::cerr << "Impossible de supprimer le fichier de temporaire " << SpathTmp.c_str() << std::endl;
                            //std::cerr << "errno: " << errno << " " << strerror(errno) << std::endl;
                        }

                        parallelProcess->destroyLogger();
                        //on arrete le processus
                        exit(0);

                    } else {
                        //PROCESSUS PERE
                        //on va répondre a la requête
                        LOGGER_DEBUG("Processus parallele lance ");
                        LOGGER_DEBUG("Création de la dalle "+Spath);
                        LOGGER_DEBUG("Log dans le fichier "+SpathErr);

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

int Rok4Server::createSlabOnFly(Layer* L, std::string tileMatrix, int tileCol, int tileRow, Style *style, std::string format, std::string path) {

    //Variables utilisees
    std::vector<Image*> images;
    Image *curImage;
    Image *image;
    Image *mergeImage;
    Image *lastImage;
    std::string bLevel;
    int width, height, error, tileH,tileW;
    Rok4Format::eformat_data pyrType = Rok4Format::UNKNOWN;
    Style * bStyle;
    std::vector<Source*> bSources;
    int state = 0;
    struct stat buffer;
    int bSize = 0;

    //On cree la dalle sous forme d'image
    LOGGER_INFO("Create Slab on Fly");
    //Calcul des paramètres nécessaires
    LOGGER_DEBUG("Compute parameters");
    //la correspondance est assurée par les vérifications qui ont eu lieu dans getTile()
    std::string level = tileMatrix;
    Pyramid * pyr = L->getDataPyramid();
    CRS dst_crs = pyr->getTms()->getCrs();
    error = 0;
    Interpolation::KernelType interpolation = L->getResampling();


    //---- on va créer la bbox associée à la dalle
    LOGGER_DEBUG("Compute BBOX");
    Level* lev = pyr->getLevel(tileMatrix);

    BoundingBox<double> bbox = lev->tileIndicesToSlabBbox(tileCol,tileRow) ;
    bbox.print();
    //---- bbox creee

    //width and height
    LOGGER_DEBUG("Compute width and height");
    width = lev->getSlabWidth();
    height = lev->getSlabHeight();
    tileW = lev->getTm()->getTileW();
    tileH = lev->getTm()->getTileH();


    //--------------------------------------------------------------------------------------------------------
    //CREATION DE L'IMAGE
    LOGGER_DEBUG("Create Image");

    bSources = lev->getSources();
    bSize = bSources.size();

    for(int i = bSize-1; i >= 0; i-- ) {

        eSourceType type = bSources.at(i)->getType();

        if (type == PYRAMID) {

            //----on recupère la Pyramide Source
            Pyramid *bPyr = reinterpret_cast<Pyramid*>(bSources.at(i));

            LOGGER_DEBUG("basedPyramid");
            pyrType = bPyr->getFormat();
            bStyle = bPyr->getStyle();
            bLevel = bPyr->getLevels().begin()->second->getId();

            LOGGER_DEBUG("Create reprojected image");
            curImage = bPyr->createBasedSlab(bLevel, bbox, dst_crs, servicesConf, width, height, interpolation, error);
            LOGGER_DEBUG("Created");
            if (curImage != NULL) {
                //On applique un style à l'image
                image = styleImage(curImage, pyrType, bStyle, format, bSize, bPyr);
                if (!image) {
                     LOGGER_ERROR("Impossible d'appliquer le style");
                     state = 1;
                     return state;
                } else {
                    LOGGER_DEBUG("Apply style");
                    images.push_back ( image );
                }
            } else {
                LOGGER_ERROR("Impossible de générer la dalle car l'une des basedPyramid du layer "+L->getTitle()+" ne renvoit pas de tuile");
                state = 1;
                delete bStyle;
                return state;
            }

            delete bStyle;

        } else {

            if (type == WEBSERVICE) {

                //----on recupère le WebService Source
                WebMapService *wms = reinterpret_cast<WebMapService*>(bSources.at(i));

                //----traitement de la requete
                image = wms->createSlabFromRequest(width,height,bbox);

                if (image) {
                    images.push_back(image);
                } else {
                    LOGGER_ERROR("Impossible de generer la tuile car l'un des WebServices du layer "+L->getTitle()+" ne renvoit pas de tuile");
                    state = 1;
                    return state;
                }

            }

        }

    }


    //On merge les images récupérés dans chacune des basedPyramid et chaque WebServices
    if (images.size() != 0) {

        mergeImage = mergeImages(images, pyrType, style, dst_crs, bbox);
        LOGGER_DEBUG("Merged differents basedImages");
        if (mergeImage == NULL) {
            LOGGER_ERROR("Impossible de générer la dalle car l'opération de merge n'a pas fonctionné");
            state = 1;
            return state;
        }

        if (images.size() == 1 && pyr->getChannels() != mergeImage->getChannels()) {
            lastImage = new ConvertedChannelsImage(pyr->getChannels(),mergeImage);
        } else {
            lastImage = mergeImage;
        }

    } else {
        LOGGER_ERROR("Impossible de générer la dalle car aucune image n'a été récupérée");
        state = 1;
        return state;
    }


    //IMAGE CREEE
    //-------------------------------------------------------------------------------------------------------

    //-------------------------------------------------------------------------------------------------------
    //ECRITURE DE L'IMAGE
    //on transforme la dalle en image Rok4 que l'on stocke

    Rok4ImageFactory R4IF;
    //à cause d'un problème de typage...
    char * pathToWrite = (char *)path.c_str();
    LOGGER_DEBUG("Create Rok4Image");

    FileContext* fc = new FileContext("");
    fc->connection();

    Rok4Image * finalImage = R4IF.createRok4ImageToWrite(
        pathToWrite,bbox,lastImage->getResX(),lastImage->getResY(),
        lastImage->getWidth(),lastImage->getHeight(),pyr->getChannels(),
        pyr->getSampleFormat(),pyr->getBitsPerSample(),
        pyr->getPhotometric(),pyr->getSampleCompression(),tileW, tileH, fc
    );

    LOGGER_DEBUG("Created");

    if (finalImage != NULL) {
        //LOGGER_DEBUG ( "Write" );
        LOGGER_DEBUG("Write Slab");
        if (finalImage->writeImage(lastImage) < 0) {
            LOGGER_ERROR("Impossible de générer la dalle car son écriture en mémoire a échoué");
            state = 1;
            delete lastImage;
            delete finalImage;
            return state;
        } else {
            LOGGER_DEBUG("Written");
        }
    } else {
        LOGGER_ERROR("Impossible de générer la dalle car la création d'une Rok4Image ne marche pas");
        state = 1;
        delete lastImage;
        delete finalImage;
        return state;
    }

    delete lastImage;
    delete finalImage;

    //IMAGE ECRITE
    //------------------------------------------------------------------------------------------------------

    return state;

}

DataStream* Rok4Server::WMSGetFeatureInfo ( Request* request ) {
    std::vector<Layer*> layers;
    std::vector<Layer*> query_layers;
    BoundingBox<double> bbox ( 0.0, 0.0, 0.0, 0.0 );
    int width, height;
    int X, Y;
    CRS crs;
    std::string format;
    std::string info_format;
    int feature_count = 1;
    std::vector<Style*> styles;
    std::map <std::string, std::string > format_option;
    //exception ?

    DataStream* errorResp = getFeatureInfoParamWMS (request, layers, query_layers, bbox, width, height, crs, format, styles, info_format, X, Y, feature_count, format_option);
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getFeatureInfo" ) );
        return errorResp;
    }
    return CommonGetFeatureInfo( "wms", query_layers.at(0), bbox, width, height, crs, info_format, X, Y, format, feature_count );

}


DataStream* Rok4Server::WMTSGetFeatureInfo ( Request* request ) {
    Layer* layer;
    std::string tileMatrix,format;
    int tileCol,tileRow;
    Style* style=0;
    int X, Y;
    std::string info_format;

    LOGGER_DEBUG("WMTSGetFeatureInfo");

    LOGGER_DEBUG("Verification des parametres de la requete");

    DataStream* errorResp = getFeatureInfoParamWMTS (request, layer, tileMatrix, tileCol, tileRow, format, style, info_format, X, Y);
    
    if ( errorResp ) {
        LOGGER_ERROR ( _ ( "Probleme dans les parametres de la requete getFeatureInfo" ) );
        return errorResp;
    }
    Pyramid* pyr = layer->getDataPyramid();

    // On a déjà vérifié dans getFeatureInfoParamWMTS que la pyramide possédait bien ce niveau
    Level* level = pyr->getLevel(tileMatrix);

    BoundingBox<double> bbox = level->tileIndicesToTileBbox(tileCol,tileRow) ;
    int height = level->getTm()->getTileH();
    int width = level->getTm()->getTileW();
    CRS crs = pyr->getTms()->getCrs();
    return CommonGetFeatureInfo( "wmts", layer, bbox, width, height, crs, info_format, X, Y, format, 1 );
}

DataStream* Rok4Server::CommonGetFeatureInfo ( std::string service, Layer* layer, BoundingBox<double> bbox, int width, int height, CRS crs, std::string info_format , int X, int Y, std::string format, int feature_count){
    std::string getFeatureInfoType = layer->getGFIType();
    if ( getFeatureInfoType.compare( "PYRAMID" ) == 0 ) {
        LOGGER_DEBUG("GFI sur pyramide");
        
        BoundingBox<double> pxBbox ( 0.0, 0.0, 0.0, 0.0 );
        pxBbox.xmin = (bbox.xmax-bbox.xmin)/double (width)*double (X) + bbox.xmin;
        pxBbox.xmax = (bbox.xmax-bbox.xmin)/double (width) + pxBbox.xmin;
        pxBbox.ymax = bbox.ymax - (bbox.ymax-bbox.ymin)/double (height)*double (Y);
        pxBbox.ymin = pxBbox.ymax - (bbox.ymax-bbox.ymin)/double (height);
        
        int error;
        Image* image;
        image = layer->getbbox ( servicesConf, pxBbox, 1, 1, crs, 0, error );
        if ( image == 0 ) {
           switch ( error ) {
             case 1: {
               return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "bbox invalide" ), service ) );
             }
             case 2: {
               return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "bbox trop grande" ), service ) );
             }
             default : {
               return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Impossible de repondre a la requete" ), service ) );
             }
           }
        }

        std::vector<std::string> strData;
        Rok4Format::eformat_data pyrType = layer->getDataPyramid()->getFormat();
        int n = image->getChannels();
        switch ( pyrType ) {
            case Rok4Format::TIFF_RAW_INT8 :
            case Rok4Format::TIFF_JPG_INT8 :
            case Rok4Format::TIFF_PNG_INT8 :
            case Rok4Format::TIFF_LZW_INT8 :
            case Rok4Format::TIFF_ZIP_INT8 :
            case Rok4Format::TIFF_PKB_INT8 : {
                uint8_t* intbuffer = new uint8_t[n*sizeof(uint8_t)];
                image->getline(intbuffer,0);
                for ( int i = 0 ; i < n; i ++ ) {
                  std::stringstream ss;
                  ss << (int) intbuffer[i];
                  strData.push_back( ss.str() );
                }
                break;
            }
            case Rok4Format::TIFF_RAW_FLOAT32 :
            case Rok4Format::TIFF_LZW_FLOAT32 :
            case Rok4Format::TIFF_ZIP_FLOAT32 :
            case Rok4Format::TIFF_PKB_FLOAT32 : {
                float* floatbuffer = new float[n*sizeof(float)];
                image->getline(floatbuffer,0);
                for ( int i = 0 ; i < n; i ++ ) {
                  std::stringstream ss;
                  ss << (float) floatbuffer[i];
                  strData.push_back( ss.str() );
                }
                delete[] floatbuffer;
                break;
            }
            default:
              return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Erreur interne."), service ) );
        }
        GetFeatureInfoEncoder gfiEncoder(strData, info_format);
        DataStream* responseDS = gfiEncoder.getDataStream();
        if (responseDS == NULL){
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Info_format non ") +info_format+ _( " supporté par la couche ") + layer->getId() , service ) );
        }
        delete image;
        return responseDS;
        
    } else if ( getFeatureInfoType.compare( "EXTERNALWMS" ) == 0 ) {
        LOGGER_DEBUG("GFI sur WMS externe");
        WebService* myWMSV = new WebService(layer->getGFIBaseUrl(),1,1,10);
        std::stringstream vectorRequest;
        std::string crsstring = crs.getRequestCode();
        if(layer->getGFIForceEPSG()){
            // FIXME
            if(crsstring=="IGNF:LAMB93"){
                crsstring = "EPSG:2154";
            }
        }

        vectorRequest << layer->getGFIBaseUrl()
                << "REQUEST=GetFeatureInfo"
                << "&SERVICE=" << layer->getGFIService()
                << "&VERSION=" << layer->getGFIVersion()
                << "&LAYERS=" << layer->getGFILayers()
                << "&QUERY_LAYERS=" << layer->getGFIQueryLayers()
                << "&INFO_FORMAT=" << info_format
                << "&FORMAT=" << format
                << "&FEATURE_COUNT=" << feature_count
                << "&CRS=" << crsstring
                << "&WIDTH=" << width
                << "&HEIGHT=" << height
                << "&I=" << X
                << "&J=" << Y
                 // compatibilité 1.1.1
                << "&SRS=" << crsstring
                << "&X=" << X
                << "&Y=" << Y;
            
        
        // Les params sont ok : on passe maintenant a la recup de l'info
        char xmin[64];
        sprintf(xmin, "%-.*G", 16, bbox.xmin);
        char xmax[64];
        sprintf(xmax, "%-.*G", 16, bbox.xmax);
        char ymin[64];
        sprintf(ymin, "%-.*G", 16, bbox.ymin);
        char ymax[64];
        sprintf(ymax, "%-.*G", 16, bbox.ymax);

        if ( ( crs.getAuthority() =="EPSG" || crs.getAuthority() =="epsg" || layer->getGFIForceEPSG() ) && crs.isLongLat() && layer->getGFIVersion() == "1.3.0" ) {
            vectorRequest << "&BBOX=" << ymin << "," << xmin << "," << ymax << "," << xmax;
        } else {
            vectorRequest << "&BBOX=" << xmin << "," << ymin << "," << xmax << "," << ymax;
        }

        LOGGER_DEBUG("REQUETE = " << vectorRequest.str());
        RawDataStream* response = myWMSV->performRequestStream (vectorRequest.str());
        if(response == NULL){
            delete myWMSV;
            return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,_ ( "Internal server error" ),"wms" ) );
        }

        delete myWMSV;
        return response;
    } else if ( getFeatureInfoType.compare( "SQL" ) == 0 ) {
        LOGGER_DEBUG("GFI sur SQL");
        // Non géré pour le moment. (nouvelle lib a integrer)
        return new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,_ ( "GFI depuis un SQL non géré." ), service ) );
    } else {
        return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "ERRORRRRRR !" ), service ) );
    }
}


void Rok4Server::processWMTS ( Request* request, FCGX_Request&  fcgxRequest ) {
    if ( request->request == RequestType::GETCAPABILITIES ) {
        S.sendresponse ( WMTSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == RequestType::GETTILE ) {
        S.sendresponse ( getTile ( request ), &fcgxRequest );
    } else if ( request->request == RequestType::GETFEATUREINFO) {
        S.sendresponse ( WMTSGetFeatureInfo ( request ), &fcgxRequest );
    } else if ( request->request == RequestType::GETVERSION ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->getParam("request")+_ ( " n'est pas prise en charge par ce serveur." ) + ROK4_INFO,"wmts" ) ),&fcgxRequest );
    } else if ( request->request == RequestType::REQUEST_MISSING ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre REQUEST n'est pas renseigne." ) ,"wmts" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,_ ( "L'operation " ) +request->getParam("request")+_ ( " n'est pas prise en charge par ce serveur." ),"wmts" ) ),&fcgxRequest );
    }
}


void Rok4Server::processTMS ( Request* request, FCGX_Request&  fcgxRequest ) {

    if ( request->request == RequestType::GETCAPABILITIES ) {
        S.sendresponse ( TMSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == RequestType::GETSERVICES ) {
        S.sendresponse ( TMSGetServices ( request ),&fcgxRequest );
    } else if ( request->request == RequestType::GETTILE ) {
        S.sendresponse ( getTile ( request ), &fcgxRequest );
    } else if ( request->request == RequestType::GETLAYER ) {
        S.sendresponse ( TMSGetLayer ( request ), &fcgxRequest );
    } else if ( request->request == RequestType::GETLAYERMETADATA ) {
        S.sendresponse ( TMSGetLayerMetadata ( request ), &fcgxRequest );
    } else {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, std::string ( "L'operation n'est pas prise en charge par ce serveur." ) + ROK4_INFO,"tms" ) ),&fcgxRequest );
    }
}

void Rok4Server::processWMS ( Request* request, FCGX_Request&  fcgxRequest ) {
    //le capabilities est présent pour une compatibilité avec le WMS 1.1.1
    if ( request->request == RequestType::GETCAPABILITIES) {
        S.sendresponse ( WMSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == RequestType::GETMAP) {
        S.sendresponse ( getMap ( request ), &fcgxRequest );
    } else if ( request->request == RequestType::GETFEATUREINFO) {
        S.sendresponse ( WMSGetFeatureInfo ( request ), &fcgxRequest );
    } else if ( request->request == RequestType::GETVERSION ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->getParam("request")+_ ( " n'est pas prise en charge par ce serveur." ) + ROK4_INFO,"wms" ) ),&fcgxRequest );
    } else if ( request->request == RequestType::REQUEST_MISSING ) {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE, ( "Le parametre REQUEST n'est pas renseigne." ) ,"wms" ) ),&fcgxRequest );
    } else {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED, ( "L'operation " ) +request->getParam("request")+_ ( " n'est pas prise en charge par ce serveur." ),"wms" ) ),&fcgxRequest );
    }
}

void Rok4Server::processRequest ( Request * request, FCGX_Request&  fcgxRequest ) {

    if ( serverConf->supportWMTS && request->service == ServiceType::WMTS) {
        processWMTS ( request, fcgxRequest );
    }
    else if ( serverConf->supportWMS && request->service == ServiceType::WMS ) {
        processWMS ( request, fcgxRequest );
    }
    else if ( serverConf->supportTMS && request->service == ServiceType::TMS) {
        processTMS ( request, fcgxRequest );
    }
    else if ( serverConf->supportTMS && request->service == ServiceType::SERVICE_MISSING) {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_MISSING_PARAMETER_VALUE,_ ( "Le service est manquant" ),"wmts" ) ),&fcgxRequest );
    }
    else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,_ ( "Le service est inconnu pour ce serveur." ),"wmts" ) ),&fcgxRequest );
    }
}

/******************* GETTERS / SETTERS *****************/

ServicesXML* Rok4Server::getServicesConf() { return servicesConf; }
ServerXML* Rok4Server::getServerConf() { return serverConf; }
std::map<std::string, Layer*>& Rok4Server::getLayerList() { return serverConf->layersList; }
std::map<std::string, TileMatrixSet*>& Rok4Server::getTmsList() { return serverConf->tmsList; }
std::map<std::string, Style*>& Rok4Server::getStylesList() { return serverConf->stylesList; }
std::map<std::string,std::vector<std::string> >& Rok4Server::getWmsCapaFrag() { return wmsCapaFrag; }
std::vector<std::string>& Rok4Server::getWmtsCapaFrag() { return wmtsCapaFrag; }

ContextBook* Rok4Server::getObjectBook() {return serverConf->getContextBook();}

int Rok4Server::getFCGISocket() { return sock; }
void Rok4Server::setFCGISocket ( int sockFCGI ) { sock = sockFCGI; }
bool Rok4Server::isRunning() { return running ; }
bool Rok4Server::isWMTSSupported(){ return serverConf->supportWMTS ; }
bool Rok4Server::isWMSSupported(){ return serverConf->supportWMS ; }
bool Rok4Server::isTMSSupported(){ return serverConf->supportTMS ; }
