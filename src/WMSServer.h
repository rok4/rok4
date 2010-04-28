#ifndef _WMSSERVER_
#define _WMSSERVER_

#include "config.h"

#include "Listener.h"
#include "ResponseSender.h"
#include "HttpResponse.h"
#include "WMSRequest.h"

#include <pthread.h>
#include "Pyramid.h"
#include <map>
//#include "pixel_type.h"


class WMSServer {
  private:
  int nbthread;
  pthread_t Thread[128];
  Listener L;
  ResponseSender S; 

  std::map<std::string, Pyramid*> Pyramids;

  static void* thread_loop(void* arg);

  HttpResponse* checkWMSrequest(WMSRequest* request);
  HttpResponse* checkWMTSrequest(WMSRequest* request);



  HttpResponse* getMap(WMSRequest* request);
  HttpResponse* getTile(WMSRequest* request);
  HttpResponse* processRequest(WMSRequest *request);
  HttpResponse* processWMS (WMSRequest *request);
  HttpResponse* processWMTS(WMSRequest *request);
  HttpResponse* dispatch(WMSRequest *request);



  public:
  void run();
  WMSServer(int nbthread, int port);
};


#endif

