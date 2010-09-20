#ifndef LAYER_H_
#define LAYER_H_

#include <vector>
#include <string>
#include "Pyramid.h"

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
public:
	std::string getId();

	Tile* gettile(int x, int y, std::string tmId);
	Image* getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs = 0);

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

    Layer(std::string id, std::string title, std::string abstract, std::vector<std::string> & keyWords, std::vector<Pyramid*> & pyramids, std::vector<std::string> & styles, double minRes, double maxRes, std::vector<std::string> & WMSCRSList, bool opaque, std::string authority, std::string resampling)
    :id(id), title(title), abstract(abstract), keyWords(keyWords), pyramids(pyramids), styles(styles), minRes(minRes), maxRes(maxRes), WMSCRSList(WMSCRSList), opaque(opaque), authority(authority), resampling(resampling)
    {
    }

};

#endif /* LAYER_H_ */
