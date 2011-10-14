#ifndef REQUEST_H_
#define REQUEST_H_

#include <map>
#include "BoundingBox.h"
#include "Data.h"
#include "CRS.h"
#include "Layer.h"
#include "ServicesConf.h"

/**
* @brief Classe request
* @brief Decodage d'une requete HTTP
*/

class Request {
private:
	void url_decode(char *src);
	bool hasParam(std::string paramName);
	std::string getParam(std::string paramName);
public:
	std::string hostName;
	std::string path;
	std::string service;
	std::string request;
	std::map<std::string, std::string> params;

	DataSource* getTileParam(ServicesConf& servicesConf,  std::map<std::string,TileMatrixSet*>& tmsList, std::map<std::string, Layer*>& layerList, Layer*& layer, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, Style* &style);
	DataStream* getMapParam(ServicesConf& servicesConf, std::map< std::string, Layer* >& layerList, Layer*& layer, BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format, Style*& style);

	Request(char* strquery, char* hostName, char* path);
	virtual ~Request();
};

#endif /* REQUEST_H_ */
