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
    void url_decode ( char *src );
    bool hasParam ( std::string paramName );
    std::string getParam ( std::string paramName );

public:
    std::string hostName;
    std::string path;
    std::string service;
    std::string request;
    std::string scheme;
    std::map<std::string, std::string> params;
    DataSource* getTileParam ( ServicesConf& servicesConf,  std::map<std::string,TileMatrixSet*>& tmsList, std::map<std::string, Layer*>& layerList, Layer*& layer, std::string &tileMatrix, int &tileCol, int &tileRow, std::string  &format, Style* &style );
    DataStream* getMapParam ( ServicesConf& servicesConf, std::map< std::string, Layer* >& layerList, Layer*& layer, BoundingBox< double >& bbox, int& width, int& height, CRS& crs, std::string& format, Style*& style,std::map <std::string, std::string >& format_option );
    
    DataStream* getCapWMSParam ( ServicesConf& servicesConf, std::string& version);
    DataStream* getCapWMTSParam ( ServicesConf& servicesConf, std::string& version);

    Request ( char* strquery, char* hostName, char* path, char* https );
    Request ( char* strquery, char* hostName, char* path, char* https, std::string postContent );
    virtual ~Request();
};

#endif /* REQUEST_H_ */
