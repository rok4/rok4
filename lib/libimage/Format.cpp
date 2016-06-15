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

/**
 * \file Format.cpp
 ** \~french
 * \brief Implémentation des namespaces Compression, SampleFormat, Photometric, ExtraSample et Format
 * \details
 * \li SampleFormat : gère les types de canaux acceptés par les classes d'Image
 * \li Compression : énumère et manipule les différentes compressions
 * \li Format : énumère et manipule les différentes format d'image
 * \li Photometric : énumère et manipule les différentes photométries
 * \li ExtraSample : énumère et manipule les différents type de canal supplémentaire
 ** \~english
 * \brief Implement the namespaces Compression, SampleFormat, Photometric, ExtraSample et Format
 * \details
 * \li SampleFormat : managed sample type accepted by Image classes
 * \li Compression : enumerate and managed different compressions
 * \li Format : enumerate and managed different formats
 * \li Photometric : enumerate and managed different photometrics
 * \li ExtraSample : enumerate and managed different extra sample types
 */

#include "Format.h"
#include <string.h>
#include <ctype.h>
#include <algorithm>

namespace Compression {

const char *compression_name[] = {
    "UNKNOWN",
    "NONE",
    "DEFLATE",
    "JPEG",
    "PNG",
    "LZW",
    "PACKBITS",
    "JPEG2000"
};

eCompression fromString ( std::string strComp ) {
    int i;
    for ( i = compression_size; i ; --i ) {
        if ( strComp.compare ( compression_name[i] ) == 0 )
            break;
    }
    return static_cast<eCompression> ( i );
}

std::string toString ( eCompression comp ) {
    return std::string ( compression_name[comp] );
}

}

namespace Photometric {

const char *photometric_name[] = {
    "UNKNOWN",
    "GRAY",
    "RGB",
    "YCBCR",
    "MASK"
};

ePhotometric fromString ( std::string strPh ) {  

    int i;
    std::transform(strPh.begin(), strPh.end(), strPh.begin(), toupper);

    for ( i = photometric_size; i ; --i ) {
        if ( strPh.compare ( photometric_name[i] ) == 0 )
            break;
    }
    return static_cast<ePhotometric> ( i );
}

std::string toString ( ePhotometric ph ) {
    return std::string ( photometric_name[ph] );
}

}

namespace ExtraSample {

const char *extraSample_name[] = {
    "UNKNOWN",
    "ASSOCIATED ALPHA",
    "UNASSOCIATED ALPHA"
};

eExtraSample fromString ( std::string strPh ) {
    int i;
    for ( i = extraSample_size; i ; --i ) {
        if ( strPh.compare ( extraSample_name[i] ) == 0 )
            break;
    }
    return static_cast<eExtraSample> ( i );
}

std::string toString ( eExtraSample es ) {
    return std::string ( extraSample_name[es] );
}

}

namespace SampleFormat {

const char *sampleformat_name[] = {
    "UNKNOWN",
    "UINT",
    "FLOAT"
};

eSampleFormat fromString ( std::string strSF ) {
    int i;
    for ( i = sampleformat_size; i ; --i ) {
        if ( strSF.compare ( sampleformat_name[i] ) == 0 )
            break;
    }
    return static_cast<eSampleFormat> ( i );
}

std::string toString ( eSampleFormat sf ) {
    return std::string ( sampleformat_name[sf] );
}

}

namespace Rok4Format {

const char *eformat_name[] = {
    "UNKNOWN",
    "TIFF_RAW_INT8",
    "TIFF_JPG_INT8",
    "TIFF_PNG_INT8",
    "TIFF_LZW_INT8",
    "TIFF_ZIP_INT8",
    "TIFF_PKB_INT8",
    "TIFF_RAW_FLOAT32",
    "TIFF_LZW_FLOAT32",
    "TIFF_ZIP_FLOAT32",
    "TIFF_PKB_FLOAT32"
};

const char *eformat_mime[] = {
    "UNKNOWN",
    "image/tiff",
    "image/jpeg",
    "image/png",
    "image/tiff",
    "image/tiff",
    "image/tiff",
    "image/x-bil;bits=32",
    "image/tiff",
    "image/x-bil;bits=32",
    "image/tiff"
};

const char *eformat_encoding[] = {
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "deflate",
    ""
};

eformat_data fromString ( std::string strFormat ) {
    int i;
    for ( i=eformat_size; i ; --i ) {
        if ( strFormat.compare ( eformat_name[i] ) ==0 )
            break;
    }
    return static_cast<eformat_data> ( i );
}

std::string toString ( eformat_data format ) {
    return std::string ( eformat_name[format] );
}

std::string toMimeType ( eformat_data format ) {
    return std::string ( eformat_mime[format] );
}

eformat_data fromMimeType ( std::string mime ) {
    int i;
    for ( i=eformat_size; i ; --i ) {
        if ( mime.compare ( eformat_mime[i] ) == 0 )
            break;
    }
    return static_cast<eformat_data> ( i );
}

std::string toEncoding ( eformat_data format ) {
    return std::string ( eformat_encoding[format] );
}

int toSizePerChannel ( eformat_data format ) {
    if (format >= 7) {
        return 4;
    } else {
        return 1;
    }
}

int getPixelSize ( eformat_data format ) {

    // selon le type des images source : 1=uint8_t   2=uint16_t    4=float
    int pixel = 1;
    if (format == Rok4Format::UNKNOWN) {
        pixel = 1;
    }
    if (format == Rok4Format::TIFF_RAW_INT8 || format == Rok4Format::TIFF_JPG_INT8
            || format == Rok4Format::TIFF_PNG_INT8 || format == Rok4Format::TIFF_LZW_INT8
            || format == Rok4Format::TIFF_ZIP_INT8 || format == Rok4Format::TIFF_PKB_INT8) {
        pixel = 1;
    }
    if (format == Rok4Format::TIFF_RAW_FLOAT32 || format == Rok4Format::TIFF_LZW_FLOAT32
            || format == Rok4Format::TIFF_ZIP_FLOAT32 || format == Rok4Format::TIFF_PKB_FLOAT32) {
        pixel = 4;
    }

    return pixel;

}

}
