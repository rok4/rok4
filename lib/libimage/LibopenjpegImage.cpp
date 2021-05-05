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

/**
 * \file LibopenjpegImage.cpp
 ** \~french
 * \brief Implémentation des classes LibopenjpegImage et LibopenjpegImageFactory
 * \details
 * \li LibopenjpegImage : gestion d'une image au format JPEG2000, en lecture, utilisant la librairie openjpeg
 * \li LibopenjpegImageFactory : usine de création d'objet LibopenjpegImage
 ** \~english
 * \brief Implement classes LibopenjpegImage and LibopenjpegImageFactory
 * \details
 * \li LibopenjpegImage : manage a JPEG2000 format image, reading, using the library openjpeg
 * \li LibopenjpegImageFactory : factory to create LibopenjpegImage object
 */

#include <string.h>
#include <cstring>
#include <ctype.h>

#include "LibopenjpegImage.h"
#include <boost/log/trivial.hpp>
#include "Utils.h"

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */

static Photometric::ePhotometric toROK4Photometric ( OPJ_COLOR_SPACE ph, int channels ) {
    switch ( ph ) {
    case OPJ_CLRSPC_SRGB :
        return Photometric::RGB;
    case OPJ_CLRSPC_GRAY :
        return Photometric::GRAY;
    case OPJ_CLRSPC_UNSPECIFIED :
        switch(channels) {
            case 1:
                return Photometric::GRAY;
            case 2:
                return Photometric::GRAY;
            case 3:
                return Photometric::RGB;
            case 4:
                return Photometric::RGB;
            default:
                return Photometric::UNKNOWN;
        }
        
    default :
        return Photometric::UNKNOWN;
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------- LOGGERS DE OPENJPEG ------------------------------------- */

static void error_callback ( const char *msg, void *client_data ) {
    ( void ) client_data;
    BOOST_LOG_TRIVIAL(error) <<  msg ;
}

static void warning_callback ( const char *msg, void *client_data ) {
    ( void ) client_data;
    BOOST_LOG_TRIVIAL(warning) <<  msg ;
}

static void info_callback ( const char *msg, void *client_data ) {
    ( void ) client_data;
    BOOST_LOG_TRIVIAL(debug) <<  msg ;
}

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibopenjpegImage* LibopenjpegImageFactory::createLibopenjpegImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {

    // Set decoding parameters to default values
    opj_dparameters_t parameters;                   /* decompression parameters */
    opj_image_t* image = NULL;
    opj_stream_t *l_stream = NULL;                          /* Stream */
    opj_codec_t* l_codec = NULL;                            /* Handle to a decompressor */
    opj_codestream_index_t* cstr_index = NULL;

    opj_set_default_decoder_parameters ( &parameters );
    strncpy ( parameters.infile, filename, IMAGE_MAX_FILENAME_LENGTH * sizeof ( char ) );

    /************** INITIALISATION DES OBJETS OPENJPEG *********/

    // Ouverture du fichier bianire en lecture
    FILE *file = NULL;
    file = std::fopen ( filename , "rb" );

    if ( !file ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to open the JPEG2000 file (to read) " << filename ;
        return NULL;
    }

    // Récupération du format du JPEG2000 (magic code) pour savoir quel codec utiliser pour la décompression
    unsigned char * magic_code = ( unsigned char * ) malloc ( 12 );

    if ( std::fread ( magic_code, 1, 12, file ) != 12 ) {
        free ( magic_code );
        std::fclose ( file );
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read the magic code for the JPEG2000 file " << filename ;
        return NULL;
    }

    std::fclose ( file );

    // Format MAGIC Code
    if ( memcmp ( magic_code, JP2_RFC3745_MAGIC, 12 ) == 0 || memcmp ( magic_code, JP2_MAGIC, 4 ) == 0 ) {
        l_codec = opj_create_decompress ( OPJ_CODEC_JP2 );
        BOOST_LOG_TRIVIAL(debug) <<  "Ok, use format JP2 !" ;
    } else if ( memcmp ( magic_code, J2K_CODESTREAM_MAGIC, 4 ) == 0 ) {
        l_codec = opj_create_decompress ( OPJ_CODEC_J2K );
        BOOST_LOG_TRIVIAL(debug) <<  "Ok, use format J2K !" ;
    } else {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled format for the JPEG2000 file " << filename ;
        free ( magic_code );
        return NULL;
    }

    // Nettoyage
    free ( magic_code );

    /* catch events using our callbacks and give a local context */
    opj_set_info_handler ( l_codec, info_callback,00 );
    opj_set_warning_handler ( l_codec, warning_callback,00 );
    opj_set_error_handler ( l_codec, error_callback,00 );

    /* Setup the decoder decoding parameters using user parameters */
    if ( !opj_setup_decoder ( l_codec, &parameters ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to setup the decoder for the JPEG2000 file " << filename ;
        opj_destroy_codec ( l_codec );
        return NULL;
    }

    l_stream = opj_stream_create_default_file_stream ( filename,1 );
    if ( !l_stream ) {
        std::fclose ( file );
        BOOST_LOG_TRIVIAL(error) <<  "Unable to create the stream (to read) for the JPEG2000 file " << filename ;
        return NULL;
    }

    /* Read the main header of the codestream and if necessary the JP2 boxes*/
    if ( ! opj_read_header ( l_stream, l_codec, &image ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to read the header for the JPEG2000 file " << filename ;
        opj_stream_destroy ( l_stream );
        opj_destroy_codec ( l_codec );
        opj_image_destroy ( image );
        return NULL;
    }
    
    /************** RECUPERATION DES INFORMATIONS **************/

    // BitsPerSample
    int bitspersample = image->comps[0].prec;
    int width = image->comps[0].w;
    int height = image->comps[0].h;
    int channels = image->numcomps;
    SampleFormat::eSampleFormat sf = SampleFormat::UINT;
    Photometric::ePhotometric ph = toROK4Photometric ( image->color_space , channels);
    if ( ph == Photometric::UNKNOWN ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unhandled color space (" << image->color_space << ") in the JPEG2000 image " << filename ;
        return NULL;
    }

    // On vérifie que toutes les composantes ont bien les mêmes carctéristiques
    for ( int i = 1; i < channels; i++ ) {
        if ( bitspersample != image->comps[i].prec || width != image->comps[i].w || height != image->comps[i].h ) {
            BOOST_LOG_TRIVIAL(error) <<  "All components have to be the same in the JPEG image " << filename ;
            return NULL;
        }
    }

    /********************** CONTROLES **************************/

    if ( ! LibopenjpegImage::canRead ( bitspersample, sf ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" ;
        BOOST_LOG_TRIVIAL(error) <<  "\t for the image to read : " << filename ;
        return NULL;
    }
    
    if ( resx > 0 && resy > 0 ) {
        if (! Image::dimensionsAreConsistent(resx, resy, width, height, bbox)) {
            BOOST_LOG_TRIVIAL(error) <<  "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" ;
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }
    
    /************** LECTURE DE L'IMAGE EN ENTIER ***************/

    /* Get the decoded image */
    if ( ! ( opj_decode ( l_codec, l_stream, image ) && opj_end_decompress ( l_codec, l_stream ) ) ) {
        BOOST_LOG_TRIVIAL(error) <<  "Unable to decode JPEG2000 file " << filename ;
        opj_destroy_codec ( l_codec );
        opj_stream_destroy ( l_stream );
        opj_image_destroy ( image );
        return NULL;
    }

    opj_destroy_codec ( l_codec );
    opj_stream_destroy ( l_stream );

    /******************** CRÉATION DE L'OBJET ******************/

    return new LibopenjpegImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, Compression::JPEG2000,
        image
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibopenjpegImage::LibopenjpegImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    opj_image_t *jp2ptr ) :

    Jpeg2000Image ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),

    jp2image ( jp2ptr ) {

}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibopenjpegImage::_getline ( T* buffer, int line ) {

    T buffertmp[width * channels];

    for (int i = 0; i < width; i++) {
        int index = width * line + i;
        for (int j = 0; j < channels; j++) {
            buffertmp[i*channels + j] = jp2image->comps[j].data[index];
        }
    }

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convertLine(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixelSize * width);
    }
    
    return width * getChannels();
}


int LibopenjpegImage::getline ( uint8_t* buffer, int line ) {
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        return _getline ( buffer,line );
    } else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) { // uint16
        /* On ne convertit pas les entiers 16 bits en entier sur 8 bits (aucun intérêt)
         * On va copier le buffer entier 16 bits sur le buffer entier, de même taille en octet (2 fois plus grand en "nombre de cases")*/
        uint16_t int16line[width * getChannels()];
        _getline ( int16line, line );
        memcpy ( buffer, int16line, width * getPixelSize() );
        return width * getPixelSize();
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) { // float
        /* On ne convertit pas les nombres flottants en entier sur 8 bits (aucun intérêt)
         * On va copier le buffer flottant sur le buffer entier, de même taille en octet (4 fois plus grand en "nombre de cases")*/
        float floatline[width * getChannels()];
        _getline ( floatline, line );
        memcpy ( buffer, floatline, width * getPixelSize() );
        return width * getPixelSize();
    }
}

int LibopenjpegImage::getline ( uint16_t* buffer, int line ) {
    
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en entier 16 bits mais l'image lue est sur 8 bits : on convertit
        uint8_t* buffer_t = new uint8_t[width * getChannels()];
        _getline ( buffer_t,line );
        convert ( buffer, buffer_t, width * getChannels() );
        delete [] buffer_t;
        return width * getChannels();
    } else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) { // uint16
        return _getline ( buffer,line );        
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) { // float
        /* On ne convertit pas les nombres flottants en entier sur 16 bits (aucun intérêt)
        * On va copier le buffer flottant sur le buffer entier 16 bits, de même taille en octet (2 fois plus grand en "nombre de cases")*/
        float floatline[width * channels];
        _getline ( floatline, line );
        memcpy ( buffer, floatline, width*pixelSize );
        return width*pixelSize;
    }
}

int LibopenjpegImage::getline ( float* buffer, int line ) {
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint8_t* buffer_t = new uint8_t[width * getChannels()];
        _getline ( buffer_t,line );
        convert ( buffer, buffer_t, width * getChannels() );
        delete [] buffer_t;
        return width * getChannels();
    } else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) { // uint16
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint16_t* buffer_t = new uint16_t[width * getChannels()];
        _getline ( buffer_t,line );
        convert ( buffer, buffer_t, width * getChannels() );
        delete [] buffer_t;
        return width * getChannels();   
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) { // float
        return _getline ( buffer, line );
    }
}





