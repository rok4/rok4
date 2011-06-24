#ifndef _ROK4API
#define _ROK4API

/**
* \file Rok4Api.h
* \brief Interface de l'API de ROK4
*/

#ifdef __cplusplus

#include "Rok4Server.h"

extern "C" {

#else

#include <stdint.h>

// Types
typedef void Rok4Server;

#endif

typedef struct{
	char* queryString;
	char* hostName;
	char* scriptName;
	char* service;
	char* operationType;
} HttpRequest;

typedef struct{
        int status;
        char* type;
        char* content;
} HttpResponse;

typedef struct{
	char* filename;
	uint32_t posoff;
	uint32_t possize;
} TileRef;

// Functions

Rok4Server* rok4InitServer(const char* serverConfigFile);
HttpRequest* rok4InitRequest(const char* queryString, const char* hostName, const char* scriptName);
HttpResponse* rok4GetWMTSCapabilities(const char* hostName, const char* scriptName, Rok4Server* server);
HttpResponse* rok4GetTile(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server);
HttpResponse* rok4GetTileReferences(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server, TileRef* tileRef);
HttpResponse* rok4GetOperationNotSupportedException(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server);

void rok4KillServer(Rok4Server* server);

#ifdef __cplusplus
}
#endif

#endif
