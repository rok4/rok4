#include "Image.h"

#include "Rok4Server.h"
#include <iostream>

#include "TiffEncoder.h"
#include "ColorizePNGEncoder.h"
#include "JPEGEncoder.h"
#include "BilEncoder.h"
#include <sstream>
#include <vector>
#include <map>
#include "Message.h"
#include <fstream>
#include <cstring>
#include "Logger.h"
#include "TileMatrixSet.h"
#include "Layer.h"
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
		if((rc=FCGX_Accept_r(&fcgxRequest)) < 0) {
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
* @brief Construction du serveur
*/
Rok4Server::Rok4Server(int nbThread, ServicesConf& servicesConf, std::map<std::string,Layer*> &layerList, std::map<std::string,TileMatrixSet*> &tmsList) :
                       sock(0), servicesConf(servicesConf), layerList(layerList), tmsList(tmsList), threads(nbThread) {

	buildWMSCapabilities();
	buildWMTSCapabilities();
}

/*
 * Lancement des threads du serveur
 */
void Rok4Server::run() {
	 int init=FCGX_Init();

// Pour faire que le serveur fcgi communique sur le port xxxx utiliser FCGX_OpenSocket
        // Ceci permet de pouvoir lancer l'application sans que ce soit le serveur web qui la lancer automatiquement
        // Utile
        //  * Pour faire du profiling (grof)
        //  * Pour lancer rok4 sur plusieurs serveurs distants
        //  Voir si le choix ne peut pas être pris automatiquement en regardant comment un serveur web lance l'application fcgi.

        // A décommenter pour utiliser valgrind
        // Ex : valgrind --leak-check=full --show-reachable=yes rok4 2> leak.txt
        // Ensuite redemarrer le serveur Apache configure correctement. Attention attendre suffisamment longtemps l'initialisation de valgrind

        // sock = FCGX_OpenSocket(":1990", 50);

        // Cf. aussi spawn-fcgi qui est un spawner pour serveur fcgi et qui permet de specifier un port d ecoute
        // Exemple : while (true) ; do spawn-fcgi -n -p 9000 -- ./rok4 -f ../config/server-nginx.conf ; done


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
		capa = capa + wmsCapaFrag[i] + "http://" + request->hostName + request->path + "?";
	}
	capa = capa + wmsCapaFrag.back();

	return new MessageDataStream(capa,"text/xml");
}

DataStream* Rok4Server::WMTSGetCapabilities(Request* request) {
	/* concaténation des fragments invariant de capabilities en intercalant les
	 * parties variables dépendantes de la requête */
	std::string capa = "";
	for (int i=0; i < wmtsCapaFrag.size()-1; i++){
		capa = capa + wmtsCapaFrag[i] + "http://" + request->hostName + request->path +"?";
	}
	capa = capa + wmtsCapaFrag.back();

	return new MessageDataStream(capa,"application/xml");
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
	std::string format, styles;

	// Récupération des paramètres
	DataStream* errorResp = request->getMapParam(servicesConf, layerList, L, bbox, width, height, crs, format,styles);
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
		return new ColorizePNGEncoder(image,true);
	else if(format == "image/tiff")
		return new TiffEncoder(image);
	else if(format == "image/jpeg")
		return new JPEGEncoder(image);
	else if(format == "image/x-bil;bits=32")
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
	std::string tileMatrix,format,style;
	int tileCol,tileRow;

	// Récupération des parametres de la requete
	DataSource* errorResp = request->getTileParam(servicesConf, tmsList, layerList, L, tileMatrix, tileCol, tileRow, format,style);

	if (errorResp){
		LOGGER_ERROR("Probleme dans les parametres de la requete getTile");
		return errorResp;
	}

	return L->gettile(tileCol, tileRow, tileMatrix);
}

/** Traite les requêtes de type WMTS */
void Rok4Server::processWMTS(Request* request, FCGX_Request&  fcgxRequest){
	if (request->request == "getcapabilities"){
		S.sendresponse(WMTSGetCapabilities(request),&fcgxRequest);
	}else if (request->request == "gettile"){
		S.sendresponse(getTile(request), &fcgxRequest);
	}else{
		S.sendresponse(new SERDataSource(new ServiceException("",OWS_OPERATION_NOT_SUPORTED,"L'operation "+request->request+" n'est pas prise en charge par ce serveur.","wmts")),&fcgxRequest);
	}
}

/** Traite les requêtes de type WMS */
void Rok4Server::processWMS(Request* request, FCGX_Request&  fcgxRequest) {
	if (request->request == "getcapabilities"){
		S.sendresponse(WMSGetCapabilities(request),&fcgxRequest);
	}else if (request->request == "getmap"){
		S.sendresponse(getMap(request), &fcgxRequest);
	}else{
		S.sendresponse(new SERDataStream(new ServiceException("",OWS_OPERATION_NOT_SUPORTED,"L'operation "+request->request+" n'est pas prise en charge par ce serveur.","wms")),&fcgxRequest);
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
