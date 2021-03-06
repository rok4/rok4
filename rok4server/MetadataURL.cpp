/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file MetadataURL.cpp
 * \~french
 * \brief Implémentation de la classe MetadataURL gérant les liens vers les métadonnées dans les documents de capacités
 * \~english
 * \brief Implement the MetadataURL Class handling capabilities metadata link elements
 */

#include "MetadataURL.h"

MetadataURL::MetadataURL ( std::string format, std::string href,
                           std::string type ) : ResourceLocator ( format,href ), type ( type ) {

}

MetadataURL::MetadataURL ( const MetadataURL& origMtdUrl ) : ResourceLocator ( origMtdUrl ) {
    type = origMtdUrl.type;
}

MetadataURL& MetadataURL::operator= ( const MetadataURL& other ) {
    if ( this != &other ) {
        ResourceLocator::operator= ( other );
        this->type = other.type;
    }
    return *this;
}

bool MetadataURL::operator== ( const MetadataURL& other ) const {
    return ( this->type.compare ( other.type ) == 0
             && this->getFormat().compare ( other.getFormat() ) == 0
             && this->getHRef().compare ( other.getHRef() ) == 0 );
}

bool MetadataURL::operator!= ( const MetadataURL& other ) const {
    return ! ( *this == other );
}

MetadataURL::~MetadataURL() {

}

