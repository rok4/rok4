#ifndef TILEMATRIX_H
#define TILEMATRIX_H

#include <string>

class TileMatrix {
private:
	std::string id;
	double res;
	double x0;
	double y0;
	int tileW;
	int tileH;
	long int matrixW;
	long int matrixH;
public:
	std::string getId();
    double getRes();
    double getX0();
    double getY0();
    int getTileW();
    int getTileH();


	TileMatrix(std::string id,double res,double x0,double y0,int tileW,	int tileH,long int matrixW, long int matrixH) :
		id(id), res(res), x0(x0), y0(y0), tileW(tileW), tileH(tileH), matrixW(matrixW), matrixH(matrixH) {};

	/*virtual ~TileMatrix();*/
};

#endif /* TILEMATRIX_H_ */
