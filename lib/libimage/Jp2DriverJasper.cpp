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

#include "Jp2DriverJasper.h"

#include <jasper/jasper.h>

Libjp2Image * Jp2DriverJasper::createLibjp2ImageToRead(char* filename, BoundingBox<double> bbox, double resx, double resy) {

    LOGGER_DEBUG("Not yet implemented !");
    LOGGER_DEBUG("Version Driver Jasper : " << jas_getversion());

    int     width,
            height,
            channels,
            bitspersample;

    int has_alpha = 0;

    uint8_t * data;

    return NULL;


//    // The input image is to be read from a file.
//    jas_stream_t *in = NULL;
//    in = jas_stream_fopen(filename, "rb");
//    if (! in) {
//        LOGGER_ERROR("canot open input image file !");
//        return NULL;
//    }

//    // FIXME : get list format ?
//    // jas_image_fmtinfo_t *fmtinfo;
//    // int fmtid;
//    // for (fmtid = 0;; ++fmtid) {
//    //     if (! (fmtinfo = jas_image_lookupfmtbyid(fmtid))) {
//    //         break;
//    //     }
//    //     LOGGER_DEBUG("frmt : " << fmtinfo->name << " | " << fmtinfo->desc);
//    // }

//    int idfmt = -1; // 4 -> jp2 : JPEG-2000 JP2 File Format Syntax (ISO/IEC 15444-1)
//    // idfmt = jas_image_getfmt(in);


//    // if (idfmt < 0) {
//    //     LOGGER_ERROR("input image has unknown format !");
//    //     return NULL;
//    // }

//    // uint_fast32_t size = jas_image_rawsize(in);

//    // Get the input image data.
//    jas_tmr_t dectmr;
//    jas_image_t *image;

//    jas_tmr_start(&dectmr);
//    image = jas_image_decode(in, idfmt, NULL);
//    if (! image) {
//        LOGGER_ERROR("cannot load image data !");
//        return NULL;
//    }
//    jas_tmr_stop(&dectmr);

//    // Component with 3 or 4 channels only...
//    int numcmpts = jas_image_numcmpts(image);

//    if (numcmpts == 1 || numcmpts == 2) { // Gray + (Alpha)
//        LOGGER_ERROR("Gray (A) has not been yet implemented !");
//        return NULL;
//    }
//    else if (numcmpts == 3 || numcmpts == 4) { // RGB + (Alpha)
//        LOGGER_DEBUG("Component : RGB(Alpha).");
//        if (numcmpts == 4) {
//            has_alpha = 1;
//        }
//    }
//    else {
//        LOGGER_ERROR("Number of component > 4, unknow ?!");
//        return NULL;
//    }

//    // FIXME : we could force the color space to RGB ?

//    // FIXME : data ?

//    // If this fails, we don't care.
//    (void) jas_stream_close(in);

//    // Clean
//    jas_image_destroy(image);
//    jas_image_clearfmts();

//    return new Libjp2Image (
//            width, height, resx, resy, channels, bbox, filename,
//            SampleFormat::UINT, bitspersample, Photometric::RGB, Compression::NONE,
//            data);
}
