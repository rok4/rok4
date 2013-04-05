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

#ifndef LZWDECODER_H
#define LZWDECODER_H
#include <cstddef>
#include <climits>
#include <stdint.h>
#include <vector>
#include <string>
#include <list>
//
typedef std::list<uint8_t> lzwWord;

class lzwDecoder {
private:
    uint8_t maxBit;
    std::vector<lzwWord> dict;
    uint16_t nextCode;
    uint16_t maxCode;
    uint8_t bitSize;

    uint32_t buffer;
    uint8_t nReadbits;

    uint16_t code;

    char lastChar;
    uint16_t lastCode;

    bool firstPass;

    void clearDict();
public:

    lzwDecoder ( uint8_t maxBit=12 );
    uint8_t* decode ( const uint8_t * in, size_t inSize, size_t &outSize );
    ~lzwDecoder();
};

#endif // LZWDECODER_H
