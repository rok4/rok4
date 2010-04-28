#include "WMSServer.h"

#include "Image.h"
#include "Layer.h"
#include "Pyramid.h"

//#include <iostream>
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
#include <cstdio>

  /*
   * Boucle principale exécuté par un thread dédié
   */
  void* WMSServer::thread_loop(void* arg) {
    WMSServer* server = (WMSServer*) (arg);   
//    for(int i = 0; i < 4; i++) {
    while(1) {
      int conn_fd = server->L.get_connection();
      WMSRequest* request = new WMSRequest(conn_fd);      
      if(!request->query) {
        if(close(conn_fd) < 0) perror("WMSServer::thread_loop close");
        delete request;
        continue;
      }
      HttpResponse* response = server->dispatch(request);
      server->S.sendresponse(response, conn_fd);
      delete request;
    }
    return 0;
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
      Logger(DEBUG) << "wmsserver:getMap" << std::endl;
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
      Logger(DEBUG) << "wmsserver:getTile" << std::endl;

      std::map<std::string, Pyramid*>::iterator it = Pyramids.find(std::string(request->layers));
      if(it == Pyramids.end()) return 0;
      Pyramid* P = it->second;

      LOGGER(DEBUG) << " request : " << request->tilecol << " " << request->tilerow << " " << request->tilematrix << " " << request->transparent << " " << request->format << std::endl;
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


  void WMSServer::run() {
    L.start();
    for(int i = 0; i < nbthread; i++) 
      pthread_create(&Thread[i], NULL, WMSServer::thread_loop, (void*) this);
    for(int i = 0; i < nbthread; i++) 
      pthread_join(Thread[i], NULL);
  }




  WMSServer::WMSServer(int nbthread, int port) : nbthread(nbthread), L(port) {

    std::vector<Layer*> L1;

    for(int l = 0, s = 8192; s <= 2097152; l++) {
      double res = double(s)/4096.;
      std::ostringstream ss;
      ss << "/mnt/geoportail/ppons/ortho/cache/" << s;
//      ss << "/media/disk/cache/ortho/" << s;      
      LOGGER(DEBUG) << ss.str() << std::endl;
      Layer *TL = new TiledLayer<RawDecoder>("EPSG:2154", 256, 256, 3, res, res, 0, 16777216, ss.str(),16, 16, 2); //IGNF:LAMB93
      L1.push_back(TL);
      s *= 2;   
    }

    Layer** LL = new Layer*[L1.size()];
    for(int i = 0; i < L1.size(); i++) LL[i] = L1[i];
    Pyramid* P1 = new Pyramid(LL, L1.size());
    Pyramids["ORTHO"] = P1;
/*
    std::vector<Layer<pixel_rgb>*> L2;
    for(int l = 0, s = 2048; s <= 2097152; l++) {
      double res = double(s)/4096.;
      std::ostringstream ss;
      ss << "/mnt/geoportail/ppons/ortho-jpeg/" << s;
      LOGGER(DEBUG) << ss.str() << std::endl;
      Layer<pixel_rgb> *TL = new TiledFileLayer<JpegTile ,pixel_rgb>("IGNF:LAMB93", 256, 256, res, 0, 16777216, ss.str(),16,2); //IGNF:LAMB93
      L2.push_back(TL);
      s *= 2;   
    }

    Pyramid<pixel_rgb>* P2 = new Pyramid<pixel_rgb>(L2, "image/jpeg", false);
    Layers_rgb["ORTHO_JPEG"] = P2;


    std::vector<Layer<pixel_gray>*> L3;
    for(int l = 0, s = 512; s <= 2097152; l++) {
      double res = double(s)/4096.;
      std::ostringstream ss;
      ss << "/mnt/geoportail/ppons/parcellaire/" << s;
      LOGGER(DEBUG) << ss.str() << std::endl;
      Layer<pixel_gray> *TL = new TiledFileLayer<PngTile ,pixel_gray>("IGNF:LAMB93", 256, 256, res, 0, 16777216, ss.str(),16,2); //IGNF:LAMB93
      L3.push_back(TL);
      s *= 2;   
    }

    Pyramid<pixel_gray>* P3 = new Pyramid<pixel_gray>(L3, "image/png", true);
    Layers_gray["PARCELLAIRE"] = P3;


    std::vector<Layer<pixel_rgb>*> L4;
    for(int l = 0, s = 16384; s <= 2097152; l++) {
      double res = double(s)/4096.;
      std::ostringstream ss;
//      ss << "/mnt/geoportail/ppons/scan25/cache/" << s;
      ss << "/data/cache/scan25/" << s;      
      LOGGER(DEBUG) << ss.str() << std::endl;
      Layer<pixel_rgb> *TL = new TiledFileLayer<RawTile ,pixel_rgb>("EPSG:2154", 256, 256, res, 0, 16777216, ss.str(),16,2); //IGNF:LAMB93
      L4.push_back(TL);
      s *= 2;   
    }

    Pyramid<pixel_rgb>* P4 = new Pyramid<pixel_rgb>(L4, "image/png", true);
    Layers_rgb["SCAN25"] = P4;



    std::vector<Layer<pixel_rgb>*> L5;
    double s=1228.8;
    for(int l = 0 ; s <= 5033164.8; l++) {
      double res = s/4096.;
      std::ostringstream ss;
      ss.precision(10);
      ss << "/mnt/geoportail/ppons/ortho_idf/" << s;
      LOGGER(DEBUG) << ss.str() << std::endl;
      Layer<pixel_rgb> *TL = new TiledFileLayer<RawTile ,pixel_rgb>("IGNF:LAMB93", 256, 256, res, 0, 10066329.6, ss.str(),16,2); //IGNF:LAMB93
      L5.push_back(TL);
      s *= 2;
    }

    Pyramid<pixel_rgb>* P5 = new Pyramid<pixel_rgb>(L5, "image/tiff", false);
    Layers_rgb["IDF"] = P5;


    std::vector<Layer<pixel_float>*> L6;
    s=2.8125;
    for(int l = 0 ; s <= 90; l++) {
      double res = s/8192.;
      std::ostringstream ss;
      ss.precision(15);
      ss << "/mnt/geoportail/ppons/wms_mnt/" << s;
      LOGGER(DEBUG) << ss.str() << std::endl;
      Layer<pixel_float> *TL = new TiledFileLayer<RawTile ,pixel_float>("IGNF:RGF93", 256, 256, res, -90.0, 90.0, ss.str(),32,0); //IGNF:RGF93
      L6.push_back(TL);
      s *= 2;
    }

    Pyramid<pixel_float>* P6 = new Pyramid<pixel_float>(L6, "image/bil", false);
    Layers_float["MNT"] = P6;
*/
  }


  int main(int argc, char** argv) {

    LOGGER(DEBUG) << "Lancement du serveur" << std::endl;
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, 0) == -1) perror("sigaction");

    WMSServer W(NB_THREAD,PORT_WMSSERVER);
    W.run(); 
  }


