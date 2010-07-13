#ifndef CONFLOADER_H_
#define CONFLOADER_H_

#include <vector>
#include <cstring>
#include "Layer.h"
#include "TileMatrixSet.h"

namespace  ConfLoader {
	bool getTechnicalParam(int &nbThread, std::string &layerDir, std::string &tmsDir);
	bool buildTMSList(std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList);
	bool buildLayersList(std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Layer*> &layers);
};

#endif /* CONFLOADER_H_ */
