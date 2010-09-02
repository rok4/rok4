#ifndef _BOUNDINGBOX_
#define _BOUNDINGBOX_

#include "Logger.h"

template<typename T> 
struct BoundingBox {
  public:
  T xmin, ymin, xmax, ymax;
  BoundingBox(T xmin, T ymin, T xmax, T ymax) : xmin(xmin), ymin(ymin), xmax(xmax), ymax(ymax) {}  

  void print() {
    LOGGER_DEBUG("BBOX = " << xmin << " " << ymin << " " << xmax << " " << ymax);
  }
};
#endif

