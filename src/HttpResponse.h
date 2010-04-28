#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include <stdint.h>
#include <unistd.h>
#include <cstring>

#include "Logger.h"

/**
 *D
 */
class HttpResponse {
  public:
  /** D */
  const char* type;

  /** D */
  HttpResponse(const char* type) : type(type) {}

  /** D */
  virtual ~HttpResponse() {}

  /** D */
  virtual const uint8_t* get_data(size_t &size) {size = 0; return 0;};

  /** D */
  virtual size_t getdata(uint8_t *buffer, size_t size) = 0;
};



/**
 *D
 */
class StaticHttpResponse : public HttpResponse {
  private:
  const uint8_t* data;
  const size_t size;
  size_t pos;

  public:
  /** D */
  StaticHttpResponse(const char* type, const uint8_t* data, size_t size) : HttpResponse(type), data(data), size(size), pos(0) {
      LOGGER(DEBUG) << " StaticHttpResponse " << size << " " << (void*) data << std::endl;
    }

  /** D */
  ~StaticHttpResponse() {if(data) delete[] data;}

  /** D */
  const uint8_t* get_data(size_t &sz) {    
    sz = size;
    LOGGER(DEBUG) << " StaticHttpResponse " << size << " " << (void*) data << std::endl;
    return data;
  }

  /** D */
  size_t getdata(uint8_t *buffer, size_t sz) {    
      LOGGER(DEBUG) << " StaticHttpResponse geline " << sz << std::endl;  
    if(sz > size - pos) sz = size - pos;
    memcpy(buffer, data + pos, sz);
    pos += sz;
    return sz;   
  };




};




#endif
