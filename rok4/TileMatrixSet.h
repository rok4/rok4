/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

#ifndef TILEMATRIXSET_H_
#define TILEMATRIXSET_H_

#include <string>
#include <vector>
#include <map>
#include "TileMatrix.h"
#include "CRS.h"

/**
* @class TileMatrixSet
* @brief Implementation des TMS du WMTS
*/

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
    std::string getTitle() {
        return title;
    }
    std::string getAbstract() {
        return abstract;
    }
    std::vector<std::string>* getKeyWords() {
        return &keyWords;
    }
    CRS getCrs() const {
        return crs;
    }
    //TODO
    int best_scale ( double resolution_x, double resolution_y );

    TileMatrixSet ( std::string id, std::string title, std::string abstract, std::vector<std::string> & keyWords, CRS& crs, std::map<std::string, TileMatrix> & tmList ) :
            id ( id ), title ( title ), abstract ( abstract ), keyWords ( keyWords ), crs ( crs ), tmList ( tmList ) {};


    bool operator== ( const TileMatrixSet& other ) const;
    bool operator!= ( const TileMatrixSet& other ) const;

    TileMatrixSet ( const TileMatrixSet& t ) {
        id=t.id;
        title=t.title;
        abstract=t.abstract;
        keyWords=t.keyWords;
        crs=t.crs;
        tmList=t.tmList;
    }
    ~TileMatrixSet() {}
};

#endif /* TILEMATRIXSET_H_ */
