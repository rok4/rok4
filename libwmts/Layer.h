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

	HttpResponse* gettile(int x, int y, std::string tmId);
	Image* getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs = 0);

	Layer(std::string id, std::string title, std::string abstract, std::vector<std::string> &keyWords,
		std::vector<Pyramid*> &pyramids, std::vector<std::string> &styles,
		double minRes, double maxRes, std::vector<std::string> &WMSCRSList,
		bool opaque, std::string authority, std::string resampling) :
			id(id), title(title), abstract(abstract), keyWords(keyWords),
			pyramids(pyramids), styles(styles),
			minRes(minRes),maxRes(maxRes), WMSCRSList(WMSCRSList),
			opaque(opaque), authority(authority), resampling(resampling) {}

	/*virtual ~Layer();*/
};

#endif /* LAYER_H_ */
