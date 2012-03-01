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

#ifndef DECODER_H
#define DECODER_H

//#include <stdint.h>// pour uint8_t
//#include <cstddef> // pour size_t
//#include <string>

#include "Data.h"
#include "Image.h"
#include "Utils.h"

struct JpegDecoder {
    static const uint8_t* decode(DataSource* encData, size_t &size);
};

struct PngDecoder {
    static const uint8_t* decode(DataSource* encData, size_t &size);
};

struct LzwDecoder {
    static const uint8_t* decode(DataSource* encData, size_t &size);
};


struct InvalidDecoder {
    static const uint8_t* decode(DataSource* encData, size_t &size) {
        size = 0;
        return 0;
    }
};




/**
 * Classes Decoder
 */
template <class Decoder>
class DataSourceDecoder : public DataSource {
private:
    DataSource* encData;
    const uint8_t* decData;
    size_t decSize;
public:
    DataSourceDecoder(DataSource* encData) : encData(encData), decData(0), decSize(0) {}

    ~DataSourceDecoder() {
        if (decData)
            delete[] decData;
        delete encData;
    }

    const uint8_t* getData(size_t &size) {
        if (!decData && encData) {
            decData = Decoder::decode(encData, decSize);
            if (!decData) {
                delete[] encData;
                encData = 0;
            }
        }
        size = decSize;
        return decData;
    }

    bool releaseData() {
        if (encData) encData->releaseData();
        if (decData) delete[] decData;
        decData = 0;
    }

    std::string getType() {
        return "image/bil";
    }
    int getHttpStatus() {
        return 200;
    }
};




class ImageDecoder : public Image {
private:
    DataSource* dataSource;

    int source_width;
    int source_height;
    int margin_top;
    int margin_left;

    int pixel_size; // type des images source : 1=uint8_t       4=float

    // La donnee brute (source) est de type uint8_t
    const uint8_t* rawData;

    int getDataline(uint8_t* buffer, int line);

    int getDataline(float* buffer, int line);

    template<typename T> inline int getNoDataline(T* buffer, int line) {
        memset(buffer, 0, width * channels * sizeof(T));
        return width * channels;
    }

    // TODO : a deplacer dans le cpp (je n'y suis pas arrive a cause d un probleme de compilation lie au template)
    template<typename T>
    inline int _getline(T* buffer, int line) {

        if (rawData) { // Est ce que l'on a de la donnee
            return getDataline(buffer, line);
            // TODO: libérer le dataSource lorsque l'on lit la dernière ligne de l'image...
        }
        else if (dataSource) { // Non alors on essaye de la l'initialiser depuis dataSource
            size_t size;
            if (rawData = dataSource->getData(size)) {
                return getDataline(buffer, line);
            }
            else {
                delete dataSource;
                dataSource = 0;
            }
        }
        LOGGER_DEBUG("Decoding error, fill with black");
        return getNoDataline(buffer, line);
    }

public:
    ImageDecoder(DataSource* dataSource, int source_width, int source_height, int channels,
                 BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.),
                 int margin_left = 0, int margin_top = 0, int margin_right = 0, int margin_bottom = 0, int pixel_size=1) :
            Image(source_width - margin_left - margin_right, source_height - margin_top - margin_bottom, channels, bbox),
            dataSource(dataSource),
            source_width(source_width),
            source_height(source_height),
            margin_top(margin_top),
            margin_left(margin_left),
            pixel_size(pixel_size),
            rawData(0) {}

    /* Implémentation de l'interface Image */
    inline int getline(uint8_t* buffer, int line) {
        return _getline(buffer, line);
    }
    inline int getline(float* buffer, int line)   {
        return _getline(buffer, line);
    }

    ~ImageDecoder() {
        if (dataSource) {
            dataSource->releaseData();
            delete dataSource;
            //delete [] rawData;
        }
    }


};



#endif


