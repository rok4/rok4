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

#ifndef PENTEIMAGE_H
#define PENTEIMAGE_H

#include "Image.h"
#include <string>

 
using namespace std;
 


class PenteImage : public Image {
private:
    Image* origImage;
    uint8_t* pente;
    float* bufferTmp;
    float matrix[9];
	float resolution;
	string algo;
    //uint8_t center;
    int _getline ( uint8_t* buffer, int line );
    int _getline ( uint16_t* buffer, int line );
    int _getline ( float* buffer, int line );
    int getOrigLine ( uint8_t* buffer, int line );
    int getOrigLine ( uint16_t* buffer, int line );
    int getOrigLine ( float* buffer, int line );
    void generate();
    void generateLine ( int line, float* line1, float* line2 , float* line3, string algo );

public:
    virtual int getline ( float* buffer, int line );
    virtual int getline ( uint8_t* buffer, int line );
    virtual int getline ( uint16_t* buffer, int line );
    PenteImage ( Image* image, float resolution, string algo);
    virtual ~PenteImage();
};

#endif // PENTEIMAGE_H
