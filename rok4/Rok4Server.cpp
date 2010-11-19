#include "Image.h"

#include "Rok4Server.h"
#include <iostream>

#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"
#include <sstream>
#include <vector>
#include <map>
#include "Message.h"
#include <fstream>
#include <cstring>
#include "Logger.h"
#include "Pyramid.h"
#include "TileMatrixSet.h"
#include "Layer.h"
#include "ConfLoader.h"
#include "ServiceException.h"
#include "fcgiapp.h"
#include <proj_api.h>


/**
 * Boucle principale exécuté par chaque thread à l'écoute des requêtes de utilisateur.
 */
void* Rok4Server::thread_loop(void* arg)
{
	Rok4Server* server = (Rok4Server*) (arg);
	FCGX_Request fcgxRequest;

	if (FCGX_InitRequest(&fcgxRequest, server->sock, 0)!=0){
		LOGGER_FATAL("Le listener FCGI ne peut etre initialise");
	}

	while(true){
		int rc;

		if(FCGX_Accept_r(&fcgxRequest) < 0) {
			LOGGER_ERROR("FCGX_InitRequest renvoie le code d'erreur" << rc);
			break;
		}

		/* DEBUG: La boucle suivante permet de lister les valeurs dans fcgxRequest.envp
		char **p;
	    for (p = fcgxRequest.envp; *p; ++p) {
	    	LOGGER_DEBUG((char*)*p);
	    }*/

		/* On espère récupérer le nom du host tel qu'il est exprimé dans la requete avec HTTP_HOST.
		 * De même, on espère récupérer le path tel qu'exprimé dans la requête avec SCRIPT_NAME.
		 */
		
		Request* request = new Request(FCGX_GetParam("QUERY_STRING", fcgxRequest.envp),
		                               FCGX_GetParam("HTTP_HOST", fcgxRequest.envp),
		                               FCGX_GetParam("SCRIPT_NAME", fcgxRequest.envp));
		server->processRequest(request, fcgxRequest);
		delete request;
		FCGX_Finish_r(&fcgxRequest);
	}

	return 0;
}

/**
Construction du serveur
 */
Rok4Server::Rok4Server(int nbThread, ServicesConf servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList) :
                       sock(0), servicesConf(servicesConf), layerList(layerList), tmsList(tmsList), threads(nbThread) {
	int init=FCGX_Init();


  // Pour faire que le serveur fcgi communique sur le port xxxx utiliser FCGX_OpenSocket
	// Ceci permet de pouvoir lancer l'application sans que ce soit le serveur web qui la lancer automatiquement
	// Utile
	//  * Pour faire du profiling (grof)
	//  * Pour lancer rok4 sur plusieurs serveurs distants
	//  Voir si le choix ne peut pas être pris automatiquement en regardant comment un serveur web lance l'application fcgi.


//	sock = FCGX_OpenSocket(":1998", 50);
	buildWMSCapabilities();
	buildWMTSCapabilities();
}

/*
 * Lancement des threads du serveur
 */
void Rok4Server::run() {
	for(int i = 0; i < threads.size(); i++){
		pthread_create(&(threads[i]), NULL, Rok4Server::thread_loop, (void*) this);
	}
	for(int i = 0; i < threads.size(); i++)
		pthread_join(threads[i], NULL);
}


DataStream* Rok4Server::WMSGetCapabilities(Request* request) {
	/* concaténation des fragments invariant de capabilities en intercalant les
	 * parties variables dépendantes de la requête */
	std::string capa = wmsCapaFrag[0] + "http://" + request->hostName;
	for (int i=1; i < wmsCapaFrag.size()-1; i++){
		capa = capa + wmsCapaFrag[i] + "http://" + request->hostName + request->path;
	}
	capa = capa + wmsCapaFrag.back();

	//LOGGER_DEBUG("=> WMSGetCapabilities" << capa);
	return new MessageDataStream(capa,"text/xml");
}

DataStream* Rok4Server::WMTSGetCapabilities(Request* request) {
	/* concaténation des fragments invariant de capabilities en intercalant les
	 * parties variables dépendantes de la requête */
	std::string capa = "";
	for (int i=0; i < wmtsCapaFrag.size()-1; i++){
		capa = capa + wmtsCapaFrag[i] + "http://" + request->hostName + request->path;
	}
	capa = capa + wmtsCapaFrag.back();

	//LOGGER_DEBUG("=> WMTSGetCapabilities" << capa);
	return new MessageDataStream(capa,"text/xml");
}

/*
 * Traitement d'une requete GetMap
 * @return Un pointeur sur le flux de donnees resultant
 * @return Un message d'erreur en cas d'erreur
 */

DataStream* Rok4Server::getMap(Request* request)
{
	Layer* L;
	BoundingBox<double> bbox(0.0, 0.0, 0.0, 0.0);
	int width, height;
	CRS crs;
	std::string format;

	// Récupération des paramètres
	DataStream* errorResp = request->getMapParam(servicesConf, layerList, L, bbox, width, height, crs, format);
	if (errorResp){
		LOGGER_ERROR("Probleme dans les parametres de la requete getMap");
		return errorResp;
	}

	Image* image = L->getbbox(bbox, width, height, crs);

	if (image == 0)
		return 0;
	//FIXME : cela est-il une erreur?

	if(format=="image/png")
		return new PNGEncoder(image);
	else if(format == "image/tiff")
		return new TiffEncoderStream(image);
	else if(format == "image/jpeg")
		return new JPEGEncoder(image);
	LOGGER_ERROR("Le format "<<format<<" ne peut etre traite");
	return new SERDataStream(new ServiceException("",WMS_INVALID_FORMAT,"Le format "+format+" ne peut etre traite","wms"));
}

