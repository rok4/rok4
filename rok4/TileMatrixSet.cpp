#include "TileMatrixSet.h"

std::string TileMatrixSet::getId(){return id;}
std::map<std::string, TileMatrix>* TileMatrixSet::getTmList(){return &tmList;}

/**
 * Rapid comparison of two TileMatrixSet, Keywords and TileMatrix are not verified
 * @return true if attributes are equal and if lists have the same size
 */
bool TileMatrixSet::operator==(const TileMatrixSet& other) const
{
    return (this->keyWords.size()==other.keyWords.size()
           && this->tmList.size()==other.tmList.size()
           && this->id.compare(other.id)==0
           && this->title.compare(other.title)==0
           && this->abstract.compare(other.abstract)==0
           && this->crs==other.crs); 
}

bool TileMatrixSet::operator!=(const TileMatrixSet& other) const
{
    return !(*this == other);
}
