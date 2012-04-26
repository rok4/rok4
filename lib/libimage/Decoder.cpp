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


#include "Decoder.h"
#include <setjmp.h>
#include "Logger.h"

#include "jpeglib.h"
#include "zlib.h"
#include "byteswap.h"
#include "lzwDecoder.h"

/*
 * Fonctions déclarées pour la libjpeg
 */
void init_source (jpeg_decompress_struct *cinfo) {}

boolean fill_input_buffer (jpeg_decompress_struct *cinfo) {
    return false;
}

void skip_input_data (jpeg_decompress_struct *cinfo, long num_bytes) {
    if (num_bytes > 0) {
        cinfo->src->next_input_byte += (size_t) num_bytes;
        cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
    }
}

void term_source (jpeg_decompress_struct *cinfo) {}

// Gestion des erreurs fatales pour la libjpeg

/*
 * Structure destinee a gerer les erreurs de la libjepg
 */

struct my_error_mgr {
    struct jpeg_error_mgr pub;    /* "public" fields */

    jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}


const uint8_t* JpegDecoder::decode(DataSource* source, size_t &size) {
    size = 0;
    if (!source) return 0;

    size_t encSize;
    const uint8_t* encData = source->getData(encSize);

    if (!encData) return 0;

    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;

    // Initialisation de cinfo
    jpeg_create_decompress(&cinfo);

    cinfo.src = new jpeg_source_mgr;
    cinfo.src->init_source = init_source;
    cinfo.src->fill_input_buffer = fill_input_buffer;
    cinfo.src->skip_input_data = skip_input_data;
    cinfo.src->resync_to_restart = jpeg_resync_to_restart; // use default method
    cinfo.src->term_source = term_source;

    cinfo.src->next_input_byte = encData;
    cinfo.src->bytes_in_buffer = encSize;

    // Initialisation de la gestion des erreurs
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    // Initialisation du contexte retourne par setjmp pour my_error_exit
    if (setjmp(jerr.setjmp_buffer)) {
        delete cinfo.src;
        jpeg_destroy_decompress(&cinfo);
        return 0;
    }

    uint8_t* raw_data = 0;

    // Lecture
    if (jpeg_read_header(&cinfo, TRUE)==JPEG_HEADER_OK) {

        int linesize = cinfo.image_width * cinfo.num_components;
        size = linesize * cinfo.image_height;
        raw_data = new uint8_t[size];

        // TODO: définir J_COLOR_SPACE out_color_space en fonction du nombre de canal ?
        // Vérifier que le jpeg monocanal marche ???

        jpeg_start_decompress(&cinfo);
        while (cinfo.output_scanline < cinfo.image_height) {
            uint8_t *line = raw_data + cinfo.output_scanline * linesize;

            if (jpeg_read_scanlines(&cinfo, &line, 1) < 1) {
                jpeg_finish_decompress(&cinfo);
                delete cinfo.src;
                jpeg_destroy_decompress(&cinfo);
                LOGGER_ERROR("Probleme lecture tuile Jpeg");
                delete[] raw_data;
                size = 0;
                return 0;
            }

        }

        jpeg_finish_decompress(&cinfo);
    }
    else
        LOGGER_ERROR("Erreur de lecture en tete jpeg");


    // Destruction de cinfo
    delete cinfo.src;
    jpeg_destroy_decompress(&cinfo);
    return raw_data;
}

/**
 * Decodage de donnee PNG
 */
const uint8_t* PngDecoder::decode(DataSource* source, size_t &size) {

//      LOGGER(DEBUG) << (intptr_t) source << std::endl;
    size = 0;
    if (!source) return 0;

    size_t encSize;
    const uint8_t* encData = source->getData(encSize);

    if (!encData) return 0;

    // Initialisation du flux
    z_stream zstream;
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.data_type = Z_BINARY;
    int zinit;
    if ( (zinit=inflateInit(&zstream)) != Z_OK)
    {
        if (zinit==Z_MEM_ERROR)
            LOGGER_ERROR("Decompression PNG : pas assez de memoire");
        else if (zinit==Z_VERSION_ERROR)
            LOGGER_ERROR("Decompression PNG : versions de zlib incompatibles");
        else if (zinit==Z_STREAM_ERROR)
            LOGGER_ERROR("Decompression PNG : parametres invalides");
        else
            LOGGER_ERROR("Decompression PNG : echec");
        return 0;
    }

    int height = bswap_32(*((const uint32_t*)(encData + 16)));
    int width = bswap_32(*((const uint32_t*)(encData + 20)));
    // TODO: vérifier cohérence header...
    int channels;

//      TODO if(encData[24] != 8) ERROR

    switch (encData[25]) {
    case 0:
        channels = 1;
        break;
    case 2:
        channels = 3;
        break;
    default:; // TODO ERROR;
    }

    uint8_t* raw_data = new uint8_t[height * width * channels];
    int linesize = width * channels;

    zstream.next_in = (uint8_t*)(encData + 41); // 41 = 33 header + 8(chunk idat)
    zstream.avail_in = encSize - 57;     // 57 = 41 + 4(crc) + 12(IEND)

    // Decompression du flux ligne par ligne
    uint8_t tmp;
    for (int h = 0; h < height; h++) {
        zstream.next_out = &tmp;
        zstream.avail_out = 1;
        // Decompression 1er octet de la ligne (=0 dans le cache)
        if (inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) {
            LOGGER_ERROR("Decompression PNG : probleme png decompression au debut de la ligne " << h);
            delete[] raw_data;
            return 0;
        }
        // Decompression des pixels de la ligne
        zstream.next_out = (uint8_t*) (raw_data + h*linesize);
        zstream.avail_out = linesize * sizeof(uint8_t);
        if (int err = inflate(&zstream, Z_SYNC_FLUSH)) {

            if (err == Z_STREAM_END && h == height-1) break; // fin du fichier OK.

            LOGGER_ERROR("Decompression PNG : probleme png decompression des pixels de la ligne " << h << " " << err);
            delete[] raw_data;
            return 0;
        }
    }
    // Destruction du flux
    if (inflateEnd(&zstream)!=Z_OK) {
        LOGGER_ERROR("Decompression PNG : probleme de liberation du flux");
        delete[] raw_data;
        return 0;
    }

    size = width * height * channels;

    return raw_data;
}

