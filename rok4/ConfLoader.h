#ifndef CONFLOADER_H_
#define CONFLOADER_H_

#include <vector>
#include <string>
#include "ServicesConf.h"
#include "Layer.h"
#include "TileMatrixSet.h"

namespace  ConfLoader {
	bool getTechnicalParam(std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int &nbThread, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir);
	bool buildTMSList(std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList);
	bool buildLayersList(std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Layer*> &layers, bool reprojectionCapability,bool inspire);
	ServicesConf * buildServicesConf(std::string servicesConfigFile);
};

#endif /* CONFLOADER_H_ */
