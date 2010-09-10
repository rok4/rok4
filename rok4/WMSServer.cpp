#include "Image.h"

#include "WMSServer.h"
#include <iostream>

#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"

#include <sstream>
#include <vector>
#include <map>
#include "Error.h"

#include <fstream>
#include <signal.h>
#include <cstring>
#include "Logger.h"
#include "Pyramid.h"
#include "TileMatrixSet.h"
#include "Layer.h"
#include "ConfLoader.h"

#include "fcgiapp.h"


  /**
   * Boucle principale exécuté par chaque thread à l'écoute des requêtes de utilisateur.
   */
  void* WMSServer::thread_loop(void* arg) {
    WMSServer* server = (WMSServer*) (arg);   

    int rc;
    FCGX_Request fcgxRequest;

    
//    int sock = 0;
    // Pour faire que le serveur fcgi communique sur le port xxxx utiliser FCGX_OpenSocket
    // Ceci permet de pouvoir lancer l'application sans que ce soit le serveur web qui la lancer automatiquement
    // Utile 
    //  * Pour faire du profiling (grof)
    //  * Pour lancer rok4 sur plusieurs serveurs distants
    //
    //  Voir si le choix ne peut pas être pris automatiquement en regardant comment un serveur web lance l'application fcgi.
    //

    if (FCGX_InitRequest(&fcgxRequest, server->sock, 0)!=0){
    	LOGGER_FATAL("Le listenner FCGI ne peut etre initialise");
    }

//     for(int i = 0; i < 5; i++) {
    while(true){
        rc = FCGX_Accept_r(&fcgxRequest);

        if (rc < 0){
            LOGGER_DEBUG("FCGX_InitRequest renvoie le code d'erreur" << rc);
            break;
        }
	LOGGER_DEBUG("Creation requete");
	Request* request = new Request(FCGX_GetParam("QUERY_STRING", fcgxRequest.envp), FCGX_GetParam("SERVER_NAME", fcgxRequest.envp));
	LOGGER_DEBUG("Traitement requete");
      	HttpResponse* response = server->processRequest(request);
	
	LOGGER_DEBUG("Send response");
      	server->S.sendresponse(response, &fcgxRequest);
	LOGGER_DEBUG("Delete request");
      	delete request;
	LOGGER_DEBUG("Finish");
        FCGX_Finish_r(&fcgxRequest);
    }

    return 0;
  }


