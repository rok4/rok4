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

#ifndef _FILEDATASOURCE_
#define _FILEDATASOURCE_

#include "Data.h"

/*
 * Classe qui lit les tuiles d'un fichier tuilé.
 */

class FileDataSource : public DataSource {
private:
    std::string filename;
    const uint32_t posoff;		// Position dans le fichier des 4 octets indiquant la position de la tuile dans le fichier
    const uint32_t possize;		// Position dans le fichier des 4 octets indiquant la taille de la tuile dans le fichier
    uint8_t* data;
    size_t size;
    std::string type;
public:
    FileDataSource ( const char* filename, const uint32_t posoff, const uint32_t possize, std::string type );
    const uint8_t* getData ( size_t &tile_size );

    /*
     * @ return le type MIME de la source de donnees
     */
    std::string getType() {
        return type;
    }

    bool releaseData();

    ~FileDataSource() {
        releaseData();
    }

    int getHttpStatus() {
        return 200;
    }
};

#endif
