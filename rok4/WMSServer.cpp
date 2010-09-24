#include "Image.h"

#include "WMSServer.h"
#include <iostream>

#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"

#include <sstream>
#include <vector>
#include <map>
#include "Message.h"

#include <fstream>
#include <signal.h>
#include <cstring>
#include "Logger.h"
#include "Pyramid.h"
#include "TileMatrixSet.h"
#include "Layer.h"
#include "ConfLoader.h"

#include "fcgiapp.h"


/**
 * Boucle principale exécuté par chaque thread à l'écoute des requêtes de utilisateur.
 */
void* WMSServer::thread_loop(void* arg)
{
	WMSServer* server = (WMSServer*) (arg);   

	int rc;
	FCGX_Request fcgxRequest;

	//    int sock = 0;
	// Pour faire que le serveur fcgi communique sur le port xxxx utiliser FCGX_OpenSocket
	// Ceci permet de pouvoir lancer l'application sans que ce soit le serveur web qui la lancer automatiquement
	// Utile
	//  * Pour faire du profiling (grof)
	//  * Pour lancer rok4 sur plusieurs serveurs distants
	//  Voir si le choix ne peut pas être pris automatiquement en regardant comment un serveur web lance l'application fcgi.

	if (FCGX_InitRequest(&fcgxRequest, server->sock, 0)!=0){
		LOGGER_FATAL("Le listener FCGI ne peut etre initialise");
	}

	while(true){
		rc = FCGX_Accept_r(&fcgxRequest);

		if (rc < 0){
			LOGGER_ERROR("FCGX_InitRequest renvoie le code d'erreur" << rc);
			break;
		}
		Request* request = new Request(FCGX_GetParam("QUERY_STRING", fcgxRequest.envp), FCGX_GetParam("SERVER_NAME", fcgxRequest.envp));
		if(request->service == "wms")
			server->S.sendresponse(server->processWMS(request),&fcgxRequest);
		else if(request->service=="wmts")
		{	
			Tile* tile;
			server->S.sendresponse(server->processWMTS(request,tile),&fcgxRequest);
			delete tile;
		}
		else
			server->S.sendresponse(new MessageDataSource("Invalid request","text/plain"),&fcgxRequest);
		delete request;
		FCGX_Finish_r(&fcgxRequest);
	}

	return 0;
}

/**
Construction du serveur
 */
WMSServer::WMSServer(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList) :
		            		   nbThread(nbThread), sock(0), servicesConf(servicesConf), layerList(layerList), tmsList(tmsList) {
	int init=FCGX_Init();
	//  sock = FCGX_OpenSocket(":1998", 50);
	buildWMSCapabilities();
	buildWMTSCapabilities();
}

/*
 * Lancement des threads du serveur
 */
void WMSServer::run() {
	for(int i = 0; i < nbThread; i++)
		pthread_create(&Thread[i], NULL, WMSServer::thread_loop, (void*) this);
	for(int i = 0; i < nbThread; i++)
		pthread_join(Thread[i], NULL);
}


DataStream* WMSServer::WMSGetCapabilities(Request* request) {
	/*FIXME: remplacer le nom du serveur dans les adresses des capabilities*/
	LOGGER_DEBUG("=> WMSGetCapabilities" << WMSCapabilities);
	return new MessageDataStream(WMSCapabilities,"text/xml");
}

DataSource* WMSServer::WMTSGetCapabilities(Request* request) {
	// TODO à faire
	return new MessageDataSource("Not yet implemented!","text/plain");
}

/*
 * Traitement d'une requete GetMap
 * @return Un pointeur sur le flux de donnees resultant
 * @return Un message d'erreur en cas d'erreur
 */

DataStream* WMSServer::getMap(Request* request)
{
	std::string layer;
	BoundingBox<double> bbox(0.0, 0.0, 0.0, 0.0);
	int width;
	int height;
	std::string crs;
	std::string format;
	LOGGER_DEBUG( "wmsserver:getMap layers : " << layer );

	// Récupération des paramètres
	DataStream* errorResp = request->getMapParam(layer, bbox, width, height, crs, format);
	if (errorResp){
		LOGGER_ERROR("probleme dans les parametres de la requete getMap");
		return errorResp;
	}

	// Vérification des paramètres
	std::map<std::string, Layer*>::iterator it = layerList.find(layer);
	if(it == layerList.end()){
		LOGGER_ERROR("le layer "<<layer<<" est inconnu.");
		return new MessageDataStream("Layer "+layer+" inconnu ","text/plain");
	}
	Layer* L = it->second;

	Image* image = L->getbbox(bbox, width, height, crs.c_str());

	if (image == 0)
		return 0;
	//FIXME : cela est-il une erreur?

	if(format=="image/png")
		return new PNGEncoder(image);
	else if(format == "image/tiff")
		return new TiffEncoder(image);
	else if(format == "image/jpeg")
		return new JPEGEncoder(image);
	return new PNGEncoder(image);
}

