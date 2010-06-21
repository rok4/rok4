#ifndef _SENDER_
#define _SENDER_

#include "HttpResponse.h"
#include "libfcgi/fcgiapp.h"

class ResponseSender {
  private:

  public:
  int sendresponse(HttpResponse* response, int conn_fd);
  int sendresponse(HttpResponse* response, FCGX_Request* request);
};


#endif

