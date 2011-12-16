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
        uint8_t* content;
	size_t contentSize;
} HttpResponse;

typedef struct{
	char* filename;
	uint32_t posoff;
	uint32_t possize;
	char* type;
	int width;
	int height;
	int channels;
} TileRef;

typedef struct{
	uint8_t data[128];
} TiffHeader;

typedef struct{
	size_t size;
	uint8_t* data;
} PngPaletteHeader;

typedef struct{
	size_t size;
	uint8_t* data;
} TilePalette;

// Functions

Rok4Server* rok4InitServer(const char* serverConfigFile);
HttpRequest* rok4InitRequest(const char* queryString, const char* hostName, const char* scriptName, const char* https);
HttpResponse* rok4GetWMTSCapabilities(const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server);

HttpResponse* rok4GetTile(const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server);
HttpResponse* rok4GetTileReferences(const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server, TileRef* tileRef, TilePalette* palette);
HttpResponse* rok4GetNoDataTileReferences(const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server, TileRef* tileRef, TilePalette* palette);

TiffHeader* rok4GetTiffHeader(int width, int height, int channels);
PngPaletteHeader* rok4GetPngPaletteHeader(int width, int height, TilePalette* palette);
HttpResponse* rok4GetOperationNotSupportedException(const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server);
void rok4DeleteRequest(HttpRequest* request);
void rok4DeleteResponse(HttpResponse* response);
void rok4FlushTileRef(TileRef* tileRef);
void rok4DeleteTiffHeader(TiffHeader* header);
void rok4DeletePngPaletteHeader(PngPaletteHeader* header);
void rok4DeleteTilePalette(TilePalette* palette);

void rok4KillServer(Rok4Server* server);

#ifdef __cplusplus
}
#endif

#endif
