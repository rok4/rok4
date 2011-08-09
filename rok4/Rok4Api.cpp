/**
* \file Rok4Api.cpp
* \brief Implementation de l'API de ROK4
*/

#include "Rok4Api.h"
#include "config.h"
#include <proj_api.h>
#include "ConfLoader.h"
#include "Message.h"
#include "Request.h"
#include "RawImage.h"
#include "TiffEncoder.h"

/**
* @brief Initialisation d'une reponse a partir d'une source
* @brief Les donnees source sont copiees dans la reponse
*/

HttpResponse* initResponseFromSource(DataSource* source){
        HttpResponse* response=new HttpResponse;
        response->status=source->getHttpStatus();
        response->type=new char[source->getType().length()+1];
        strcpy(response->type,source->getType().c_str());
        size_t buffer_size;
        const uint8_t *buffer = source->getData(buffer_size);
	// TODO : tester sans copie memoire (attention, la source devrait etre supprimee plus tard)
        response->content=new uint8_t[buffer_size];
        memcpy(response->content,(uint8_t*)buffer,buffer_size);
	response->contentSize=buffer_size;
        return response;
}

/**
* @fn const char *pj_finder(const char *name)
* @brief Finder pour utiliser la fonction callback pj_set_finder de la libproj
*/

char PROJ_LIB[1024] = PROJ_LIB_PATH;
const char *pj_finder(const char *name) {
  strcpy(PROJ_LIB + 15, name);
  return PROJ_LIB;
}

/**
* @brief Initialisation du serveur ROK4
* @param serverConfigFile : nom du fichier de configuration des parametres techniques
* @return : pointeur sur le serveur ROK4, NULL en cas d'erreur (forcement fatale)
*/

Rok4Server* rok4InitServer(const char* serverConfigFile){
	// Initialisation de l'acces au parametrage de la libproj
	pj_set_finder( pj_finder );

	// Initialisation des parametres techniques
	LogOutput logOutput;
	int nbThread,logFilePeriod;
	LogLevel logLevel;
	bool reprojectionCapability;
	std::string strServerConfigFile=serverConfigFile,strLogFileprefix,strServicesConfigFile,strLayerDir,strTmsDir;
	if (!ConfLoader::getTechnicalParam(strServerConfigFile, logOutput, strLogFileprefix, logFilePeriod, logLevel, nbThread, reprojectionCapability, strServicesConfigFile, strLayerDir, strTmsDir)){
		std::cerr<<"ERREUR FATALE : Impossible d'interpreter le fichier de configuration du serveur "<<strServerConfigFile<<std::endl;
		return false;
	}

	Logger::setOutput(logOutput);
	// Initialisation du logger
	if (logOutput==ROLLING_FILE){
		RollingFileAccumulator* acc = new RollingFileAccumulator(strLogFileprefix,logFilePeriod);
		// Attention : la fonction Logger::setAccumulator n'est pas threadsafe
		for (int i=0;i<=logLevel;i++)
			Logger::setAccumulator((LogLevel)i, acc);
		std::ostream &log = LOGGER(DEBUG);
        	log.precision(8);
	        log.setf(std::ios::fixed,std::ios::floatfield);
	}
	else if (logOutput==STANDARD_OUTPUT_STREAM_FOR_ERRORS){
	}

	std::cout<<"Envoi des messages dans la sortie du logger"<< std::endl;
        LOGGER_INFO("*** DEBUT DU FONCTIONNEMENT DU LOGGER ***");

	// Construction des parametres de service
	ServicesConf* servicesConf=ConfLoader::buildServicesConf(strServicesConfigFile);
	if (servicesConf==NULL){
		LOGGER_FATAL("Impossible d'interpreter le fichier de conf "<<strServicesConfigFile);
                LOGGER_FATAL("Extinction du serveur ROK4");
		sleep(1);	// Pour laisser le temps au logger pour se vider	
		return NULL;
	}
	// Chargement des TMS
	std::map<std::string,TileMatrixSet*> tmsList;
	if (!ConfLoader::buildTMSList(strTmsDir,tmsList)){
		LOGGER_FATAL("Impossible de charger la conf des TileMatrix");
                LOGGER_FATAL("Extinction du serveur ROK4");
		sleep(1);       // Pour laisser le temps au logger pour se vider
		return NULL;
	}
	
	// Chargement des layers
	std::map<std::string, Layer*> layerList;
	if (!ConfLoader::buildLayersList(strLayerDir,tmsList,layerList,reprojectionCapability)){
		LOGGER_FATAL("Impossible de charger la conf des Layers/pyramides");
                LOGGER_FATAL("Extinction du serveur ROK4");
		sleep(1);       // Pour laisser le temps au logger pour se vider
		return NULL;
	}

	// Instanciation du serveur
	return new Rok4Server(nbThread, *servicesConf, layerList, tmsList);
}

