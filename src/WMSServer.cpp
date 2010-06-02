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

// S.C.
#include "fcgiapp.h"
#include "fcgi_stdio.h"


  /**
   * Boucle principale exécuté par un thread dédié
   */
  void* WMSServer::thread_loop(void* arg) {
    WMSServer* server = (WMSServer*) (arg);   

    int rc;
    FCGX_Request request;

    FCGX_InitRequest(&request, 0, 0);

    while(true)
    {
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

        /* Some platforms require accept() serialization, some don't.. */
        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);

        if (rc < 0)
            break;

	WMSRequest* wmsrequest = new WMSRequest(FCGX_GetParam("QUERY_STRING", request.envp));

      	HttpResponse* response = server->dispatch(wmsrequest);

      	server->S.sendresponse(response, &request);
      	delete wmsrequest;

        sleep(2);

        FCGX_Finish_r(&request);
    }

    return 0;
  }

/**
Construction du serveur
*/

  WMSServer::WMSServer(int nbthread) : nbthread(nbthread) {

	//S.C.
	int init=FCGX_Init();

    std::vector<Layer*> L1;

//    for(int l = 0, s = 8192; s <= 2097152; l++) {
      for(int l = 0, s = 2048; s <= 2097152; l++) {
      double res = double(s)/4096.;
      std::ostringstream ss;
      //ss << "/mnt/geoportail/ppons/ortho/cache/" << s;
      ss << "/mnt/geoportail/ppons/ortho-jpeg/" << s;
      LOGGER_DEBUG( ss.str() );
      Layer *TL = new TiledLayer<RawDecoder>("EPSG:2154", 256, 256, 3, res, res, 0, 16777216, ss.str(),16, 16, 2); //IGNF:LAMB93
      L1.push_back(TL);
      s *= 2;   
    }

    Layer** LL = new Layer*[L1.size()];
    for(int i = 0; i < L1.size(); i++) LL[i] = L1[i];
    Pyramid* P1 = new Pyramid(LL, L1.size());
    //Pyramids["ORTHO"] = P1;
    Pyramids["ORTHO_JPEG"] = P1;
  }
	
#include <fstream>

int main(int argc, char** argv) {

    Logger::configure("../config/logConfig.xml");
    LOGGER_DEBUG( "Lancement du serveur");

    WMSServer W(NB_THREAD);

    W.run(); 
  }

  void WMSServer::run() {

    for(int i = 0; i < nbthread; i++) 
      pthread_create(&Thread[i], NULL, WMSServer::thread_loop, (void*) this);
    for(int i = 0; i < nbthread; i++) 
      pthread_join(Thread[i], NULL);
  }

  HttpResponse* WMSServer::checkWMSrequest(WMSRequest* request) {
    if(request->layers == 0)    return new Error("Missing parameter: layers/l");
    if(request->bbox   == 0)    return new Error("Missing parameter: bbox");
    if(request->width > 10000)  return new Error("Invalid parameter (too large): width/w");
    if(request->height > 10000) return new Error("Invalid parameter (too large): height/h");
    return 0;
  }

  HttpResponse* WMSServer::checkWMTSrequest(WMSRequest* request) {
    if(request->layers == 0)      return new Error("Missing parameter: layers/l");
    if(request->tilecol < 0)      return new Error("Missing or invalid parameter: tilecol/x");
    if(request->tilerow < 0)      return new Error("Missing or invalid parameter: tilerow/y");
    if(request->tilematrix == 0)  return new Error("Missing or invalid parameter: tilematrix/z");    
    return 0;
  }
  
  
  HttpResponse* WMSServer::getMap(WMSRequest* request) {
      LOGGER_DEBUG( "wmsserver:getMap" );
      std::map<std::string, Pyramid*>::iterator it = Pyramids.find(std::string(request->layers));
      if(it == Pyramids.end()) return 0;
      Pyramid* P = it->second;

//      if(!P.transparent) request->transparent = false; // Si la couche ne supporte pas de transparence, forcer transparent = false;
//      if(!request.format) request->format     = P.format;

      Image* image = P->getbbox(*request->bbox, request->width, request->height, request->crs);
      if (image == 0) return 0;
      return new PNGEncoder(image);
    }


    HttpResponse* WMSServer::getTile(WMSRequest* request) {
      LOGGER_DEBUG ("wmsserver:getTile" );

      std::map<std::string, Pyramid*>::iterator it = Pyramids.find(std::string(request->layers));
      if(it == Pyramids.end()) return 0;
      Pyramid* P = it->second;
      //std::stringstream msg(std::ios_base::in);
      //msg << " request : " << request->tilecol << " " << request->tilerow << " " << request->tilematrix << " " << request->transparent << " " << request->format;
      //LOGGER_DEBUG( msg.str() );
      LOGGER_DEBUG(  " request : " << request->tilecol << " " << request->tilerow << " " << request->tilematrix << " " << request->transparent << " " << request->format );
      return P->gettile(request->tilecol, request->tilerow, request->tilematrix);
    }


  HttpResponse* WMSServer::processWMTS(WMSRequest* request) {
    HttpResponse* response;
    response = getTile(request);
    if(response) return response;
    else return new Error("Unknown layer: " + std::string(request->layers));
  }

  HttpResponse* WMSServer::processWMS(WMSRequest* request) {
    HttpResponse* response;
    response = getMap(request);
    if(response) return response;
    else return new Error("Unknown layer: " + std::string(request->layers));
  }


  HttpResponse* WMSServer::dispatch(WMSRequest* request) {
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


