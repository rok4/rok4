#include "WMSServer.h"

#include "Image.h"
#include "Layer.h"
#include "Pyramid.h"

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

#include <signal.h>
#include "Logger.h"
#include "PyramidFactory.h"

// S.C.
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
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

        /* Some platforms require accept() serialization, some don't.. */
        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);

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

  WMSServer::WMSServer(int nbthread) : nbthread(nbthread) {

	int init=FCGX_Init();
	/* TODO: c'est là qu'on va charger le fichier de conf du contenu servi par le serveur */

	/* TODO on obtient une liste des fichier de conf des pyramides à servir */
	std::map<std::string, std::string> PyramidConfs;
	PyramidConfs["ORTHO"] = "/mnt/geoportail/ppons/ortho-jpeg/pyramid.xml";
	PyramidConfs["ORTHO_TIFF"] = "/mnt/geoportail/ppons/ortho/cache/pyramid.xml";

	/* Pour chaque fichier on crée la pyramide décrite */
	std::map<std::string, std::string>::iterator iter = PyramidConfs.begin();
	while (iter != PyramidConfs.end()){
		Pyramid* P = PyramidFactory::make(iter->second);
		if (P==NULL){
			LOGGER_ERROR("La description de pyramide n'a pu être chargée " << iter->second);
			++iter;
			continue;
		}
		pyramids[iter->first] = P;
		++iter;
	}
	if (pyramids.empty()){
		LOGGER_FATAL("Le serveur n'a pu charger aucune pyramide: aucune image ne pourra être servie.")
	}
  }
	
#include <fstream>

int main(int argc, char** argv) {

	/* the following loop is for fcgi debugging purpose */
    int stopSleep = 0;
    while (getenv("SLEEP") != NULL && stopSleep == 0) {
        sleep(2);
    }

    Logger::configure("../config/logConfig.xml");
    LOGGER_INFO( "Lancement du serveur ROK4");

    WMSServer W(NB_THREAD);
    W.run();

    LOGGER_INFO( "Extinction du serveur ROK4");
  }

  void WMSServer::run() {

    for(int i = 0; i < nbthread; i++) 
      pthread_create(&Thread[i], NULL, WMSServer::thread_loop, (void*) this);
    for(int i = 0; i < nbthread; i++) 
      pthread_join(Thread[i], NULL);
  }

  HttpResponse* WMSServer::getMap(WMSRequest* request) {
      LOGGER_DEBUG( "wmsserver:getMap" );

      std::map<std::string, Pyramid*>::iterator it = pyramids.find(std::string(request->layers));
      if(it == pyramids.end()) return 0;
      Pyramid* P = it->second;

      Image* image = P->getbbox(*request->bbox, request->width, request->height, request->crs);
      if (image == 0) return 0;
      return new PNGEncoder(image);
    }


    HttpResponse* WMSServer::getTile(WMSRequest* request) {
      LOGGER_DEBUG ("wmsserver:getTile" );

      std::map<std::string, Pyramid*>::iterator it = pyramids.find(std::string(request->layers));
      if(it == pyramids.end()) return 0;
      Pyramid* P = it->second;

      LOGGER_DEBUG(  " request : " << request->tilecol << " " << request->tilerow << " " << request->tilematrix << " " << request->transparent << " " << request->format );
      return P->gettile(request->tilecol, request->tilerow, request->tilematrix);
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