/*
 * Traitement d'une requete GetTile
 * @return Un pointeur sur la source de donnees de la tuile requetee
 * @return Un message d'erreur en cas d'erreur
 */

DataSource* WMSServer::getTile(Request* request, Tile* tile)
{
	std::string layer,tileMatrixSet,tileMatrix,format;
	int tileCol,tileRow;

	// Récupération des paramètres de la requete
	DataSource* errorResp = request->getTileParam(layer, tileMatrixSet,tileMatrix, tileCol, tileRow, format);
	if (errorResp)
		return errorResp;
	// Vérification de l'adéquation entre les paramètres et la conf du serveur (ancien checkWMS)
	// Existence du layer
	//FIXME : pourquoi ce controle ici?
	std::map<std::string, Layer*>::iterator it = layerList.find(layer);
	if(it == layerList.end())
		return new MessageDataSource("Unknown layer","text/plain");
	Layer* L = it->second;
	// l'existence du TMS et TM est controlée dans gettile()
	// TODO: vérifier que c'est effectivement le cas

	// Récupération de la tuile
	tile=L->gettile(tileCol, tileRow, tileMatrix);
	DataSource* source=tile->getDataSource();
	size_t size;
	if (source->get_data(size))
		return source;
	else
		return tile->getNoDataSource();
}


/** tTraite les requêtes de type WMTS */
DataSource* WMSServer::processWMTS(Request* request, Tile* tile)
{
	if (request->request == "getcapabilities")
		return(WMTSGetCapabilities(request));
	else if (request->request == "gettile")
		return (getTile(request, tile));
	else
		return new MessageDataSource("Invalid request","text/plain");
}

/** Traite les requêtes de type WMS */
DataStream* WMSServer::processWMS(Request* request) {
	if (request->request == "getcapabilities")
		return(WMSGetCapabilities(request));
	else if (request->request == "getmap")
		return (getMap(request));
	else
		return new MessageDataStream("Invalid request","text/plain");
}

int main(int argc, char** argv) {

	/* the following loop is for fcgi debugging purpose */
	int stopSleep = 0;
	while (getenv("SLEEP") != NULL && stopSleep == 0) {
		sleep(2);
	}

	/* Chargement de la conf technique du serveur */
	Logger::configure("/var/tmp/rok4cxx.log");
	LOGGER_INFO( "Lancement du serveur ROK4");

	//chargement de la conf technique
	int nbThread;
	std::string layerDir;
	std::string tmsDir;
	if(!ConfLoader::getTechnicalParam(nbThread, layerDir, tmsDir)){
		LOGGER_FATAL("Impossible d'interpréter le fichier de conf server.conf");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web ne le relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		//sleep(10);
		return 1;
	}

	//chargement de la conf services
	ServicesConf *servicesConf=ConfLoader::buildServicesConf();
	if(!servicesConf){
		LOGGER_FATAL("Impossible d'interpréter le fichier de conf services.conf");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web ne le relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		//sleep(10);
		return 1;
	}


	//chargement des TMS
	std::map<std::string,TileMatrixSet*> tmsList;
	if(!ConfLoader::buildTMSList(tmsDir,tmsList)){
		LOGGER_FATAL("Impossible de charger la conf des TileMatrix");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web ne le relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		/* DEBUG
      	   sleep(10);*/
		return 1;
	}

	//chargement des Layers
	std::map<std::string, Layer*> layerList;
	if(!ConfLoader::buildLayersList(layerDir,tmsList,layerList)){
		LOGGER_FATAL("Impossible de charger la conf des Layers/pyramides");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web nele relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		/* DEBUG
      	    sleep(10);*/
		return 1;
	}

	//construction du serveur.
	WMSServer W(nbThread, *servicesConf, layerList, tmsList);

	W.run();

	LOGGER_INFO( "Extinction du serveur ROK4");
}



