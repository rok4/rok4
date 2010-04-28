#ifndef _BOUNDINGBOX_
#define _BOUNDINGBOX_

#include "config.h"
#include "Logger.h"

template<typename T> 
class BoundingBox {
  public:
  T xmin, ymin, xmax, ymax;
  BoundingBox(T xmin, T ymin, T xmax, T ymax) : xmin(xmin), ymin(ymin), xmax(xmax), ymax(ymax) {
//LOGGER(DEBUG) << "xmin=" << xmin << " xmax=" << xmax << std::endl;
    assert(xmin <= xmax);
    assert(ymin <= ymax);    
  }
};
#endif

