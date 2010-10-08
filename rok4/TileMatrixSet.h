#ifndef TILEMATRIXSET_H_
#define TILEMATRIXSET_H_

#include <string>
#include <vector>
#include <map>
#include "TileMatrix.h"
#include "Logger.h"


class TileMatrixSet {
private:
	std::string id;
	std::string title;
	std::string abstract;
	std::vector<std::string> keyWords;
	std::string crs;
	std::map<std::string, TileMatrix> tmList;
public:
	// FIXME: comment interdire à l'utilisateur de modifier ce map?
	std::map<std::string, TileMatrix> getTmList();

	std::string getId();

	std::string getCrs() const
	{return crs;}


	int best_scale(double resolution_x, double resolution_y);

	TileMatrixSet(std::string id, std::string title, std::string abstract, std::vector<std::string> & keyWords, std::string crs, std::map<std::string, TileMatrix> & tmList) :
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

};

#endif /* TILEMATRIXSET_H_ */
