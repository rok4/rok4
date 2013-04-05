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

#include "Data.h"
#include <cstring> // pour memcpy
#include "Logger.h"

/**
 * Constructeur.
 * Le paramètre dataStream est complètement lu. Il est donc inutilisable par la suite.
 */

// TODO : peut être optimisé, à mettre au propre
BufferedDataSource::BufferedDataSource ( DataStream& dataStream ) : type ( dataStream.getType() ), httpStatus ( dataStream.getHttpStatus() ), dataSize ( 0 ) {
    // On initialise data à une taille arbitraire de 32Ko.
    size_t maxSize = 32768;
    data = new uint8_t[maxSize];

    while ( !dataStream.eof() ) { // On lit le DataStream jusqu'au bout
        size_t size = dataStream.read ( data + dataSize, maxSize - dataSize );
        dataSize += size;
        if ( size == 0 || dataSize == maxSize ) { // On alloue 2 fois plus de place si on en manque.
            maxSize *= 2;
            uint8_t* tmp = new uint8_t[maxSize];
            memcpy ( tmp, data, dataSize );
            delete[] data;
            data = tmp;
        }
    }

    // On réalloue exactement la taille nécessaire pour ne pas perdre de place
    uint8_t* tmp = new uint8_t[dataSize];

    memcpy ( tmp, data, dataSize );
    delete[] data;
    data = tmp;
}

