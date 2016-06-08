/*
 * Copyright © (2011-2013) Institut national de l'information
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

#ifndef LEVELXML_H
#define LEVELXML_H

#include <vector>
#include <string>
#include <tinyxml.h>
#include <tinystr.h>

#include "Level.h"

#include "config.h"
#include "intl.h"

class LevelXML
{
    friend class Level;

    public:
        LevelXML( TiXmlElement* levelElement, std::string fileName, std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML, std::map<std::string, TileMatrixSet*> &tmsList , std::map<std::string, Style *> stylesList, bool times);
        ~LevelXML(){
        }

        std::string getId() {
            return id;
        }

        bool isOk() { return ok; }

    protected:

        //----VARIABLE
        TileMatrix *tm;
        //std::string id;
        //std::string baseDir;
        int32_t minTileRow=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
        int32_t maxTileRow=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
        int32_t minTileCol=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
        int32_t maxTileCol=-1; // valeur conventionnelle pour indiquer que cette valeur n'est pas renseignee.
        int tilesPerWidth;
        int tilesPerHeight;
        int pathDepth;
        std::string noDataFilePath="";
        std::string noDataObjectName="";
        Context *context;
        std::string prefix = "";
        std::vector<Source*> sSources;
        bool specificLevel = false;
        bool noFile = false;
        std::string baseDir;

        // Sans stockage
        bool onDemand;

        // Avec stockage
        bool onFly;

    private:

        bool ok;
};

#endif // LEVELXML_H

