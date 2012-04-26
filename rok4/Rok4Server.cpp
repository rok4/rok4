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

#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"
#include "BilEncoder.h"
#include "Message.h"
#include "Logger.h"
#include "TileMatrixSet.h"
#include "Layer.h"
#include "ServiceException.h"
#include "fcgiapp.h"
#include "PaletteDataSource.h"



/**
 * Boucle principale exécuté par chaque thread à l'écoute des requêtes de utilisateur.
 */
void* Rok4Server::thread_loop ( void* arg ) {
    Rok4Server* server = ( Rok4Server* ) ( arg );
    FCGX_Request fcgxRequest;
    if ( FCGX_InitRequest ( &fcgxRequest, server->sock, FCGI_FAIL_ACCEPT_ON_INTR ) !=0 ) {
        LOGGER_FATAL ( "Le listener FCGI ne peut etre initialise" );
    }

    while ( server->isRunning() ) {
        std::string content;
        bool postRequest;

        int rc;
        if ( ( rc=FCGX_Accept_r ( &fcgxRequest ) ) < 0 ) {
            LOGGER_ERROR ( "FCGX_InitRequest renvoie le code d'erreur" << rc );
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
            LOGGER_DEBUG ( "Request Content :"<< std::endl << content );
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
    LOGGER_DEBUG("Extinction du thread");
    return 0;
}

/**
* @brief Construction du serveur
*/
Rok4Server::Rok4Server ( int nbThread, ServicesConf& servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList, std::map<std::string,Style*> &styleList, char *& projEnv) :
        sock ( 0 ), servicesConf ( servicesConf ), layerList ( layerList ), tmsList ( tmsList ),styleList(styleList), projEnv(projEnv) , threads ( nbThread ), running(false), notFoundError(NULL) {

    LOGGER_DEBUG ( "Build WMS Capabilities" );
    buildWMSCapabilities();
    LOGGER_DEBUG ( "Build WMTS Capabilities" );
    buildWMTSCapabilities();
}

Rok4Server::~Rok4Server()
{
    if (notFoundError) {
        delete notFoundError;
        notFoundError = NULL;
    }
}

void Rok4Server::initFCGI()
{
    int init=FCGX_Init();

    // Pour faire que le serveur fcgi communique sur le port xxxx utiliser FCGX_OpenSocket
    // Ceci permet de pouvoir lancer l'application sans que ce soit le serveur web qui la lancer automatiquement
    // Utile
    //  * Pour faire du profiling (grof)
    //  * Pour lancer rok4 sur plusieurs serveurs distants
    //  Voir si le choix ne peut pas être pris automatiquement en regardant comment un serveur web lance l'application fcgi.

    // A décommenter pour utiliser valgrind
    // Ex : valgrind --leak-check=full --show-reachable=yes rok4 2> leak.txt
    // Ensuite redemarrer le serveur Apache configure correctement. Attention attendre suffisamment longtemps l'initialisation de valgrind

    // sock = FCGX_OpenSocket(":9000", 0);

    // Cf. aussi spawn-fcgi qui est un spawner pour serveur fcgi et qui permet de specifier un port d ecoute
    // Exemple : while (true) ; do spawn-fcgi -n -p 9000 -- ./rok4 -f ../config/server-nginx.conf ; done
}



/*
 * Lancement des threads du serveur
 */
void Rok4Server::run() {
    running = true;

    for ( int i = 0; i < threads.size(); i++ ) {
        pthread_create ( & ( threads[i] ), NULL, Rok4Server::thread_loop, ( void* ) this );
    }
    for ( int i = 0; i < threads.size(); i++ )
        pthread_join ( threads[i], NULL );
}

void Rok4Server::terminate()
{
    running = false;
    //FCGX_ShutdownPending();
    // Terminate FCGI Thread
    for ( int i = 0; i < threads.size(); i++ ) {
        pthread_kill(threads[i], SIGPIPE );
    }

}



/**
 * @vriedf test de la présence de paramName dans option
 * @return true si présent
 */
bool Rok4Server::hasParam ( std::map<std::string, std::string>& option, std::string paramName ) {
    std::map<std::string, std::string>::iterator it = option.find ( paramName );
    if ( it == option.end() ) {
        return false;
    }
    return true;
}


/**
 * @vriedf récupération du parametre paramName dans la requete
 * @return la valeur du parametre si existant "" sinon
 */
std::string Rok4Server::getParam ( std::map<std::string, std::string>& option, std::string paramName ) {
    std::map<std::string, std::string>::iterator it = option.find ( paramName );
    if ( it == option.end() ) {
        return "";
    }
    return it->second;
}



DataStream* Rok4Server::WMSGetCapabilities ( Request* request ) {

    std::string version;
    DataStream* errorResp = request->getCapWMSParam(servicesConf,version);
    if ( errorResp ) {
        LOGGER_ERROR ( "Probleme dans les parametres de la requete getCapabilities" );
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

    std::string version;
    DataStream* errorResp = request->getCapWMTSParam(servicesConf,version);
    if ( errorResp ) {
        LOGGER_ERROR ( "Probleme dans les parametres de la requete getCapabilities" );
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

/*
 * Traitement d'une requete GetMap
 * @return Un pointeur sur le flux de donnees resultant
 * @return Un message d'erreur en cas d'erreur
 */

DataStream* Rok4Server::getMap ( Request* request ) {
    Layer* L;
    BoundingBox<double> bbox ( 0.0, 0.0, 0.0, 0.0 );
    int width, height;
    CRS crs;
    std::string format;
    Style* style=0;
    std::map <std::string, std::string > format_option;

    // Récupération des paramètres
    DataStream* errorResp = request->getMapParam ( servicesConf, layerList, L, bbox, width, height, crs, format ,style, format_option );
    if ( errorResp ) {
        LOGGER_ERROR ( "Probleme dans les parametres de la requete getMap" );
        return errorResp;
    }

    int error;
    Image* image = L->getbbox (servicesConf, bbox, width, height, crs, error );

    LOGGER_DEBUG ( "GetMap de Style : " << style->getId() << " pal size : "<<style->getPalette()->getPalettePNGSize() );

    if ( image == 0 ) {
        switch (error) {

        case 1: {
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"bbox invalide","wms" ) );
        }
        case 2: {
            return new SERDataStream ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"bbox trop grande","wms" ) );
        }
        default : {
            return new SERDataStream ( new ServiceException ( "",OWS_NOAPPLICABLE_CODE,"Impossible de repondre a la requete","wms" ) );
        }
        }
    }
    if ( format=="image/png" )
        return new PNGEncoder ( image,style->getPalette() );
    else if ( format == "image/tiff" ) { // Handle compression option
        eformat_data pyrType = L->getDataPyramid()->getFormat();
        switch (pyrType) {

        case TIFF_RAW_FLOAT32 :
        case TIFF_DEFLATE_FLOAT32 :
        case TIFF_LZW_FLOAT32 :
            if ( getParam(format_option,"compression").compare("lzw")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_LZW_FLOAT32);
            }
            if ( getParam(format_option,"compression").compare("deflate")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_DEFLATE_FLOAT32);
            }
            if ( getParam(format_option,"compression").compare("raw")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_RAW_FLOAT32);
            }
            return TiffEncoder::getTiffEncoder(image, pyrType);
        case TIFF_RAW_INT8 :
        case TIFF_DEFLATE_INT8 :
        case TIFF_LZW_INT8 :
            if ( getParam(format_option,"compression").compare("lzw")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_LZW_INT8);
            }
            if ( getParam(format_option,"compression").compare("deflate")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_DEFLATE_INT8);
            }
            if ( getParam(format_option,"compression").compare("raw")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_RAW_INT8);
            }
            return TiffEncoder::getTiffEncoder(image, pyrType);
        default:
            if ( getParam(format_option,"compression").compare("lzw")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_LZW_INT8);
            }
            if ( getParam(format_option,"compression").compare("deflate")==0) {
                return TiffEncoder::getTiffEncoder(image, TIFF_DEFLATE_INT8);
            }
            return TiffEncoder::getTiffEncoder(image, TIFF_RAW_INT8);
        }
    }
    else if ( format == "image/jpeg" )
        return new JPEGEncoder ( image );
    else if ( format == "image/x-bil;bits=32" )
        return new BilEncoder ( image );
    LOGGER_ERROR ( "Le format "<<format<<" ne peut etre traite" );
    return new SERDataStream ( new ServiceException ( "",WMS_INVALID_FORMAT,"Le format "+format+" ne peut etre traite","wms" ) );
}

