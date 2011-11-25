#ifndef CONFLOADER_H_
#define CONFLOADER_H_

#include <vector>
#include <string>
#include "ServicesConf.h"
#include "Style.h"
#include "Layer.h"
#include "TileMatrixSet.h"
#include "tinyxml.h"

class  ConfLoader {
public:
	#ifdef UNITTEST
	friend class CppUnitConfLoader;
	#endif //UNITTEST
	static bool getTechnicalParam(std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int &nbThread, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir);
	static bool buildStylesList(std::string styleDir, std::map<std::string,Style*> &stylesList,bool inspire);
	static bool buildTMSList(std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList);
	static bool buildLayersList(std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Style*> &stylesList, std::map<std::string,Layer*> &layers, bool reprojectionCapability,bool inspire);
	static ServicesConf * buildServicesConf(std::string servicesConfigFile);
	
private:
	static Style* parseStyle(TiXmlDocument* doc,std::string fileName,bool inspire);
	static Style* buildStyle(std::string fileName,bool inspire);
	static TileMatrixSet* parseTileMatrixSet(TiXmlDocument* doc,std::string fileName);
	static TileMatrixSet* buildTileMatrixSet(std::string fileName);
	static Pyramid* parsePyramid(TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList);
	static Pyramid* buildPyramid(std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList);
	static Layer * parseLayer(TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability,bool inspire);
	static Layer * buildLayer(std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability,bool inspire);
	static bool parseTechnicalParam(TiXmlDocument* doc,std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int& nbThread, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir);
	static ServicesConf * parseServicesConf(TiXmlDocument* doc,std::string servicesConfigFile);
};

#endif /* CONFLOADER_H_ */
