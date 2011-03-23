#ifndef TILEMATRIXSET_H_
#define TILEMATRIXSET_H_

#include <string>
#include <vector>
#include <map>
#include "TileMatrix.h"
#include "CRS.h"

class TileMatrixSet {
private:
	std::string id;
	std::string title;
	std::string abstract;
	std::vector<std::string> keyWords;
	CRS crs;
	std::map<std::string, TileMatrix> tmList;
public:
	std::map<std::string, TileMatrix>* getTmList();
	std::string getId();
	CRS getCrs() const {return crs;}
	int best_scale(double resolution_x, double resolution_y);

	TileMatrixSet(std::string id, std::string title, std::string abstract, std::vector<std::string> & keyWords, CRS& crs, std::map<std::string, TileMatrix> & tmList) :
		id(id), title(title), abstract(abstract), keyWords(keyWords), crs(crs), tmList(tmList) {};
	TileMatrixSet(const TileMatrixSet& t)
	{
		id=t.id;
		title=t.title;
		abstract=t.abstract;
		keyWords=t.keyWords;
		crs=t.crs;
		tmList=t.tmList;
	}
	~TileMatrixSet(){}
};

#endif /* TILEMATRIXSET_H_ */
