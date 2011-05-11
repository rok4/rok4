#include "Image.h"

#include "Rok4Server.h"
#include <iostream>

#include "TiffEncoder.h"
#include "PNGEncoder.h"
#include "JPEGEncoder.h"
#include "BilEncoder.h"
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
		FCGX_Free(&fcgxRequest,1);
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

	// A décommenter pour utiliser valgrind
	// Ex : valgrind --leak-check=full --show-reachable=yes rok4 2> leak.txt
	// Ensuite redemmarrer le serveur Apache configure correctement. Attention attendre suffisamment longtemps l'initialisation de valgrind
	
	// sock = FCGX_OpenSocket(":1990", 50);

	// Cf. aussi spawn-fcgi qui est un spawner pour serveur fcgi et qui permet de specifier un port d ecoute
	// Exemple : while (true) ; do spawn-fcgi -n -p 9000 -- ./rok4 -f ../config/server-nginx.conf ; done
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
	
	int error;
	Image* image = L->getbbox(bbox, width, height, crs, error);

	if (image == 0){
		if (error==1)
			return new SERDataStream(new ServiceException("",OWS_INVALID_PARAMETER_VALUE,"bbox invalide","wms"));
		else
			return new SERDataStream(new ServiceException("",OWS_NOAPPLICABLE_CODE,"Impossible de repondre a la requete","wms"));
	}
	if(format=="image/png")
		return new PNGEncoder(image);
	else if(format == "image/tiff")
		return new TiffEncoder(image);
	else if(format == "image/jpeg")
		return new JPEGEncoder(image);
	else if(format == "image/bil")
                return new BilEncoder(image);
	LOGGER_ERROR("Le format "<<format<<" ne peut etre traite");
	return new SERDataStream(new ServiceException("",WMS_INVALID_FORMAT,"Le format "+format+" ne peut etre traite","wms"));
}

/*
 * Traitement d'une requete GetTile
 * @return Un pointeur sur la source de donnees de la tuile requetee
 * @return Un message d'erreur en cas d'erreur
 */

DataSource* Rok4Server::getTile(Request* request)
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

	return  L->gettile(tileCol, tileRow, tileMatrix);
}

/** Traite les requêtes de type WMTS */
void Rok4Server::processWMTS(Request* request, FCGX_Request&  fcgxRequest){
	if (request->request == "getcapabilities"){
		S.sendresponse(WMTSGetCapabilities(request),&fcgxRequest);
	}else if (request->request == "gettile"){
		S.sendresponse(getTile(request), &fcgxRequest);
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

/* Usage de la ligne de commande */

void usage() {
        std::cerr<<" Usage : rok4 [-f server_config_file]"<<std::endl;
}

/*
* main
* @return -1 en cas d'erreur
*/

int main(int argc, char** argv) {
	
	/* the following loop is for fcgi debugging purpose */
	int stopSleep = 0;
	while (getenv("SLEEP") != NULL && stopSleep == 0) {
		sleep(2);
	}

	// Lecture des arguments de la ligne de commande
	std::string serverConfigFile=DEFAULT_SERVER_CONF_PATH;
	for(int i = 1; i < argc; i++) {
                if(argv[i][0] == '-') {
                        switch(argv[i][1]) {
                        case 'f': // fichier de configuration du serveur
                                if(i++ >= argc){
					std::cerr<<"Erreur sur l'option -f"<<std::endl;
					usage();
					return -1;
				}
                                serverConfigFile.assign(argv[i]);
                                break;
			default:
				usage();
				return -1;
			}
		}
	}

	std::cout<< "Lancement du serveur rok4..."<<std::endl;

	// Initialisation de l'accès au paramétrage de la libproj
	// Cela evite d'utiliser la variable d'environnement PROJ_LIB
	pj_set_finder( pj_finder );

	/* Chargement de la conf technique du serveur */

	// Chargement de la conf technique
	std::string logFileprefix;
	int logFilePeriod;
	int nbThread;
	std::string layerDir;
	std::string tmsDir;
	if(!ConfLoader::getTechnicalParam(serverConfigFile, logFileprefix, logFilePeriod, nbThread, layerDir, tmsDir)){
		std::cerr<<"ERREUR FATALE : Impossible d'interpréter le fichier de configuration du serveur "<<serverConfigFile<<std::endl;
		std::cerr<<"Extinction du serveur ROK4"<<std::endl;
		// On attend 10s pour eviter que le serveur web ne le relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		// sleep(10);
		return -1;
	}


        // Initialisation des loggers
        RollingFileAccumulator* acc = new RollingFileAccumulator(logFileprefix,logFilePeriod);
        Logger::setAccumulator(DEBUG, acc);
        Logger::setAccumulator(INFO , acc);
        Logger::setAccumulator(WARN , acc);
        Logger::setAccumulator(ERROR, acc);
        Logger::setAccumulator(FATAL, acc);
	std::cout<<"Envoi des messages dans la sortie du logger"<< std::endl;
	LOGGER_INFO("*** DEBUT DU FONCTIONNEMENT DU LOGGER ***");

	// Chargement de la conf services
	ServicesConf* servicesConf=ConfLoader::buildServicesConf();
	if(!servicesConf){
		LOGGER_FATAL("Impossible d'interpréter le fichier de conf services.conf");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web ne le relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		// sleep(10);
		return -1;
	}


	// Chargement des TMS
	std::map<std::string,TileMatrixSet*> tmsList;
	if(!ConfLoader::buildTMSList(tmsDir,tmsList)){
		LOGGER_FATAL("Impossible de charger la conf des TileMatrix");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web ne le relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
      	   	// sleep(10);
		return -1;
	}

	// Chargement des Layers
	std::map<std::string, Layer*> layerList;
	if(!ConfLoader::buildLayersList(layerDir,tmsList,layerList)){
		LOGGER_FATAL("Impossible de charger la conf des Layers/pyramides");
		LOGGER_FATAL("Extinction du serveur ROK4");
		// On attend 10s pour eviter que le serveur web nele relance tout de suite
		// avec les mêmes conséquences et sature les logs trop rapidement.
		/* DEBUG
      	    sleep(10);*/
		return -1;
	}

	// Construction du serveur.
	Rok4Server W(nbThread, *servicesConf, layerList, tmsList);

	W.run();

	// Extinction du serveur
	LOGGER_INFO( "Extinction du serveur ROK4");

	delete servicesConf;

	std::map<std::string,TileMatrixSet*>::iterator iTms;
        for (iTms=tmsList.begin();iTms!=tmsList.end();iTms++)
		delete (*iTms).second;

	std::map<std::string, Layer*>::iterator iLayer;
	for (iLayer=layerList.begin();iLayer!=layerList.end();iLayer++)
		delete (*iLayer).second;

	delete acc;
}