/**
Construction du serveur
*/
  WMSServer::WMSServer(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList) :
		               nbThread(nbThread), sock(0), servicesConf(servicesConf), layerList(layerList), tmsList(tmsList) {
	int init=FCGX_Init();
//  sock = FCGX_OpenSocket(":1998", 50);
	buildWMSCapabilities();
	buildWMTSCapabilities();
  }
	

  void WMSServer::run() {

    for(int i = 0; i < nbThread; i++)
      pthread_create(&Thread[i], NULL, WMSServer::thread_loop, (void*) this);
    for(int i = 0; i < nbThread; i++)
      pthread_join(Thread[i], NULL);
  }


  HttpResponse* WMSServer::WMSGetCapabilities(Request* request) {
	  /*FIXME: remplacer le nom du serveur dans les adresses des capabilities*/
	  LOGGER_DEBUG("=> WMSGetCapabilities");
	  LOGGER_DEBUG("GetCapa:               " << WMSCapabilities);
	  char * resp= new char[WMSCapabilities.length()+1];
	  strcpy(resp,WMSCapabilities.c_str());
	  return new StaticHttpResponse("text/xml", (const uint8_t*) resp, WMSCapabilities.length());
  }

  HttpResponse* WMSServer::WMTSGetCapabilities(Request* request) {
	  // TODO à faire
	  return new Error("Not yet implemented!");
  }

  HttpResponse* WMSServer::getMap(Request* request) {
	  std::string layer;
	  BoundingBox<double> bbox(0.0, 0.0, 0.0, 0.0);
	  int width;
	  int height;
	  std::string crs;
	  std::string format;
	  LOGGER_DEBUG( "wmsserver:getMap layers : " << layer );

	  // récupération des paramètres
	  HttpResponse* errorResp = request->getMapParam(layer, bbox, width, height, crs, format);
	  if (errorResp){
		  LOGGER_DEBUG ("probleme dans les parametres de la requete getMap");
		  return errorResp;
	  }

	  // vérification des paramètres
	  std::map<std::string, Layer*>::iterator it = layerList.find(layer);
	  if(it == layerList.end()){
		  LOGGER_DEBUG("le layer "<<layer<<" est inconnu.");
		  return 0;
	  }
	  Layer* L = it->second;

	  Image* image = L->getbbox(bbox, width, height, crs.c_str());
	  if (image == 0) return 0;

	  LOGGER_DEBUG( "wmsserver:getMap : format : " << format);
	  if(format=="image/png")
		  return new PNGEncoder(image);
	  else if(format == "image/tiff")
		  return new TiffEncoder(image);
	  else if(format == "image/jpeg")
		  return new JPEGEncoder(image);

	  return new PNGEncoder(image);
  }


    HttpResponse* WMSServer::getTile(Request* request) {
    	std::string layer;
    	int tileCol;
    	int tileRow;
    	std::string tileMatrixSet;
    	std::string tileMatrix;
    	std::string format;

    	LOGGER_DEBUG ("wmsserver:getTile" );

    	// récupération des paramètres
    	HttpResponse* errorResp = request->getTileParam(layer, tileMatrixSet,tileMatrix, tileCol, tileRow, format);
    	if (errorResp){
    		LOGGER_DEBUG ("probleme dans les parametres de la requete getTile");
    		return errorResp;
    	}
    	LOGGER_DEBUG(  " request : col:" << tileCol << " row:" << tileRow << " tm:" << tileMatrix << " fmt:" << format );

    	// vérification de l'adéquation entre les paramètres et la conf du serveur (ancien checkWMS)
		// existance du layer
    	std::map<std::string, Layer*>::iterator it = layerList.find(layer);
    	if(it == layerList.end()){
    		LOGGER_DEBUG("Erreur layer inexistante : "<< layer);
    		return new Error("Unknown layer");
    	}
    	Layer* L = it->second;
		// l'existance du TMS et TM est controlée dans gettile()


    	// récupération de la tuile.
    	return L->gettile(tileCol, tileRow, tileMatrix);
    }

    /** traite les requêtes de type WMTS */
    HttpResponse* WMSServer::processWMTS(Request* request) {
    	HttpResponse* response;

    	if (request->request == "getcapabilities"){
    		response = WMTSGetCapabilities(request);
    	}else if (request->request == "gettile"){
    		response = getTile(request);
    	}else{
    		LOGGER_DEBUG("Request inconnu");
    		return new Error("Invalid request");
    	}

    	if(response){
    		return response;
    	}else{
    		// on devrait avoir une réponse.
    		return new Error("Unknown error");
    	}

  }

  /** Traite les requêtes de type WMS */
  HttpResponse* WMSServer::processWMS(Request* request) {
    HttpResponse* response;

    if (request->request == "getcapabilities"){
    	response = WMSGetCapabilities(request);
    }else if (request->request == "getmap"){
    	response = getMap(request);
    }else{
		  LOGGER_DEBUG("Request inconnu");
		  return new Error("Invalid request");
    }

    if(response){
    	return response;
    }else{
    	// on devrait avoir une réponse.
    	return new Error("Unknown error");
    }
  }

  /**
   * Renvoie la réponse à la requête.
   */
  HttpResponse* WMSServer::processRequest(Request* request) {
	  LOGGER_DEBUG("Debut Traitement Requete");
	  if(request->service == "wms") {
		  LOGGER_DEBUG("Requete WMS");
		  return processWMS(request);
	  }else if(request->service=="wmts") {
		  LOGGER_DEBUG("Requete WMTS");
		  return processWMTS(request);
	  }else{
		  LOGGER_DEBUG("Service inconnu");
		  return new Error("Invalid request");
	  }
  }


  int main(int argc, char** argv) {

      /* the following loop is for fcgi debugging purpose */
      int stopSleep = 0;
      while (getenv("SLEEP") != NULL && stopSleep == 0) {
          sleep(2);
      }

      /* Chargement de la conf technique du serveur */
      Logger::configure("/var/tmp/rok4cxx.log");
      LOGGER_INFO( "Lancement du serveur ROK4");

      //chargement de la conf technique
      int nbThread;
      std::string layerDir;
      std::string tmsDir;
      if(!ConfLoader::getTechnicalParam(nbThread, layerDir, tmsDir)){
    	  LOGGER_FATAL("Impossible d'interpréter le fichier de conf server.conf");
    	  LOGGER_FATAL("Extinction du serveur ROK4");
    	  // On attend 10s pour eviter que le serveur web nele relance tout de suite
    	  // avec les mêmes conséquences et sature les logs trop rapidement.
    	  //sleep(10);
    	  return 1;
      }

      //chargement de la conf services
      ServicesConf *servicesConf=ConfLoader::buildServicesConf();
      if(!servicesConf){
    	  LOGGER_FATAL("Impossible d'interpréter le fichier de conf services.conf");
    	  LOGGER_FATAL("Extinction du serveur ROK4");
    	  // On attend 10s pour eviter que le serveur web ne le relance tout de suite
    	  // avec les mêmes conséquences et sature les logs trop rapidement.
    	  //sleep(10);
    	  return 1;
      }


      //chargement des TMS
      std::map<std::string,TileMatrixSet*> tmsList;
      if(!ConfLoader::buildTMSList(tmsDir,tmsList)){
    	  LOGGER_FATAL("Impossible de charger la conf des TileMatrix");
    	  LOGGER_FATAL("Extinction du serveur ROK4");
      	  // On attend 10s pour eviter que le serveur web ne le relance tout de suite
      	  // avec les mêmes conséquences et sature les logs trop rapidement.
      	  /* DEBUG
      	   sleep(10);*/
      	  return 1;
      }

      //chargement des Layers
      std::map<std::string, Layer*> layerList;
      if(!ConfLoader::buildLayersList(layerDir,tmsList,layerList)){
    	  LOGGER_FATAL("Impossible de charger la conf des Layers/pyramides");
    	  LOGGER_FATAL("Extinction du serveur ROK4");
      	  // On attend 10s pour eviter que le serveur web nele relance tout de suite
      	  // avec les mêmes conséquences et sature les logs trop rapidement.
      	  /* DEBUG
      	    sleep(10);*/
      	  return 1;
      }

      //construction du serveur.
      WMSServer W(nbThread, *servicesConf, layerList, tmsList);

      W.run();

      LOGGER_INFO( "Extinction du serveur ROK4");
    }



