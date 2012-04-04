/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

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
    friend class CppUnitConfLoaderStyle;
    friend class CppUnitConfLoaderTMS;
    friend class CppUnitConfLoaderPyramid;
    friend class CppUnitConfLoaderServicesConf;
    friend class CppUnitConfLoaderTechnicalParam;
#endif //UNITTEST
    static bool getTechnicalParam ( std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int &nbThread, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir , char*& projEnv);
    static bool buildStylesList ( std::string styleDir, std::map<std::string,Style*> &stylesList,bool inspire );
    static bool buildTMSList ( std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList );
    static bool buildLayersList ( std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Style*> &stylesList, std::map<std::string,Layer*> &layers, bool reprojectionCapability, ServicesConf* servicesConf );
    static ServicesConf * buildServicesConf ( std::string servicesConfigFile );

private:
    static Style* parseStyle ( TiXmlDocument* doc,std::string fileName,bool inspire );
    static Style* buildStyle ( std::string fileName,bool inspire );
    static TileMatrixSet* parseTileMatrixSet ( TiXmlDocument* doc,std::string fileName );
    static TileMatrixSet* buildTileMatrixSet ( std::string fileName );
    static Pyramid* parsePyramid ( TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList );
    static Pyramid* buildPyramid ( std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList );
    static Layer * parseLayer ( TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability,ServicesConf* servicesConf );
    static Layer * buildLayer ( std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability,ServicesConf* servicesConf );
    static bool parseTechnicalParam ( TiXmlDocument* doc,std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int& nbThread, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir, char*& projEnv );
    static ServicesConf * parseServicesConf ( TiXmlDocument* doc,std::string servicesConfigFile );
};

#endif /* CONFLOADER_H_ */
