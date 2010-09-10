#ifndef _WMSSERVER_
#define _WMSSERVER_

#include "config.h"

#include "ResponseSender.h"
#include "HttpResponse.h"
#include "Request.h"

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

  int sock;

  ServicesConf &servicesConf;
  std::map<std::string, Layer*> layerList;
  std::map<std::string,TileMatrixSet*> tmsList;

  static void* thread_loop(void* arg);

  void buildWMSCapabilities();
  void buildWMTSCapabilities();

  HttpResponse* getMap(Request* request);
  HttpResponse* getTile(Request* request);
  HttpResponse* WMSGetCapabilities(Request* request);
  HttpResponse* WMTSGetCapabilities(Request* request);
  HttpResponse* processRequest(Request *request);
  HttpResponse* processWMS (Request *request);
  HttpResponse* processWMTS(Request *request);

  public:
  std::string WMSCapabilities;
  std::string WMTSCapabilities;

  void run();
  WMSServer(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList);

};

#endif