/**
* @fn HttpRequest* rok4InitRequest(const char* queryString, const char* hostName, const char* scriptName)
* @brief Initialisation d'une requete
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @return Requete (memebres alloues ici, doivent etre desalloues ensuite)
* Requete HTTP, basee sur la terminologie des variables d'environnement Apache et completee par le type d'operation (au sens WMS/WMTS) de la requete
* Exemple :
* http://localhost/target/bin/rok4?SERVICE=WMTS&REQUEST=GetTile&tileCol=6424&tileRow=50233&tileMatrix=19&LAYER=ORTHO_RAW_IGNF_LAMB93&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0
* queryString="SERVICE=WMTS&REQUEST=GetTile&tileCol=6424&tileRow=50233&tileMatrix=19&LAYER=ORTHO_RAW_IGNF_LAMB93&STYLES=&FORMAT=image/tiff&DPI=96&TRANSPARENT=TRUE&TILEMATRIXSET=LAMB93_10cm&VERSION=1.0.0"
* hostName="localhost"
* scriptName="/target/bin/rok4"
* service="wmts" (en minuscules)
* operationType="Gettile" (en minuscules)
*/

HttpRequest* rok4InitRequest(const char* queryString, const char* hostName, const char* scriptName){
	std::string strQuery=queryString;
       	HttpRequest* request=new HttpRequest;
       	request->queryString=new char[strQuery.length()+1];
        strcpy(request->queryString,strQuery.c_str());
	request->hostName=new char[strlen(hostName)+1];
	strcpy(request->hostName,hostName);
	request->scriptName=new char[strlen(scriptName)+1];
        strcpy(request->scriptName,scriptName);
	Request* rok4Request=new Request((char*)strQuery.c_str(),(char*)hostName,(char*)scriptName);
	request->service=new char[rok4Request->service.length()+1];
        strcpy(request->service,rok4Request->service.c_str());
	request->operationType=new char[rok4Request->request.length()+1];
	strcpy(request->operationType,rok4Request->request.c_str());
	delete rok4Request;
	return request;
}

/**
* @brief Implementation de l'operation GetCapabilities pour le WMTS
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @return Reponse (allouee ici, doit etre desallouee ensuite)
*/

HttpResponse* rok4GetWMTSCapabilities(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server){
	std::string strQuery=queryString;
        Request* request=new Request((char*)strQuery.c_str(),(char*)hostName,(char*)scriptName);
	DataStream* stream=server->WMTSGetCapabilities(request);
	DataSource* source= new BufferedDataSource(*stream);
	HttpResponse* response=initResponseFromSource(/*new BufferedDataSource(*stream)*/source);
	delete request;
	delete stream;
	return response;
}

/**
* @brief Implementation de l'operation GetTile
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @return Reponse (allouee ici, doit etre desallouee ensuite)
*/

HttpResponse* rok4GetTile(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server){
        std::string strQuery=queryString;
        Request* request=new Request((char*)strQuery.c_str(),(char*)hostName,(char*)scriptName);
	DataSource* source=server->getTile(request);
	HttpResponse* response=initResponseFromSource(source);
	delete request;
        delete source;
        return response;
}

/**
* @brief Implementation de l'operation GetTile modifiee
* @brief La tuile n'est pas lue, les elements recuperes sont les references de la tuile : le fichier dans lequel elle est stockee et les positions d'enregistrement(sur 4 octets) dans ce fichier de l'index du premier octet de la tuile et de sa taille
* @param[in] queryString
* @param[in] hostName
* @param[in] scriptName
* @param[in] server : serveur
* @param[out] tileRef : reference de la tuile (la variable filename est allouee ici et doit etre desallouee ensuite)
* @return Reponse en cas d'exception, NULL sinon
*/

