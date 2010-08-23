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
    FCGX_Request request;

    
    int sock = 0;
    // Pour faire que le serveur fcgi communique sur le port xxxx utiliser FCGX_OpenSocket
    // Ceci permet de pouvoir lancer l'application sans que ce soit le serveur web qui la lancer automatiquement
    // Utile 
    //  * Pour faire du profiling (grof)
    //  * Pour lancer rok4 sur plusieurs serveurs distants
    //
    //  Voir si le choix ne peut pas être pris automatiquement en regardant comment un serveur web lance l'application fcgi.
    //
    //  sock = FCGX_OpenSocket(":1998", 50);

    if (FCGX_InitRequest(&request, sock, 0)!=0){
    	LOGGER_FATAL("Le listenner FCGI ne peut etre initialise");
    }

//     for(int i = 0; i < 5; i++) {
    while(true){
        rc = FCGX_Accept_r(&request);

        if (rc < 0){
            LOGGER_DEBUG("FCGX_InitRequest renvoie le code d'erreur" << rc);
            break;
        }
	LOGGER_DEBUG("Creation requete");
	WMSRequest* wmsrequest = new WMSRequest(FCGX_GetParam("QUERY_STRING", request.envp));
	LOGGER_DEBUG("Traitement requete");
      	HttpResponse* response = server->processRequest(wmsrequest);
	
	LOGGER_DEBUG("Send response");
      	server->S.sendresponse(response, &request);
	LOGGER_DEBUG("Delete request");
      	delete wmsrequest;
	LOGGER_DEBUG("Finish");
        FCGX_Finish_r(&request);
    }

    return 0;
  }

/**
Construction du serveur
*/

  WMSServer::WMSServer(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layers, std::map<std::string,TileMatrixSet*> &tmsList) :
		               nbThread(nbThread), servicesConf(servicesConf), layers(layers), tmsList(tmsList) {
	int init=FCGX_Init();
  }
	

  void WMSServer::run() {

    for(int i = 0; i < nbThread; i++)
      pthread_create(&Thread[i], NULL, WMSServer::thread_loop, (void*) this);
    for(int i = 0; i < nbThread; i++)
      pthread_join(Thread[i], NULL);
  }

  HttpResponse* WMSServer::getMap(WMSRequest* request) {
      LOGGER_DEBUG( "wmsserver:getMap layers : " << request->layers );

      std::map<std::string, Layer*>::iterator it = layers.find(std::string(request->layers));
     LOGGER_DEBUG( "it");
      if(it == layers.end())
	{	
		LOGGER_DEBUG( "Pas de tel layers : ");
		return 0;
	}
      Layer* L = it->second;
     LOGGER_DEBUG( "it1");
      Image* image = L->getbbox(*request->bbox, request->width, request->height, request->crs);
     LOGGER_DEBUG( "it2");
      if (image == 0) return 0;

      LOGGER_DEBUG( "wmsserver:getMap : format : " << request->format);
      if(strncmp(request->format, "image/png", 9) == 0)
	return new PNGEncoder(image);
      else if(strncmp(request->format, "image/tiff", 10) == 0)
        return new TiffEncoder(image);
      else if(strncmp(request->format, "image/jpeg", 10) == 0)
        return new JPEGEncoder(image);
      else
	return new PNGEncoder(image);
  }


    HttpResponse* WMSServer::getTile(WMSRequest* request) {
      LOGGER_DEBUG ("wmsserver:getTile" );

      std::map<std::string, Layer*>::iterator it = layers.find(std::string(request->layers));
      if(it == layers.end())
	{
		LOGGER_DEBUG("Erreur layer inexistante : "<<request->layers);
		return 0;
	}
      Layer* L = it->second;

      LOGGER_DEBUG(  " request : " << request->tilecol << " " << request->tilerow << " " << request->tilematrix << " " << request->transparent << " " << request->format );
      return L->gettile(request->tilecol, request->tilerow, request->tilematrix);
    }

  /** traite les requêtes de type WMTS */
  HttpResponse* WMSServer::processWMTS(WMSRequest* request) {
    HttpResponse* response;
    response = getTile(request);
    if(response) return response;
    else return new Error("Unknown layer: " + std::string(request->layers));
  }

  /** Traite les requêtes de type WMS */
  HttpResponse* WMSServer::processWMS(WMSRequest* request) {
    HttpResponse* response;
    response = getMap(request);
    if(response) return response;
    else return new Error("Unknown layer: " + std::string(request->layers));
  }

  /**
   * Renvoie la réponse à la requête.
   */
  HttpResponse* WMSServer::processRequest(WMSRequest* request) {
        LOGGER_DEBUG("Debut Traitement Requete");
    if(request->isWMSRequest()) {
        LOGGER_DEBUG("Requete WMS");
      HttpResponse* response = request->checkWMS();
      if(response)
      {
	LOGGER_DEBUG("Requete WMS invalide");
	return response;
      }
      return processWMS(request);
    }
    else if(request->isWMTSRequest()) {
	LOGGER_DEBUG("Requete WMTS");
      HttpResponse* response = request->checkWMTS();
      if(response)
	{
		LOGGER_DEBUG("Requete WMTS invalide");
		return response;
	}
      return processWMTS(request);
    }
    else 
	{
	LOGGER_DEBUG("Requete invalide");
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
      Logger::configure(LOG_CONF_PATH);
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
      std::map<std::string, Layer*> layers;
      if(!ConfLoader::buildLayersList(layerDir,tmsList,layers)){
    	  LOGGER_FATAL("Impossible de charger la conf des Layers/pyramides");
    	  LOGGER_FATAL("Extinction du serveur ROK4");
      	  // On attend 10s pour eviter que le serveur web nele relance tout de suite
      	  // avec les mêmes conséquences et sature les logs trop rapidement.
      	  /* DEBUG
      	    sleep(10);*/
      	  return 1;
      }

      //construction du serveur.
      WMSServer W(nbThread, *servicesConf, layers, tmsList);

      W.run();

      LOGGER_INFO( "Extinction du serveur ROK4");
    }



