#ifndef CONFLOADER_H_
#define CONFLOADER_H_

#include <vector>
#include <string>
#include "ServicesConf.h"
#include "Layer.h"
#include "TileMatrixSet.h"

namespace  ConfLoader {
	bool getTechnicalParam(std::string serverConfigFile, std::string& logFilePrefix, int& logFilePeriod, int &nbThread, std::string &layerDir, std::string &tmsDir);
	bool buildTMSList(std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList);
	bool buildLayersList(std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Layer*> &layers);
	ServicesConf * buildServicesConf();
};

#endif /* CONFLOADER_H_ */
