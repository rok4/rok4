/*
 * TileMatrix.cpp
 *
 *  Created on: 1 juil. 2010
 *      Author: root
 */

#include "TileMatrix.h"



double   TileMatrix::getRes()    {return res;}
double   TileMatrix::getX0()     {return x0;}
double   TileMatrix::getY0()     {return y0;}
int      TileMatrix::getTileW()  {return tileW;}
int      TileMatrix::getTileH()  {return tileH;}
long int TileMatrix::getMatrixW(){return matrixW;}
long int TileMatrix::getMatrixH(){return matrixH;}

std::string TileMatrix::getId()  {return id;}

bool TileMatrix::operator==(const TileMatrix& other) const
{
    return (this->res == other.res 
        && this->x0 == other.x0
        && this->y0 == other.y0
        && this->tileH == other.tileH
        && this->tileW == other.tileW
        && this->matrixH == other.matrixH
        && this->matrixW == other.matrixW
        && this->id.compare(other.id)==0);
}

bool TileMatrix::operator!=(const TileMatrix& other) const
{
       return !(*this == other);
}
