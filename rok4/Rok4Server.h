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

#ifndef _ROK4SERVER_
#define _ROK4SERVER_

/**
* \file Rok4Server.h
* \brief Definition de la classe Rok4Server et programme principal
*/

#include "config.h"
#include "ResponseSender.h"
#include "Data.h"
#include "Request.h"
#include <pthread.h>
#include <map>
#include <vector>
#include "ServicesConf.h"
#include "Layer.h"
#include "TileMatrixSet.h"
#include "fcgiapp.h"

/**
* \class Rok4Server
*
*/

class Rok4Server {
private:
    std::vector<pthread_t> threads;
    ResponseSender S;

    int sock;

    ServicesConf servicesConf;
    std::map<std::string, Layer*> layerList;
    std::map<std::string, TileMatrixSet*> tmsList;
    std::map<std::string, Style*> styleList;
    std::vector<std::string> wmsCapaFrag;  /// liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.
    std::vector<std::string> wmtsCapaFrag; /// liste des fragments invariants de capabilities prets à être concaténés avec les infos de la requête.

    char* projEnv;
    
    static void* thread_loop ( void* arg );

    int GetDecimalPlaces ( double number );
    void buildWMSCapabilities();
    void buildWMTSCapabilities();
    
    bool hasParam ( std::map<std::string, std::string>& option, std::string paramName );
    std::string getParam ( std::map<std::string, std::string>& option, std::string paramName );
    
    DataStream* getMap ( Request* request );
    DataStream* WMSGetCapabilities ( Request* request );
    void        processWMS ( Request *request, FCGX_Request&  fcgxRequest );
    void        processWMTS ( Request *request, FCGX_Request&  fcgxRequest );
    void        processRequest ( Request *request, FCGX_Request&  fcgxRequest );

public:
    
    ServicesConf& getServicesConf() {
        return servicesConf;
    }
    std::map<std::string, Layer*>& getLayerList() {
        return layerList;
    }
    std::map<std::string, TileMatrixSet*>& getTmsList() {
        return tmsList;
    }
    std::map<std::string, Style*>& getStyleList() {
        return styleList;
    }
    std::vector<std::string>& getWmsCapaFrag() {
        return wmsCapaFrag;
    }
    std::vector<std::string>& getWmtsCapaFrag() {
        return wmtsCapaFrag;
    }
    DataSource* getTile ( Request* request );
    DataStream* WMTSGetCapabilities ( Request* request );

    char* getProjEnv() { return projEnv;}
    void run();
    Rok4Server ( int nbThread, ServicesConf& servicesConf, std::map<std::string,Layer*> &layerList,
                 std::map<std::string,TileMatrixSet*> &tmsList, std::map<std::string,Style*> &styleList, char*& projEnv );
    ~Rok4Server ();

};

#endif

