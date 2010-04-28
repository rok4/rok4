#ifndef _SENDER_
#define _SENDER_

#include "HttpResponse.h"

class ResponseSender {
  private:

  public:
  int sendresponse(HttpResponse* response, int conn_fd);
};


#endif

