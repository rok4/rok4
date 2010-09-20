#ifndef REQUEST_H_
#define REQUEST_H_

#include <map>
#include "BoundingBox.h"
#include "Data.h"

class Request {
private:
	  void url_decode(char *src);
	  std::string getParam(std::string paramName);
public:
	std::string server;
	std::string service;
	std::string request;
	std::map<std::string, std::string> params;

	DataSource* getTileParam(std::string &layer, std::string  &tileMatrixSet, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format);
	DataStream* getMapParam(std::string &layers, BoundingBox<double> &bbox, int &width, int &height, std::string &crs, std::string &format);

	Request(char* strquery, char* serverName);
	virtual ~Request();
};

#endif /* REQUEST_H_ */
