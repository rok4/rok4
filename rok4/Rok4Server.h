#ifndef _ROK4SERVER_
#define _ROK4SERVER_

#include "config.h"
#include "ResponseSender.h"
#include "Data.h"
#include "Request.h"
#include <pthread.h>
#include <map>
#include "ServicesConf.h"
#include "Layer.h"
#include "TileMatrixSet.h"
#include "fcgiapp.h"


class Rok4Server {
private:
	int nbThread;
	pthread_t Thread[128]; /* tableau des threads => 128 thread max! *//*FIXME mettre un conteneur sans limite */
	ResponseSender S;

	int sock;

	ServicesConf &servicesConf;
	std::map<std::string, Layer*> layerList;
	std::map<std::string,TileMatrixSet*> tmsList;

	static void* thread_loop(void* arg);

	void buildWMSCapabilities();
	void buildWMTSCapabilities();

	DataStream* getMap(Request* request);
	DataSource* getTile(Request* request, Tile* tile);
	DataStream* WMSGetCapabilities(Request* request);
	DataSource* WMTSGetCapabilities(Request* request);
	DataStream* processWMS (Request *request);
	DataSource* processWMTS(Request *requesti, Tile* tile);

public:
	std::string WMSCapabilities;
	std::string WMTSCapabilities;

	void run();
	Rok4Server(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList);

};

#endif

