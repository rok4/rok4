#ifndef _SENDER_
#define _SENDER_

#include "Data.h"
#include "fcgiapp.h"

class ResponseSender {
public:
	int sendresponse(DataSource* response, FCGX_Request* request);
	int sendresponse(int status, DataSource* response, FCGX_Request* request);
	int sendresponse(DataStream* response, FCGX_Request* request);
	int sendresponse(int status, DataStream* response, FCGX_Request* request);
};


#endif

