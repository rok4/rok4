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

#include "EmptyDataSource.h"

EmptyDataSource::EmptyDataSource (int channels, std::vector<int> _color, int width, int height, Rok4Format::eformat_data format):
    channels (channels), width (width), height (height), format (format)
{

    // initialisation
    dataSize = 0;
    data = NULL;

    //stokage de color
    color = new int[channels];
    for ( int c = 0; c < channels; c++ ) color[c] = _color[c];

    //creation de l'image
    EmptyImage *empty = new EmptyImage(width,height,channels,color);

    //formatage de l'image
    DataStream *stream = NULL;

    if (format == Rok4Format::TIFF_JPG_INT8) {
        stream = new JPEGEncoder ( empty );
    } else if (format == Rok4Format::TIFF_PNG_INT8) {
        stream = new PNGEncoder ( empty, NULL );
    } else if (format == Rok4Format::TIFF_RAW_INT8) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_RAW_INT8, false );
    } else if (format == Rok4Format::TIFF_LZW_INT8) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_LZW_INT8, false );
    } else if (format == Rok4Format::TIFF_ZIP_INT8) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_ZIP_INT8, false );
    } else if (format == Rok4Format::TIFF_PKB_INT8) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_PKB_INT8, false );
    } else if (format == Rok4Format::TIFF_RAW_FLOAT32) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_RAW_FLOAT32, false );
    } else if (format == Rok4Format::TIFF_LZW_FLOAT32) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_LZW_FLOAT32, false );
    } else if (format == Rok4Format::TIFF_ZIP_FLOAT32) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_ZIP_FLOAT32, false );
    } else if (format == Rok4Format::TIFF_PKB_FLOAT32) {
        stream = TiffEncoder::getTiffEncoder ( empty, Rok4Format::TIFF_PKB_FLOAT32, false );
    }

    //transformation en datasource
    BufferedDataSource *source = NULL;
    if (stream != NULL) {
        source = new BufferedDataSource(*stream);
        delete stream;
    }

    if (source != NULL) {
        dataSize = source->getSize();
        data = new uint8_t[dataSize];
        memcpy ( data, source->getData(dataSize), dataSize );
        delete source;
    }

}
