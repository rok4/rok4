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

class LayerXML;

#ifndef LAYERXML_H
#define LAYERXML_H

#include <vector>
#include <string>
#include <tinyxml.h>

#include "Layer.h"
#include "Pyramid.h"
#include "ServicesXML.h"
#include "ServerXML.h"
#include "BoundingBox.h"
#include "MetadataURL.h"
#include "DocumentXML.h"

#include "config.h"
#include "intl.h"

class LayerXML : public DocumentXML
{
    friend class Layer;

    public:
        LayerXML(std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML );
        ~LayerXML();

        std::string getId();
        bool isOk();

    protected:
        std::string id;
        std::string title;
        std::string abstract;
        std::vector<Keyword> keyWords;
        std::string styleName;
        std::vector<Style*> styles;
        double minRes;
        double maxRes;
        std::vector<CRS> WMSCRSList;
        bool opaque;
        std::string authority;
        std::string resamplingStr;
        Interpolation::KernelType resampling;
        std::string pyramidFilePath;
        Pyramid* pyramid;
        GeographicBoundingBoxWMS geographicBoundingBox;
        BoundingBoxWMS boundingBox;
        std::vector<MetadataURL> metadataURLs;
        bool WMSauth;
        bool WMTSauth;
        bool TMSauth;
        bool times;

        bool getFeatureInfoAvailability;
        std::string getFeatureInfoType;
        std::string getFeatureInfoBaseURL;
        std::string GFIService;
        std::string GFIVersion;
        std::string GFIQueryLayers;
        std::string GFILayers;
        bool GFIForceEPSG;
    private:

        bool ok;
};

#endif // LAYERXML_H
