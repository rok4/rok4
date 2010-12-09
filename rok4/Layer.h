#ifndef LAYER_H_
#define LAYER_H_

#include <vector>
#include <string>
#include "Pyramid.h"
#include "CRS.h"

struct GeographicBoundingBoxWMS{
public:
        double minx, miny, maxx, maxy;
        GeographicBoundingBoxWMS() {}
};

struct BoundingBoxWMS{
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
	std::vector<Pyramid*> pyramids;
	std::vector<std::string> styles; //FIXME: la représentation d'un style n'est certainement pas un string.
	double minRes;
	double maxRes;
	std::vector<std::string> WMSCRSList; //FIXME: revoir le type des CRS (string = bof!)
	bool opaque;
	std::string authority;
	std::string resampling; //FIXME: revoir le type de resampling (plutot un enum).
	GeographicBoundingBoxWMS geographicBoundingBox;
	BoundingBoxWMS boundingBox;
	
public:
	std::string getId();

	DataSource* gettile(int x, int y, std::string tmId);
	Image* getbbox(BoundingBox<double> bbox, int width, int height, CRS dst_crs);

	std::string              getAbstract()   const { return abstract;}
	std::string              getAuthority()  const { return authority;}
	std::vector<std::string> getKeyWords()   const { return keyWords; }
	double                   getMaxRes()     const { return maxRes;}
	double                   getMinRes()     const { return minRes;}
	bool                     getOpaque()     const { return opaque;}
	std::vector<Pyramid*>    getPyramids()   const { return pyramids;}
	std::string              getResampling() const { return resampling;}
	std::vector<std::string> getStyles()     const { return styles; }
	std::string              getTitle()      const { return title; }
	std::vector<std::string> getWMSCRSList() const { return WMSCRSList; }
	GeographicBoundingBoxWMS getGeographicBoundingBox() const { return geographicBoundingBox; }
	BoundingBoxWMS           getBoundingBox() const { return boundingBox; }
	std::vector<std::string> getMimeFormats();

	Layer(std::string id, std::string title, std::string abstract, std::vector<std::string> & keyWords, std::vector<Pyramid*> & pyramids, std::vector<std::string> & styles, double minRes, double maxRes, std::vector<std::string> & WMSCRSList, bool opaque, std::string authority, std::string resampling, GeographicBoundingBoxWMS geographicBoundingBox, BoundingBoxWMS boundingBox)
	:id(id), title(title), abstract(abstract), keyWords(keyWords), pyramids(pyramids), styles(styles), minRes(minRes), maxRes(maxRes), WMSCRSList(WMSCRSList), opaque(opaque), authority(authority), resampling(resampling), geographicBoundingBox(geographicBoundingBox), boundingBox(boundingBox)
	{
	}

};

#endif /* LAYER_H_ */