HttpResponse* rok4GetTileReferences(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server, TileRef* tileRef){
	// Initialisation
	std::string strQuery=queryString;

	Request* request=new Request((char*)strQuery.c_str(),(char*)hostName,(char*)scriptName);
	Layer* layer;
        std::string tmId,format;
        int x,y;

	// Analyse de la requete
        DataSource* errorResp = request->getTileParam(server->getServicesConf(), server->getTmsList(), server->getLayerList(), layer, tmId, x, y, format);
	// Exception
        if (errorResp){
                LOGGER_ERROR("Probleme dans les parametres de la requete getTile");
		HttpResponse* error=initResponseFromSource(errorResp);
		delete errorResp;
		return error;
        }

	// References de la tuile
	std::map<std::string, Level*>::iterator itLevel=layer->getDataPyramid()->getLevels().find(tmId);
	if (itLevel==layer->getDataPyramid()->getLevels().end())
                return 0;
	Level* level=layer->getDataPyramid()->getLevels().find(tmId)->second;
	int n=(y%level->getTilesPerHeight())*level->getTilesPerWidth() + (x%level->getTilesPerWidth());
	
	tileRef->posoff=2048+4*n;
	tileRef->possize=2048+4*n +level->getTilesPerWidth()*level->getTilesPerHeight()*4;

        std::string imageFilePath=level->getFilePath(x, y);
	tileRef->filename=new char[imageFilePath.length()+1];
	strcpy(tileRef->filename,imageFilePath.c_str());

	tileRef->type=new char[format.length()+1];
        strcpy(tileRef->type,format.c_str());

	tileRef->width=level->getTm().getTileW();
	tileRef->height=level->getTm().getTileH();
	tileRef->channels=level->getChannels();

	delete request;
	return 0;
}

/**
* @brief Construction d'un en-tete TIFF
*/

TiffHeader* rok4GetTiffHeader(int width, int height, int channels){
	TiffHeader* header = new TiffHeader;
	RawImage* rawImage=new RawImage(width,height,channels,0);
        TiffEncoder tiffStream(rawImage);
	tiffStream.read(header->data,128);
	return header;
}

/**
* @brief Renvoi d'une exception pour une operation non prise en charge
*/

HttpResponse* rok4GetOperationNotSupportedException(const char* queryString, const char* hostName, const char* scriptName, Rok4Server* server){

	std::string strQuery=queryString;
        Request* request=new Request((char*)strQuery.c_str(),(char*)hostName,(char*)scriptName);
        DataSource* source=new SERDataSource(new ServiceException("",OWS_OPERATION_NOT_SUPORTED,"L'operation "+request->request+" n'est pas prise en charge par ce serveur.","wmts"));
        HttpResponse* response=initResponseFromSource(source);
        delete request;
        delete source;
        return response;
}

/**
* @brief Suppression d'une requete
*/

void rok4DeleteRequest(HttpRequest* request){
	delete[] request->queryString;
	delete[] request->hostName;
	delete[] request->scriptName;
	delete[] request->service;
	delete[] request->operationType;
	delete request;
}

/**
* @brief Suppression d'une reponse
*/

void rok4DeleteResponse(HttpResponse* response){
	delete[] response->type;
	delete[] response->content;
	delete response;
}

/**
* @brief Suppression d'une reference de tuile
*/

void rok4DeleteTileRef(TileRef* tileRef){
	delete[] tileRef->filename;
        delete[] tileRef->type;
	delete tileRef;
}

/**
* @brief Suppression d'un en-tete TIFF
*/

void rok4DeleteTiffHeader(TiffHeader* header){
	delete header;
}

/**
* @brief Extinction du serveur
*/

void rok4KillServer(Rok4Server* server){
        LOGGER_INFO( "Extinction du serveur ROK4");

        std::map<std::string,TileMatrixSet*>::iterator iTms;
        for (iTms=server->getTmsList().begin();iTms!=server->getTmsList().end();iTms++)
                delete (*iTms).second;

        std::map<std::string, Layer*>::iterator iLayer;
        for (iLayer=server->getLayerList().begin();iLayer!=server->getLayerList().end();iLayer++)
                delete (*iLayer).second;

	// TODO Supprimer le logger
}

