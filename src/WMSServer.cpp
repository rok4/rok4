#include "Image.h"

#include "WMSServer.h"
#include <iostream>

//#include <fstream>
//#include "pixel_type.h"
//#include "TiffEncoder.h"
#include "PNGEncoder.h"
//#include "JPEGEncoder.h"
//#include "BilEncoder.h"
//#include "Encoder.h"




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

#include "libfcgi/fcgiapp.h"


  /**
   * Boucle principale exécuté par chaque thread à l'écoute des requêtes de utilisateur.
   */
  void* WMSServer::thread_loop(void* arg) {
    WMSServer* server = (WMSServer*) (arg);   

    int rc;
    FCGX_Request request;

    if (FCGX_InitRequest(&request, 0, 0)!=0){
    	LOGGER_FATAL("Le listenner FCGI ne peut etre initialise");
    }

    while(true){
//        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

        /* Some platforms require accept() serialization, some don't.. */
//        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
//        pthread_mutex_unlock(&accept_mutex);

        if (rc < 0){
            LOGGER_DEBUG("FCGX_InitRequest renvoie le code d'erreur" << rc);
            break;
        }

	    WMSRequest* wmsrequest = new WMSRequest(FCGX_GetParam("QUERY_STRING", request.envp));

      	HttpResponse* response = server->processRequest(wmsrequest);

      	server->S.sendresponse(response, &request);
      	delete wmsrequest;

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
      LOGGER_DEBUG( "wmsserver:getMap" );

      std::map<std::string, Layer*>::iterator it = layers.find(std::string(request->layers));
      if(it == layers.end()) return 0;
      Layer* L = it->second;

      Image* image = L->getbbox(*request->bbox, request->width, request->height, request->crs);
      if (image == 0) return 0;
      return new PNGEncoder(image);
  }


    HttpResponse* WMSServer::getTile(WMSRequest* request) {
      LOGGER_DEBUG ("wmsserver:getTile" );

      std::map<std::string, Layer*>::iterator it = layers.find(std::string(request->layers));
      if(it == layers.end()) return 0;
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
    if(request->isWMSRequest()) {
      HttpResponse* response = request->checkWMS();
      if(response) return response;
      return processWMS(request);
    }
    else if(request->isWMTSRequest()) {
      HttpResponse* response = request->checkWMTS();
      if(response) return response;
      return processWMTS(request);
    }
    else return new Error("Invalid request");
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



