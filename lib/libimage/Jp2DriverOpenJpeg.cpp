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

#include <string.h>
#include <cstring>
#include <ctype.h>

#include "Jp2DriverOpenJpeg.h"

#include "Logger.h"
#include "Utils.h"
#include "Format.h"

#include "openmj2/openjpeg.h"
#include "openjpeg.h"

Jp2DriverOpenJpeg::Jp2DriverOpenJpeg(char *filename, BoundingBox< double > bbox, double resx, double resy)
{
    LOGGER_DEBUG ( "USE DRIVER OPENJPEG FOR JPEG2000." );

    m_cFilename.assign(filename);

    m_dResx     = resx;
    m_dResy     = resy;

    m_xmax      = bbox.xmax;
    m_ymin      = bbox.ymin;
    m_xmin      = bbox.xmin;
    m_ymax      = bbox.ymax;

    LOGGER_DEBUG ("Call Image (m_cFilename) : " << m_cFilename);
}
Jp2DriverOpenJpeg::Jp2DriverOpenJpeg()
{
    LOGGER_DEBUG ( "USE DRIVER OPENJPEG FOR JPEG2000." );

    m_dResx     = 0.;
    m_dResy     = 0.;
    m_xmax      = 0.;
    m_ymin      = 0.;
    m_xmin      = 0.;
    m_ymax      = 0.;
}

Jp2DriverOpenJpeg::~Jp2DriverOpenJpeg()
{
}

