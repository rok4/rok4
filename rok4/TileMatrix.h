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
    long int getMatrixW();
    long int getMatrixH();

    TileMatrix(std::string id,double res,double x0,double y0,int tileW, int tileH,long int matrixW, long int matrixH) :
            id(id), res(res), x0(x0), y0(y0), tileW(tileW), tileH(tileH), matrixW(matrixW), matrixH(matrixH) {};

    TileMatrix(const TileMatrix& t)
    {
        id=t.id;
        res=t.res;
        x0=t.x0;
        y0=t.y0;
        tileW=t.tileW;
        tileH=t.tileH;
        matrixW=t.matrixW;
        matrixH=t.matrixH;
    }

    bool operator==(const TileMatrix& other) const;
    bool operator!=(const TileMatrix& other) const;
    /*virtual ~TileMatrix();*/
};

#endif /* TILEMATRIX_H_ */
