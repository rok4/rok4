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

#ifndef LAYER_H_
#define LAYER_H_

#include <vector>
#include <string>
#include "Pyramid.h"
#include "CRS.h"
#include "Style.h"
#include "MetadataURL.h"
#include "ServicesConf.h"
#include "Interpolation.h"

struct GeographicBoundingBoxWMS {
public:
    double minx, miny, maxx, maxy;
    GeographicBoundingBoxWMS() {}
};

struct BoundingBoxWMS {
public:
    std::string srs;
    double minx, miny, maxx, maxy;
    BoundingBoxWMS() {}
};


class Layer {
private:
    std::string id;
    std::string title;
    std::string abstract;
    std::vector<std::string> keyWords;
    Pyramid* dataPyramid;
    // TODO Rajouter une metadataPyramid
    std::string defaultStyle;
    std::vector<Style*> styles;
    double minRes;
    double maxRes;
    std::vector<CRS> WMSCRSList;
    bool opaque;
    std::string authority;
    Interpolation::KernelType resampling;
    GeographicBoundingBoxWMS geographicBoundingBox;
    BoundingBoxWMS boundingBox;
    std::vector<MetadataURL> metadataURLs;

public:
    std::string getId();

    DataSource* gettile ( int x, int y, std::string tmId, DataSource* errorDataSource = NULL );
    Image* getbbox (ServicesConf& servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error );

    std::string              getAbstract()   const {
        return abstract;
    }
    std::string              getAuthority()  const {
        return authority;
    }
    std::vector<std::string> getKeyWords()   const {
        return keyWords;
    }
    double                   getMaxRes()     const {
        return maxRes;
    }
    double                   getMinRes()     const {
        return minRes;
    }
    bool                     getOpaque()     const {
        return opaque;
    }
    Pyramid*&            getDataPyramid() {
        return dataPyramid;
    }
    Interpolation::KernelType getResampling() const {
        return resampling;
    }
    std::string getDefaultStyle() const {
        return defaultStyle;
    }
    std::vector<Style*>      getStyles()     const {
        return styles;
    }
    std::string              getTitle()      const {
        return title;
    }
    std::vector<CRS> getWMSCRSList() const {
        return WMSCRSList;
    }
    GeographicBoundingBoxWMS getGeographicBoundingBox() const {
        return geographicBoundingBox;
    }
    BoundingBoxWMS           getBoundingBox() const {
        return boundingBox;
    }
    std::vector<MetadataURL> getMetadataURLs() const {
        return metadataURLs;
    }

    Layer ( std::string id, std::string title, std::string abstract,
            std::vector<std::string> & keyWords, Pyramid*& dataPyramid,
            std::vector<Style*> & styles, double minRes, double maxRes,
            std::vector<CRS> & WMSCRSList, bool opaque, std::string authority,
            Interpolation::KernelType resampling, GeographicBoundingBoxWMS geographicBoundingBox,
            BoundingBoxWMS boundingBox, std::vector<MetadataURL>& metadataURLs )
            :id ( id ), title ( title ), abstract ( abstract ), keyWords ( keyWords ),
            dataPyramid ( dataPyramid ), styles ( styles ), minRes ( minRes ),
            maxRes ( maxRes ), WMSCRSList ( WMSCRSList ), opaque ( opaque ),
            authority ( authority ),resampling ( resampling ),
            geographicBoundingBox ( geographicBoundingBox ),
            boundingBox ( boundingBox ), metadataURLs ( metadataURLs ), defaultStyle(styles.at(0)->getId()) {
    }
    ~Layer();
};

#endif /* LAYER_H_ */
