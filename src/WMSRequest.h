#ifndef _WMSREQUEST_
#define _WMSREQUEST_

#include "config.h"
#include "BoundingBox.h"
#include "Error.h"

class WMSRequest {
  private:
  char* buffer;

  void url_decode(char *src);
  void parseparam(char* key, char* value);
  int load(int conn_fd);

  public:  
  char* query;
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
  int tilematrix;
  char* tilematrixset;


  bool isWMTSRequest() {
    if(service) return (strcmp(service, "WMTS") == 0);
    if(tilecol >= 0 && tilerow >= 0 && tilematrix >= 0) return true;
    return false;
  }
  bool isWMSRequest() {
    if(service) return (strcmp(service, "WMS") == 0);
    if(bbox) return true;
    return false;
  }
  HttpResponse* checkWMS() {   
    if(layers == 0)    return new Error("Missing parameter: layers/l");
    if(bbox   == 0)    return new Error("Missing parameter: bbox");
    if(width > 10000)  return new Error("Invalid parameter (too large): width/w");
    if(height > 10000) return new Error("Invalid parameter (too large): height/h");
    if(width <= 0)     return new Error("Invalid parameter: width/w");
    if(height <= 0)    return new Error("Invalid parameter: height/h");
    return 0;
  }
  HttpResponse* checkWMTS() {   
    if(layers == 0)     return new Error("Missing parameter: layers/l");
    if(tilerow < 0)     return new Error("Invalid parameter: tilerow/x");
    if(tilecol < 0)     return new Error("Invalid parameter: tilecol/y");
    if(tilematrix < 0)  return new Error("Invalid parameter: tilematrix/z");
    return 0;
  }


  WMSRequest(int conn_fd);
  ~WMSRequest();
};


#endif
