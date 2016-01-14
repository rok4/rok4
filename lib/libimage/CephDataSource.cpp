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

#include "CephDataSource.h"
#include <fcntl.h>
#include "Logger.h"
#include <cstdio>

// Taille maximum d'une tuile WMTS
#define MAX_TILE_SIZE 1048576

CephDataSource::CephDataSource ( const char* name, const uint32_t posoff, const uint32_t possize, std::string type , CephContext* cc, std::string encoding) :
    StoreDataSource(name,posoff, possize,type,encoding), cephContext(cc) {

}

/*
 * Fonction retournant les données de la tuile
 * Le fichier ne doit etre lu qu une seule fois
 * Indique la taille de la tuile (inconnue a priori)
 */
const uint8_t* CephDataSource::getData ( size_t &tile_size ) {
    if ( data ) {
        tile_size=size;
        return data;
    }
    
    // Lecture de la position de la tuile dans le fichier
    uint8_t* uint32tab = new uint8_t[sizeof( uint32_t )];
    
    if (! cephContext->read(uint32tab, posoff, 4, name)) {
        LOGGER_ERROR ( "Erreur lors de la lecture de la position de la tuile dans l'objet " << name );
        return 0;
    }    
    uint32_t tileOffset = *((uint32_t*) uint32tab);
    
    // Lecture de la taille de la tuile dans le fichier
    // Ne lire que 4 octets (la taille de tile_size est plateforme-dependante)
    // Lecture de la position de la tuile dans le fichier    
    if (! cephContext->read(uint32tab, possize, 4, name)) {
        LOGGER_ERROR ( "Erreur lors de la lecture de la taille de la tuile dans l'objet " << name );
        return 0;
    }    
    uint32_t tileSize = *((uint32_t*) uint32tab);
    tile_size = tileSize;
    
    // La taille de la tuile ne doit pas exceder un seuil
    // Objectif : gerer le cas de fichiers TIFF non conformes aux specs du cache
    // (et qui pourraient indiquer des tailles de tuiles excessives)
    if ( tile_size > MAX_TILE_SIZE ) {
        LOGGER_ERROR ( "Tuile trop volumineuse dans le fichier " << name ) ;
        return 0;
    }
    
    // Lecture de la tuile
    data = new uint8_t[tile_size];
    if (! cephContext->read(data, tileOffset, tile_size, name)) {
        LOGGER_ERROR ( "Erreur lors de la lecture de la tuile dans l'objet " << name );
        return 0;
    }

    return data;
}

uint8_t* CephDataSource::getThisData ( const uint32_t offset, const uint32_t size ) {
    
    uint8_t* wanteddata = new uint8_t[size];
    if ( ! cephContext->read(wanteddata, offset, size, name) ) {
        LOGGER_ERROR ( "Unable to read " << size << " bytes (from the " << offset << " one) in the object " << name );
        return 0;
    }
    
    return wanteddata;
}

CephDataSource::~CephDataSource() {
}
