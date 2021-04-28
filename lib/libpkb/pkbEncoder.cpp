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

#include "pkbEncoder.h"

#include <cstring>


pkbEncoder::pkbEncoder() {

}


uint8_t* pkbEncoder::encode ( const uint8_t* in, size_t inSize, size_t& outSize ) {
    size_t pkbBufferPos= 0;
    size_t pkbBufferSize = inSize + inSize;
    uint8_t * pkbBuffer = new uint8_t[pkbBufferSize]; // Worst Case Compression
    compression_state state = BASE;
    long count=0, literalCount=0;
    uint8_t currentChar;
    size_t inputpos = 0, lastLiteralPos=0;

    while ( inputpos < inSize ) {

        // get first unread byte
        currentChar = * ( in+inputpos++ );
        count = 1;

        //find identical bytes
        while (inputpos < inSize -1 && currentChar == * ( in+inputpos )) {
            inputpos++; // input buffer position
            count++; // number of identical bytes
        }

        switch ( state ) {
        case BASE: // Initial State
            if ( count > 1 ) {

                state = RUN;
                while ( count > 128 ) {
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) -127;
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                    count -= 128;
                }
                if ( count > 1 ) {
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) ( - ( count-1 ) );
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                    count = 0;
                }
            }
            if ( count == 1 ) {
                lastLiteralPos = pkbBufferPos;
                literalCount = 1;
                * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) ( literalCount -1 );
                * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                count = 0;
                state = LITERAL;
            }
            break;
        case LITERAL: // Last was literal
            if ( count > 1 ) {  // RUN
                literalCount = 0;
                state = RUN;
                while ( count > 128 ) {
                    //state = RUN;
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) - 127;
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                    count -=128;
                }
                if ( count > 1 ) {
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) ( - ( count-1 ) );
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                } else if ( count == 1 ) { // New Literal
                    //TODO Check whether we merge the previous Lit/Run/Lit to a Lit
                    lastLiteralPos = pkbBufferPos;
                    literalCount = 1;
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) ( literalCount -1 );
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                    state = LITERAL;
                }
            } else { // Literal
                literalCount++;
                if ( literalCount > 128 ) {
                    lastLiteralPos = pkbBufferPos++;
                    literalCount -=128;
                }
                * ( pkbBuffer+lastLiteralPos ) = ( int8_t ) ( literalCount -1 );
                * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
            }
            break;
        case RUN:
            if ( count > 1 ) {
                while ( count > 128 ) {
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) -127;
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                    count -= 128;
                }
                if ( count > 1 ) {
                    * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) ( - ( count-1 ) );
                    * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
                    break;
                }
            }
            if ( count == 1 ) {
                state = LITERAL;
                lastLiteralPos = pkbBufferPos;
                literalCount = 1;
                * ( pkbBuffer+pkbBufferPos++ ) = ( int8_t ) ( literalCount -1 );
                * ( pkbBuffer+pkbBufferPos++ ) = currentChar;
            }
            break;
        }

    }

    outSize = pkbBufferPos;
    return pkbBuffer;
}


pkbEncoder::~pkbEncoder() {

}


