#ifndef TILEMATRIXSET_H_
#define TILEMATRIXSET_H_

#include <cstring>
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
public:
	std::map<std::string, TileMatrix> tmList; // Ca ne sert à rien de faire un get qui renvoie une ref
	                                          // FIXME: comment interdire à l'utilisateur de modifier ce map?
	std::string getId();

	int best_scale(double resolution_x, double resolution_y);

	TileMatrixSet(std::string id, std::string title, std::string abstract, std::vector<std::string> & keyWords, std::string crs, std::map<std::string, TileMatrix> & tmList) :
		id(id), title(title), abstract(abstract), keyWords(keyWords), crs(crs), tmList(tmList) {};

};

#endif /* TILEMATRIXSET_H_ */
