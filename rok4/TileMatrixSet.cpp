/*
 * Copyright © (2011-2013) Institut national de l'information
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

/**
 * \file TileMatrix.cpp
 * \~french
 * \brief Implémentation de la classe TileMatrixSet gérant une pyramide de matrices (Cf TileMatrix)
 * \~english
 * \brief Implement the TileMatrixSet Class handling a pyramid of matrix (See TileMatrix)
 */


#include "TileMatrixSet.h"

std::string TileMatrixSet::getId() {
    return id;
}
std::map<std::string, TileMatrix>* TileMatrixSet::getTmList() {
    return &tmList;
}

TileMatrixSet::TileMatrixSet ( const TileMatrixSetXML& t ) {
	id = t.id;
	title = t.title;
	abstract = t.abstract;
	keyWords = t.keyWords;
	crs = t.crs;
	tmList = t.listTM;
}

bool TileMatrixSet::operator== ( const TileMatrixSet& other ) const {
    return ( this->keyWords.size() ==other.keyWords.size()
             && this->tmList.size() ==other.tmList.size()
             && this->id.compare ( other.id ) ==0
             && this->title.compare ( other.title ) ==0
             && this->abstract.compare ( other.abstract ) ==0
             && this->crs==other.crs );
}

bool TileMatrixSet::operator!= ( const TileMatrixSet& other ) const {
    return ! ( *this == other );
}
