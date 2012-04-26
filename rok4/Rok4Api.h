/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

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

    typedef struct {
        char* queryString;
        char* hostName;
        char* scriptName;
        char* service;
        char* operationType;
        bool  noDataAsHttpStatus;
    } HttpRequest;

    typedef struct {
        int status;
        char* type;
        uint8_t* content;
        size_t contentSize;
    } HttpResponse;

    typedef struct {
        char* filename;
        uint32_t posoff;
        uint32_t possize;
        char* type;
        int width;
        int height;
        int channels;
	char* format;
    } TileRef;

    typedef struct {
        uint8_t data[128];
    } TiffHeader;

    typedef struct {
        size_t size;
        uint8_t* data;
    } PngPaletteHeader;

    typedef struct {
        size_t size;
        uint8_t* data;
    } TilePalette;

// Functions

    Rok4Server* rok4InitServer ( const char* serverConfigFile );
    HttpRequest* rok4InitRequest ( const char* queryString, const char* hostName, const char* scriptName, const char* https );
    HttpResponse* rok4GetWMTSCapabilities ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server );

    HttpResponse* rok4GetTile ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server );
    HttpResponse* rok4GetTileReferences ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server, TileRef* tileRef, TilePalette* palette );
    HttpResponse* rok4GetNoDataTileReferences ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server, TileRef* tileRef, TilePalette* palette );

// DEPRECATED
    TiffHeader* rok4GetTiffHeader ( int width, int height, int channels );

    TiffHeader* rok4GetTiffHeaderFormat(int width, int height, int channels, char* format, uint32_t possize);
    PngPaletteHeader* rok4GetPngPaletteHeader ( int width, int height, TilePalette* palette );
    HttpResponse* rok4GetOperationNotSupportedException ( const char* queryString, const char* hostName, const char* scriptName,const char* https, Rok4Server* server );
    void rok4DeleteRequest ( HttpRequest* request );
    void rok4DeleteResponse ( HttpResponse* response );
    void rok4FlushTileRef ( TileRef* tileRef );
    void rok4DeleteTiffHeader ( TiffHeader* header );
    void rok4DeletePngPaletteHeader ( PngPaletteHeader* header );
    void rok4DeleteTilePalette ( TilePalette* palette );

    void rok4KillServer ( Rok4Server* server );

#ifdef __cplusplus
}
#endif

#endif