Libjp2Image *Jp2DriverOpenJpeg::createLibjp2ImageToRead()
{
    int     width,
            height,
            channels,
            bitspersample;

    int has_alpha = 0;

    uint8_t * data;
    uint8_t * data_ini; // JPEG2000 data in memory

    /* ---------------------------------------- */
    /* Read the input file and put it in memory */
    /* ---------------------------------------- */

    // Set decoding parameters to default values
    opj_dparameters_t parameters;
    opj_set_default_decoder_parameters(&parameters);
    // parameters.decod_format = 1;  /* input  file format : JP2_CFMT */
    // parameters.cod_format   = 14; /* output file format : TIF_DFMT */

    // File
    strncpy(parameters.infile, m_cFilename.c_str(), 4096 * sizeof(char));

    // Open
    FILE *fsrc = NULL;
    fsrc = fopen(parameters.infile, "rb");

    if (!fsrc) {
        LOGGER_ERROR ("failed to open jp2 for reading");
        return NULL;
    }

    fseek(fsrc, 0, SEEK_END);
    int file_length = ftell(fsrc);
    fseek(fsrc, 0, SEEK_SET);
    data_ini = (unsigned char *) malloc(file_length);

    // Read
    if (fread(data_ini, 1, file_length, fsrc) != (size_t) file_length) {
        free(data_ini);
        fclose(fsrc);
        LOGGER_ERROR ("fread return a number of element different from the expected.");
        return NULL;
    }

    // Close
    fclose(fsrc);

    /* ---------------------- */
    /* Decode the code-stream */
    /* ---------------------- */

    opj_dinfo_t *dinfo = NULL;        // Handle to a decompressor
    opj_cio_t *cio = NULL;            // Byte input-output stream (CIO)
    opj_codestream_info_t cstr_info;  // Codestream information structure
    opj_image_t *image = NULL;        // Defines image data and characteristics

    // Creates a J2K/JPT/JP2 decompression structure
    dinfo = opj_create_decompress(CODEC_JP2);

    // Setup the decoder decoding parameters using the current image and user parameters
    opj_setup_decoder(dinfo, &parameters);

    // Open and allocate a memory stream for read / write */
    cio = opj_cio_open((opj_common_ptr) dinfo, data_ini, file_length);

    // Decode an image from a JPEG-2000 codestream and extract the codestream information */
    image = opj_decode_with_info(dinfo, cio, &cstr_info);

    if(!image) {
        LOGGER_ERROR ("failed to decode image!");
        opj_destroy_decompress(dinfo);
        opj_cio_close(cio);
        free(data_ini);
        return NULL;
    }

    // Close the byte stream */
    opj_cio_close(cio);

    // Free the memory containing the code-stream */
    free(data_ini);
    data_ini = NULL;

    // information sur l'image
    this->information();

    // BitsPerSample
    bitspersample = image->comps[0].prec;
    if (bitspersample != 8) {
        LOGGER_ERROR ("Only 8 bits has been implemented !");
        return NULL;
    }

    // Photometric
    // COLOR_SPACE::OPJ_CLRSPC_SRGB = 1
    if (image->color_space != 1) {
        LOGGER_ERROR ("Only RGB(A) has been implemented !");
        return NULL;
    }

    // Channels
    if(image->numcomps == 1 /* GRAY */) {

        LOGGER_ERROR ("Gray has not been yet implemented !");
        return NULL;
    }

    if(image->numcomps == 2 /* GRAY_ALPHA */
        && image->comps[0].dx == image->comps[1].dx
        && image->comps[0].dy == image->comps[1].dy
        && image->comps[0].prec == image->comps[1].prec) {

        LOGGER_ERROR ("Gray (A) has not been yet implemented !");
        return NULL;
    }

    if(image->numcomps >= 3 /* RGB_ALPHA */
        && image->comps[0].dx == image->comps[1].dx
        && image->comps[1].dx == image->comps[2].dx
        && image->comps[0].dy == image->comps[1].dy
        && image->comps[1].dy == image->comps[2].dy
        && image->comps[0].prec == image->comps[1].prec
        && image->comps[1].prec == image->comps[2].prec) {

            width     = image->comps[0].w;
            height    = image->comps[0].h;
            channels  = image->numcomps;
            has_alpha = (image->numcomps == 4);

            if (has_alpha) {
                LOGGER_INFO ("There's an alpha component !");
            }
    }

    // bbox
    BoundingBox<double> bbox (m_xmin, m_ymin, m_xmax, m_ymax);

    /* ---------------------- */
    /* Encode the new stream  */
    /* ---------------------- */

    // Data
    int imgsize = width * height;       // taille de l'image en pixels pour une couche
    int step    = 3 + has_alpha;        // 3 ou 4 couches ( = channels)
    int sgnd    = image->comps[0].sgnd; // signe
    int adjust  = sgnd ? 1 << (image->comps[0].prec - 1) : 0; // 1: signed 0: unsigned !

    // Allocation memoire
	data = (unsigned char *) malloc(imgsize*step);
	
    int i;
    int index = 0;

    for(i=0; i < imgsize * step; i += step) {
        int r, g, b, a = 0;

        // index = nb de pixels d'une couche
        if(index < imgsize) {

            // recuperation d'un pixel r,g,b (a)
            r = image->comps[0].data[index];
            g = image->comps[1].data[index];
            b = image->comps[2].data[index];
            if(has_alpha) a = image->comps[3].data[index];

            index++;

            // ajout du signe
            if(sgnd) {
                r += adjust;
                g += adjust;
                b += adjust;

                if(has_alpha) a += adjust;
            }

            // copie d'un pixel : r,g,b (a)
            data[i+0] = r ;
            data[i+1] = g ;
            data[i+2] = b ;
            if(has_alpha) data[i+3] = a;

        }
        else {
            LOGGER_DEBUG("nb de pixels d'une couche trop important !");
        }
    }
    
    // Free remaining structures
    if (dinfo) {
        opj_destroy_decompress(dinfo);
        dinfo = NULL;
    }

    // Free codestream information structure
    opj_destroy_cstr_info(&cstr_info);

    // Free image data structure
    opj_image_destroy(image);
    image = NULL;
    
    return new Libjp2Image (
        width, height, m_dResx, m_dResy, channels, bbox, parameters.infile,
        SampleFormat::UINT, bitspersample, Photometric::RGB, Compression::NONE,
        data
                );
}

void Jp2DriverOpenJpeg::information()
{
    LOGGER_DEBUG ("Not yet implemented !");
}


