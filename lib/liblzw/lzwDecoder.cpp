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

#include "lzwDecoder.h"

#include "Logger.h"
#include <cstddef>
#include <cstring>
#include <list>
#include <iostream>

#define M_CLR    256          // clear table marker 
#define M_EOD    257          // end-of-data marker 
#define BUFFER_SIZE 256*256*4 // Default tile Size

lzwDecoder::lzwDecoder(uint8_t maxBit) : maxBit(maxBit) {
    bitSize=9;
    maxCode=512;

    dict.reserve(maxCode);
    for ( int i = 0; i < 255; ++i) {
        lzwWord word;
        word.push_back(i);
        dict.push_back(word);;
    }
    dict.push_back(lzwWord());
    dict.push_back(lzwWord());


    firstPass=true;
    buffer =0;
    nReadbits = 0;
    lastChar=0;
}


void lzwDecoder::clearDict() {
    dict.clear();
    dict.reserve(maxCode);
    for ( int i = 0; i < 256; ++i) {
        lzwWord word;
        word.push_back(i);
        dict.push_back(word);;
    }
    dict.push_back(lzwWord());
    dict.push_back(lzwWord());

    bitSize=9;
    maxCode=512;
    lastCode=256;
    lastChar=0;
    firstPass = true;
}


uint8_t* lzwDecoder::decode ( const uint8_t* in, size_t inSize, size_t& outPos ) {
    
    size_t outSize= (outPos?outPos:BUFFER_SIZE);
    uint8_t* out = new uint8_t[outSize];
    outPos=0;
    lzwWord outString = lzwWord();
    //Initialization

    while (inSize) {
        while (firstPass) {
            while ( nReadbits < bitSize) {
                if ( inSize > 0) {
                    buffer = (buffer << 8) | *(in++);
                    nReadbits += 8;
                } else { // Not enough data in the current buffer. Return current state
                    return out;
                }
            }

            nReadbits -= bitSize;
            // Extract BitSize bits from buffer
            code = buffer >> nReadbits;
            // Remove extracted code from buffer
            buffer = buffer & (1 << nReadbits) - 1;
            //Test Code
            if (code == M_EOD ) { //End of Data
                return out;
            }
            if ( code == M_CLR ) { // Reset Dictionary
                this->clearDict();
            } else {
                out[outPos++] = code;
                lastCode = code;
                lastChar = code;
                firstPass = false;
            }
        }

        //Read enough data from input stream
        while ( nReadbits < bitSize) {
            if ( inSize > 0) {
                buffer = (buffer << 8) | *(in++);
                nReadbits += 8;
                inSize--;
            } else { // Not enough data in the current buffer. Return current state
                return out;
            }
        }

        nReadbits -= bitSize;
        // Extract BitSize bits from buffer
        code = buffer >> nReadbits;
        // Remove extracted code from buffer
        buffer = buffer & (1 << nReadbits) - 1;
        //Test Code
        if (code == M_EOD ) { //End of Data
            return out;
        }
        if ( code == M_CLR ) { // Reset Dictionary
            this->clearDict();
        } else {
            if (code > dict.size() - 1) {// Code Not found
                //itCode = dict.find(lastCode);
                lzwWord oldstring = dict.at(lastCode);
                outString.assign(oldstring.begin(),oldstring.end());
                outString.push_back(lastChar);
            } else { // Code found get Value
                outString.assign(dict.at(code).begin(),dict.at(code).end());
            }
            lastChar = *(outString.begin());





            for (lzwWord::iterator it = outString.begin(); it != outString.end(); it++) {
                if (outPos >= outSize) {
                    uint8_t* tmpBuffer = new uint8_t[(outSize*2)];
                    if (tmpBuffer) { // Enlarge your Buffer
                        memset(tmpBuffer+outSize ,0,outSize);
                        memcpy(tmpBuffer, out, outSize);
                        delete[] out;
                        out = tmpBuffer;
                        outSize *=2;
                    } else { //Allocation error
                        outPos = 0;
                        return NULL;
                    }
                }
                out[outPos++]= *it;
            }
            lzwWord newEntry = dict.at(lastCode);
            newEntry.push_back(lastChar);
            dict.push_back(newEntry);
            lastCode = code;
            //Dictionary need to be extended or reseted
            if (dict.size() == maxCode -1) {
                //Extend
                if (bitSize < maxBit) {
                    bitSize++;
                    maxCode*=2;
                    dict.reserve(maxCode);
                }
                // else : the next code must be M_CLR is written in maxBit bit
            }
        }

    }

    return out;
}

lzwDecoder::~lzwDecoder()
{
    dict.clear();
}


