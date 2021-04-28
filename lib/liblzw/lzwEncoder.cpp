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

#include "lzwEncoder.h"

#define M_CLR    uint16_t(256)          // clear table marker 
#define M_EOD    uint16_t(257)          // end-of-data marker 

#include <cstddef>
#include <cstdlib>
#include <cstring>

lzwEncoder::lzwEncoder()
{
    //init Vector
    dict = std::vector<lzwEncoderWord>(258, lzwEncoderWord());
    dict.reserve(512);
    maxBit=12;
    maxCode = 512;
    bitSize = 9;
    nextCode = 258;
    lastCode = 0;
    firstPass = true;
    buffer = 0;
    nWriteBits = 0;
}


void lzwEncoder::clearDict()
{
    dict.clear();
    //init Vector
    dict = std::vector<lzwEncoderWord>(258, lzwEncoderWord());
    dict.reserve(512);
    nextCode=258;
    maxCode= 512;
    bitSize=9;
}

void lzwEncoder::writeBits(uint16_t lzwCode, uint8_t* out, size_t& outPos
)
{
    buffer = ( buffer << bitSize ) | lzwCode; // put lzwCode in the write buffer
    nWriteBits += bitSize;
    while (nWriteBits >=8) { // Write 8bit of Data
        nWriteBits -= 8;
        out[outPos++] = buffer >> nWriteBits;
        buffer = buffer & (1 << nWriteBits ) - 1;
    }
}


uint8_t* lzwEncoder::encode(const uint8_t* in, size_t inSize, size_t& outSize)
{
    size_t outBufferSize = inSize*2;
    size_t outPos = 0;
    uint8_t* out = new uint8_t[outBufferSize];

    uint8_t character=0;
    uint16_t nextCode=0;
    if (firstPass && inSize) {
        //Initialize with first character
        lastCode = *(in++);
        inSize--;
        firstPass = false;
        writeBits(M_CLR,out, outPos);
    }

    while (inSize) {
        character= *(in++);
        inSize--;
        if ((nextCode = dict.at(lastCode).nextValue[character])) { // input already in dictionary waiting for new character
            lastCode = nextCode;
        } else { // Write Code and append to dictionary
            if (outPos + 2 > outBufferSize) {
                uint8_t* tmpBuffer = new uint8_t[(outBufferSize*2)];
                if (tmpBuffer) { // Enlarge your Buffer
                    memset(tmpBuffer+outBufferSize ,0,outBufferSize);
                    memcpy(tmpBuffer, out, outBufferSize);
                    delete[] out;
                    out = tmpBuffer;
                    outBufferSize *=2;
                } else { //Allocation error
                    outSize = 0;
                    return NULL;
                }
            }
            writeBits(lastCode,out, outPos); // put LastCode in the write buffer
            dict[lastCode].nextValue[character]=dict.size();
            dict.push_back(lzwEncoderWord());
            if (dict.size() == maxCode) {
                if (bitSize < maxBit) { //Extend
                    bitSize++;
                    maxCode*=2;
                    dict.reserve(maxCode);
                } else { // Clear Dict
                    writeBits(M_CLR,out, outPos);
                    clearDict();
                }
            }
            lastCode = character;

        }

    }
    writeBits(lastCode,out, outPos);
    //Should be triggered at the end
    writeBits(M_EOD,out, outPos);


    if (buffer) { // Flush the remaining data
        writeBits(buffer,out, outPos);
    }
    outSize = outPos;
    return out;
}

uint8_t* lzwEncoder::streamEncode(const uint8_t* in, size_t inSize, uint8_t* out, size_t& outSize)
{
    size_t outBufferSize = outSize;
    size_t outPos = 0;
    uint8_t character=0;
    uint16_t nextCode=0;
    if (firstPass && inSize) {
        //Initialize with first character
        lastCode = *(in++);
        inSize--;
        firstPass = false;
        writeBits(M_CLR,out, outPos);
    }

    while (inSize) {
        character= *(in++);
        inSize--;
        if ((nextCode = dict.at(lastCode).nextValue[character])) { // input already in dictionary waiting for new character
            lastCode = nextCode;
        } else { // Write Code and append to dictionary
            writeBits(lastCode,out, outPos); // put LastCode in the write buffer
            dict[lastCode].nextValue[character]=dict.size();
            dict.push_back(lzwEncoderWord());
            if (dict.size() == maxCode) {
                if (bitSize < maxBit) { //Extend
                    bitSize++;
                    maxCode*=2;
                    //dict.reserve(maxCode);
                } else { // Clear Dict
                    writeBits(M_CLR,out, outPos);
                    clearDict();
                }
            }
            lastCode = character;
            if (outPos+3 > outBufferSize) {//Buffer too small
                outSize = outPos;
                return (uint8_t*) in;
            }
        }

    }
    /*writeBits(lastCode,out, outPos);
    //Should be triggered at the end
    writeBits(M_EOD,out, outPos);


    if (buffer) { // Flush the remaining data
        writeBits(buffer,out, outPos);
    }*/
    outSize = outPos;
    return out;
}

void lzwEncoder::streamEnd(uint8_t* out, size_t& outPos)
{
    writeBits(lastCode,out, outPos);
    //Should be triggered at the end
    writeBits(M_EOD,out, outPos);


    if (buffer) { // Flush the remaining data
        writeBits(buffer,out, outPos);
    }

}


uint8_t* lzwEncoder::encodeAlt(const uint8_t* in, size_t inSize, size_t& outSize)
{
    outSize = 20;
    size_t outPos = outSize;
    size_t outBufferPos = 0;
    uint8_t* outBuffer = new uint8_t[outSize];
    memset(outBuffer ,0,outSize);
    uint8_t* out = outBuffer;
    uint8_t* oldout = out;
    uint8_t* inBuffer=(uint8_t*) in;
    out = streamEncode(in, inSize, out, outPos);
    while (out != oldout ) {

        uint8_t* tmpBuffer = new uint8_t[(outSize*2)];
        if (tmpBuffer) { // Enlarge your Buffer
            memset(tmpBuffer+outSize ,0,outSize);
            memcpy(tmpBuffer, outBuffer, outSize);
            delete[] outBuffer;
            outBuffer = tmpBuffer;
            tmpBuffer = NULL;
            outSize *=2;
        } else { //Allocation error
            outSize = 0;
            return NULL;
        }
        inBuffer = out;
        size_t inPos = (inBuffer - in);
        outBufferPos += outPos;
        oldout = outBuffer + outBufferPos;
        outPos = outSize - outPos;
        out = streamEncode(inBuffer, inSize - inPos, oldout, outPos);
    }
    outSize= outBufferPos + outPos;
    streamEnd(out,outSize);
    return out;
}


lzwEncoder::~lzwEncoder()
{

}


