#ifndef _WMSREQUEST_
#define _WMSREQUEST_
#include <cstring>
#include "config.h"
#include "BoundingBox.h"
#include "Error.h"
#include "Logger.h"

class WMSRequest {
  private:
  void url_decode(char *src);
  void parseparam(char* key, char* value);
  int load(int conn_fd);

  public:
  int width, height;
  BoundingBox<double> *bbox;
  char* service;
  char* request;
  char* crs;
  char* layers;
  char* styles;
  const char* format;
  bool transparent;
 
  int tilerow;
  int tilecol;
  std::string tilematrix;
  char* tilematrixset;


  bool isWMTSRequest() {
LOGGER_DEBUG("1111");
    if(service) return (strcmp(service, "WMTS") == 0);
    if(tilecol >= 0 && tilerow >= 0 && tilematrix != "") return true;
LOGGER_DEBUG("2222");
    return false;
  }
  bool isWMSRequest() {
    if(service) return (strcmp(service, "WMS") == 0);
    if(bbox) return true;
    return false;
  }
  HttpResponse* checkWMS() {   
    if(layers == 0)    LOGGER_DEBUG("Missing parameter: layers/l");//return new Error("Missing parameter: layers/l");
    if(bbox   == 0)    LOGGER_DEBUG("Missing parameter: bbox");//return new Error("Missing parameter: bbox");
    if(width > 10000)  LOGGER_DEBUG("Invalid parameter (too large): width/w");//return new Error("Invalid parameter (too large): width/w");
    if(height > 10000) LOGGER_DEBUG("Invalid parameter (too large): height/h");//return new Error("Invalid parameter (too large): height/h");
    if(width <= 0)     LOGGER_DEBUG("Invalid parameter: width/w");//return new Error("Invalid parameter: width/w");
    if(height <= 0)    LOGGER_DEBUG("Invalid parameter: height/h");//return new Error("Invalid parameter: height/h")

    return 0;
  }
  HttpResponse* checkWMTS() {   
    if(layers == 0)     LOGGER_DEBUG("Missing parameter: layers/l");//return new Error("Missing parameter: layers/l");
    if(tilerow < 0)     LOGGER_DEBUG("Invalid parameter: tilerow/x");//return new Error("Invalid parameter: tilerow/x");
    if(tilecol < 0)     LOGGER_DEBUG("Invalid parameter: tilecol/y");//return new Error("Invalid parameter: tilecol/y");
    if(tilematrix == "")  LOGGER_DEBUG("Invalid parameter: tilematrix/z");//return new Error("Invalid parameter: tilematrix/z");
    return 0;
  }


  WMSRequest(int conn_fd);
  WMSRequest(char * strquery);
  ~WMSRequest();
};


#endif
