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

#ifndef LIBTIFF_IMAGE_H
#define LIBTIFF_IMAGE_H

#include "Image.h"
#include "tiffio.h"
#include <string.h>

#define LIBTIFFIMAGE_MAX_FILENAME_LENGTH 512

class LibtiffImage : public Image {

    friend class libtiffImageFactory;

    private:
        TIFF* tif;
        char* filename;
        uint16_t bitspersample;
        uint16_t photometric;
        uint16_t compression;
        uint16_t rowsperstrip;

        size_t strip_size;
        uint8_t* strip_buffer;
        uint16_t current_strip;

        template<typename T>
        int _getline(T* buffer, int line);

    protected:
        /** Constructeur */
        LibtiffImage(int width, int height, int channels, BoundingBox<double> bbox, TIFF* tif, char* filename, int bitspersample, int photometric, int compression, int rowsperstrip);

    public:

        /** D */
        int getline(uint8_t* buffer, int line);

        /** D */
        int getline(float* buffer, int line);

        void inline setfilename(char* str) {strcpy(filename,str);};
        inline char* getfilename() {return filename;}
        uint16_t inline getbitspersample() {return bitspersample;}
        uint16_t inline getphotometric() {return photometric;}
        uint16_t inline getcompression() {return compression;}
        uint32_t inline getrowsperstrip() {return rowsperstrip;}
        
        /** Destructeur */
        ~LibtiffImage();
};

class libtiffImageFactory {
    public:
        LibtiffImage* createLibtiffImage(char* filename, BoundingBox<double> bbox);
        LibtiffImage* createLibtiffImage(char* filename, BoundingBox<double> bbox, int width, int height, int channels, uint16_t bitspersample, uint16_t photometric, uint16_t compression, uint16_t rowsperstrip);
};


#endif