/*
 * Traitement d'une requete GetTile
 * @return Un pointeur sur la source de donnees de la tuile requetee
 * @return Un message d'erreur en cas d'erreur
 */

DataSource* Rok4Server::getTile ( Request* request ) {
    Layer* L;
    std::string tileMatrix,format;
    int tileCol,tileRow;
    bool noDataError;
    Style* style=0;

    // Récupération des parametres de la requete
    DataSource* errorResp = request->getTileParam ( servicesConf, tmsList, layerList, L, tileMatrix, tileCol, tileRow, format, style, noDataError );

    if ( errorResp ) {
        LOGGER_ERROR ( "Probleme dans les parametres de la requete getTile" );
        return errorResp;
    }
    errorResp = NULL;
    if (noDataError) {
        if (!notFoundError) {
            notFoundError = new SERDataSource ( new ServiceException("", HTTP_NOT_FOUND, "No data found", "wmts") );
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

/** Traite les requêtes de type WMTS */
void Rok4Server::processWMTS ( Request* request, FCGX_Request&  fcgxRequest ) {
    if ( request->request == "getcapabilities" ) {
        S.sendresponse ( WMTSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == "gettile" ) {
        S.sendresponse ( getTile ( request ), &fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,"L'operation "+request->request+" n'est pas prise en charge par ce serveur.","wmts" ) ),&fcgxRequest );
    }
}

/** Traite les requêtes de type WMS */
void Rok4Server::processWMS ( Request* request, FCGX_Request&  fcgxRequest ) {
    if ( request->request == "getcapabilities" ) {
        S.sendresponse ( WMSGetCapabilities ( request ),&fcgxRequest );
    } else if ( request->request == "getmap" ) {
        S.sendresponse ( getMap ( request ), &fcgxRequest );
    } else {
        S.sendresponse ( new SERDataStream ( new ServiceException ( "",OWS_OPERATION_NOT_SUPORTED,"L'operation "+request->request+" n'est pas prise en charge par ce serveur.","wms" ) ),&fcgxRequest );
    }
}

/** Separe les requetes WMS et WMTS */
void Rok4Server::processRequest ( Request * request, FCGX_Request&  fcgxRequest ) {
    if ( request->service == "wms" ) {
        processWMS ( request, fcgxRequest );
    } else if ( request->service=="wmts" ) {
        processWMTS ( request, fcgxRequest );
    } else {
        S.sendresponse ( new SERDataSource ( new ServiceException ( "",OWS_INVALID_PARAMETER_VALUE,"Le service "+request->service+" est inconnu pour ce serveur.","wmts" ) ),&fcgxRequest );
    }
}


