#ifndef _WMSSERVER_
#define _WMSSERVER_

#include "config.h"

#include "ResponseSender.h"
#include "HttpResponse.h"
#include "WMSRequest.h"

#include <pthread.h>
#include <map>

#include "ServicesConf.h"
#include "Layer.h"
#include "TileMatrixSet.h"


class WMSServer {
  private:
  int nbThread;
  pthread_t Thread[128]; /* tableau des threads => 128 thread max! *//*FIXME mettre un conteneur sans limite */
  ResponseSender S;

  ServicesConf &servicesConf;
  std::map<std::string, Layer*> layers;
  std::map<std::string,TileMatrixSet*> tmsList;

  static void* thread_loop(void* arg);

  HttpResponse* getMap(WMSRequest* request);
  HttpResponse* getTile(WMSRequest* request);
  HttpResponse* processRequest(WMSRequest *request);
  HttpResponse* processWMS (WMSRequest *request);
  HttpResponse* processWMTS(WMSRequest *request);

  public:
  void run();
  WMSServer(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layers, std::map<std::string,TileMatrixSet*> &tmsList);

};

#endif

