#ifndef _WMSSERVER_
#define _WMSSERVER_

#include "config.h"

#include "ResponseSender.h"
#include "HttpResponse.h"
#include "WMSRequest.h"

#include <pthread.h>
#include "Pyramid.h"
#include <map>


class WMSServer {
  private:
  int nbthread;
  pthread_t Thread[128]; /* tableau des threads => 128 thread max! */
  ResponseSender S; 

  std::map<std::string, Pyramid*> pyramids;

  static void* thread_loop(void* arg);

  HttpResponse* getMap(WMSRequest* request);
  HttpResponse* getTile(WMSRequest* request);
  HttpResponse* processRequest(WMSRequest *request);
  HttpResponse* processWMS (WMSRequest *request);
  HttpResponse* processWMTS(WMSRequest *request);

  public:
  void run();
  WMSServer(int nbthread);
};


#endif