/*
 * Traitement d'une requete GetTile
 * @return Un pointeur sur la source de donnees de la tuile requetee
 * @return Un message d'erreur en cas d'erreur
 */

DataSource* Rok4Server::getTile(Request* request, Tile* tile)
{
	Layer* L;
	std::string tileMatrix,format;
	int tileCol,tileRow;

	// Récupération des parametres de la requete
	DataSource* errorResp = request->getTileParam(servicesConf, tmsList, layerList, L, tileMatrix, tileCol, tileRow, format);
	if (errorResp){
		LOGGER_ERROR("Probleme dans les parametres de la requete getTile");
		return errorResp;
	}

	// Récupération de la tuile
	tile=L->gettile(tileCol, tileRow, tileMatrix);

	DataSource* source=tile->getDataSource();
	size_t size;

	if (source->get_data(size)){
		// Ca particulier des tuiles TIFF : rajouter un en-tete
		if (source->gettype()=="image/tiff"){
			TiffEncoderSource* tif_source=new TiffEncoderSource(tile->getTileWidth(),tile->getTileHeight(),tile->channels,tile->getDataSource());
			source->release_data();
			tile->setDataSource(tif_source);
			return tile->getDataSource();
		}
		return source;
	}
	else{
		if (source->gettype()=="image/tiff")
	                return new TiffEncoderSource(tile->getTileWidth(),tile->getTileHeight(),tile->channels,tile->getNoDataSource());
		return tile->getNoDataSource();
	}
}


/** Traite les requêtes de type WMTS */
void Rok4Server::processWMTS(Request* request, FCGX_Request&  fcgxRequest){
	if (request->request == "getcapabilities"){
		S.sendresponse(WMTSGetCapabilities(request),&fcgxRequest);
	}else if (request->request == "gettile"){
		Tile * tile= NULL;
		S.sendresponse(getTile(request, tile), &fcgxRequest);
		// TODO: cette solution pour préserver les tuiles no-data doit pouvoir être améliore.
		// Appel au destructeur préservant la zone mémoire de la tuile no-data.
		// en cas d'erreur retournee, tile peut etre NULL
		if (tile!=NULL) delete tile;
	}else{
		S.sendresponse(new SERDataSource(new ServiceException("",OWS_OPERATION_NOT_SUPORTED,"La requete "+request->request+" n'est pas connue pour ce serveur.","wmts")),&fcgxRequest);
	}
}

/** Traite les requêtes de type WMS */
void Rok4Server::processWMS(Request* request, FCGX_Request&  fcgxRequest) {
	if (request->request == "getcapabilities"){
		S.sendresponse(WMSGetCapabilities(request),&fcgxRequest);
	}else if (request->request == "getmap"){
		S.sendresponse(getMap(request), &fcgxRequest);
	}else{
		S.sendresponse(new SERDataStream(new ServiceException("",OWS_OPERATION_NOT_SUPORTED,"La requete "+request->request+" n'est pas connue pour ce serveur.","wms")),&fcgxRequest);
	}
}

/** Separe les requetes WMS et WMTS */
void Rok4Server::processRequest(Request * request, FCGX_Request&  fcgxRequest ){
	if(request->service == "wms") {
		processWMS(request, fcgxRequest);
	}else if(request->service=="wmts") {
		processWMTS(request, fcgxRequest);
	}else{
		S.sendresponse(new SERDataSource(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"Le service "+request->service+" est inconnu pour ce serveur.","wmts")),&fcgxRequest);
	}
}

// TODO : A mettre ailleurs (dupliqué dans ReprojectedImage.cpp)
char PROJ_LIB[1024] = PROJ_LIB_PATH;
const char *pj_finder(const char *name) {
  strcpy(PROJ_LIB + 15, name);
  return PROJ_LIB;
}

int main(int argc, char** argv) {
	/* the following loop is for fcgi debugging purpose */
	int stopSleep = 0;
	while (getenv("SLEEP") != NULL && stopSleep == 0) {
		sleep(2);
	}

	/* Initialisation des Loggers */


	Accumulator* acc = new RollingFileAccumulator("/var/tmp/rok4"/*,86400,1024*/);
//	Accumulator* acc = new StreamAccumulator(std::cerr);

	Logger::setAccumulator(DEBUG, acc);
	Logger::setAccumulator(INFO , acc);
	Logger::setAccumulator(WARN , acc);
	Logger::setAccumulator(ERROR, acc);
	Logger::setAccumulator(FATAL, acc);

	LOGGER_INFO( "Lancement du serveur ROK4");

	// Initialisation de l'accès au paramétrage de la libproj
	/// Cela evite d'utiliser la variable d'environnement PROJ_LIB
	pj_set_finder( pj_finder );


	/* Chargement de la conf technique du serveur */

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
	Rok4Server W(nbThread, *servicesConf, layerList, tmsList);

	W.run();

	LOGGER_INFO( "Extinction du serveur ROK4");
}



