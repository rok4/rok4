/*
 * Copyright © (2011) Institut national de l'information
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

#include "pkbDecoder.h"

#include <cstddef>
#include <cstring>
#include <list>
#include <iostream>

#define BUFFER_SIZE 256*256*4 // Default tile Size

pkbDecoder::pkbDecoder() {

}


uint8_t * pkbDecoder::decode(const uint8_t * in, size_t inSize, size_t &outSize) {
    outSize = inSize * 2;
    uint8_t * out = new uint8_t[outSize];
    int8_t header;
    int count;
    size_t inPos = 0, outPos = 0;
    while (inPos < inSize -1 ) {
        header = (int8_t) *(in+inPos++);
        if (header <= 0) { // Run
            count = 1 - header;
            if (outPos + count > outSize) {
                uint8_t* tmpBuffer = new uint8_t[(outSize*2)];
                if (tmpBuffer) { // Enlarge your Buffer
                    memset(tmpBuffer+outSize ,0,outSize);
                    memcpy(tmpBuffer, out, outSize);
                    delete[] out;
                    out = tmpBuffer;
                    outSize *=2;
                } else { //Allocation error
                    outSize = 0;
                    return NULL;
                }
            }
            
            memset(out+outPos,*(in+inPos),count);
            outPos+=count;
            inPos++;
            
        } else if ( header < 128 ) { // Literal
            count = 1 + header;
            if (outPos + count > outSize) {
                uint8_t* tmpBuffer = new uint8_t[(outSize*2)];
                if (tmpBuffer) { // Enlarge your Buffer
                    memset(tmpBuffer+outSize ,0,outSize);
                    memcpy(tmpBuffer, out, outSize);
                    delete[] out;
                    out = tmpBuffer;
                    outSize *=2;
                } else { //Allocation error
                    outSize = 0;
                    return NULL;
                }
            }
            
            memcpy(out+outPos,in+inPos,count);
            outPos+=count;
            inPos+= count;
        } 
    }
    outSize = outPos;
    return out;
}


pkbDecoder::~pkbDecoder() {

}