/**
 * Decodage de donnee LZW
 */
const uint8_t* LzwDecoder::decode(DataSource* source, size_t& size)
{
    size = 0;
    if (!source) return 0;

    size_t encSize;
    const uint8_t* encData = source->getData(encSize);

    if (!encData) return 0;

    // Initialisation du flux
    lzwDecoder decoder(12);
    uint8_t* raw_data = decoder.decode(encData,encSize,size);

    if (!raw_data) return 0;

    return raw_data;
}


/**
 * Decodage de donnee DEFLATE
 */
const uint8_t* DeflateDecoder::decode(DataSource* source, size_t &size) {

    size = 0;
    if (!source) return 0;

    size_t encSize;
    const uint8_t* encData = source->getData(encSize);

    if (!encData) return 0;

    // Initialisation du flux
    z_stream zstream;
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.data_type = Z_BINARY;
    int zinit;
    if ( (zinit=inflateInit(&zstream)) != Z_OK)
    {
        if (zinit==Z_MEM_ERROR)
            LOGGER_ERROR("Decompression DEFLATE : pas assez de memoire");
        else if (zinit==Z_VERSION_ERROR)
            LOGGER_ERROR("Decompression DEFLATE : versions de zlib incompatibles");
        else if (zinit==Z_STREAM_ERROR)
            LOGGER_ERROR("Decompression DEFLATE : parametres invalides");
        else
            LOGGER_ERROR("Decompression DEFLATE : echec");
        return 0;
    }
    
    size_t rawSize = encSize * 2;
    uint8_t* raw_data = new uint8_t[rawSize];
    
    zstream.next_in = (uint8_t*)(encData);
    zstream.avail_in = encSize;
    zstream.next_out = (uint8_t*) (raw_data);
    zstream.avail_out = rawSize;
    // Decompression du flux
    while ( zstream.avail_in != 0 ) {
        if (int err = inflate(&zstream, Z_SYNC_FLUSH)) {
            if (err == Z_STREAM_END && zstream.avail_in == 0) break; // fin du fichier OK.
            if (zstream.avail_out == 0) { // Output buffer Full
                uint8_t* tmp = new uint8_t[rawSize *2];
                memcpy(tmp,raw_data,rawSize);
                delete[] raw_data;
                raw_data = tmp;
                zstream.next_out = (uint8_t*) (raw_data + rawSize);
                zstream.avail_out += rawSize;
                rawSize *=2;
            }
            LOGGER_ERROR("Decompression DEFLATE : probleme deflate decompression " << err);
            delete[] raw_data;
            size = 0;
            return 0;
        }
    }

    // Destruction du flux
    if (inflateEnd(&zstream)!=Z_OK) {
        LOGGER_ERROR("Decompression DEFLATE : probleme de liberation du flux");
        delete[] raw_data;
        size = 0;
        return 0;
    }

    size = rawSize - zstream.avail_out;

    return raw_data;
}


int ImageDecoder::getDataline(uint8_t* buffer, int line) {
    convert(buffer, rawData + ((margin_top + line) * source_width + margin_left) * channels, width * channels);
    return width * channels;
}

int ImageDecoder::getDataline(float* buffer, int line) {
    // Cas typique : chargement d'une ligne d'une image avec des pixels de type uint8_t pour interpolation
    // Conversion des uint8_t en float
    if (pixel_size==1)
        convert(buffer, rawData + ((margin_top + line) * source_width + margin_left) * channels, width * channels);
    // Cas typique : chargement d'une ligne d'une image avec des pixels de type float
    // Pas de conversion des floats
    else if (pixel_size==4)
        memcpy(buffer,rawData + ((margin_top + line) * source_width + margin_left) * channels*sizeof(float),width * channels*sizeof(float));

    return width * channels;
}
