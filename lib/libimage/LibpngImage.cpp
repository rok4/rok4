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
 * \file LibpngImage.cpp
 ** \~french
 * \brief Implémentation des classes LibpngImage et LibpngImageFactory
 * \details
 * \li LibpngImage : image physique, attaché à un fichier
 * \li LibpngImageFactory : usine de création d'objet LibpngImage
 ** \~english
 * \brief Implement classes LibpngImage and LibpngImageFactory
 * \details
 * \li LibpngImage : physical image, linked to a file
 * \li LibpngImageFactory : factory to create LibpngImage object
 */

#include "LibpngImage.h"
#include "Logger.h"
#include "Utils.h"

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */

static Photometric::ePhotometric toROK4Photometric ( png_byte ph ) {
    switch ( ph ) {
    case PNG_COLOR_TYPE_GRAY :
        return Photometric::GRAY;
    case PNG_COLOR_TYPE_GRAY_ALPHA :
        return Photometric::GRAY;
    case PNG_COLOR_TYPE_PALETTE :
        return Photometric::RGB;
    case PNG_COLOR_TYPE_RGB :
        return Photometric::RGB;
    case PNG_COLOR_TYPE_RGB_ALPHA :
        return Photometric::RGB;
    default :
        return Photometric::UNKNOWN;
    }
}

static int photometricToChannel ( png_byte ph ) {
    switch ( ph ) {
    case PNG_COLOR_TYPE_GRAY :
        return 1;
    case PNG_COLOR_TYPE_GRAY_ALPHA :
        return 2;
    case PNG_COLOR_TYPE_RGB :
        return 3;
    case PNG_COLOR_TYPE_PALETTE :
        return 3;
    case PNG_COLOR_TYPE_RGB_ALPHA :
        return 4;
    default :
        return 0;
    }
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibpngImage* LibpngImageFactory::createLibpngImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {


    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_bytep * row_pointers;

    png_byte header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *file = fopen ( filename, "rb" );
    if ( !file ) {
        LOGGER_ERROR ( "Unable to open PNG (to read) " << filename );
        return NULL;
    }
    
    size_t size = fread ( header, 1, 8, file );
    if ( png_sig_cmp ( header, 0, 8 ) ) {
        LOGGER_ERROR ( "Provided file is not recognized as a PNG file" << filename );
        return NULL;
    }

    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr) {
        LOGGER_ERROR ("[read_png_file] png_create_read_struct failed");
        return NULL;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        LOGGER_ERROR ("[read_png_file] png_create_info_struct failed");
        return NULL;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        LOGGER_ERROR ("[read_png_file] Error during init_io");
        return NULL;
    }

    png_init_io(png_ptr, file);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);

    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0, bitspersample = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UINT;
    png_byte color_type, bit_depth;

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    color_type = png_get_color_type(png_ptr, info_ptr);
    channels = photometricToChannel(color_type);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);
    bitspersample = int(bit_depth);

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/

    if (setjmp(png_jmpbuf(png_ptr))) {
        LOGGER_ERROR ("[read_png_file] Error during read_image");
        return NULL;
    }

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++)
        row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

    png_read_image(png_ptr, row_pointers);

    fclose ( file );

    /********************** CONTROLES **************************/

    if ( ! SampleFormat::isHandledSampleType ( sf, bitspersample ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
        // Arrondi a la valeur entiere la plus proche
        int calcWidth = lround ( ( bbox.xmax - bbox.xmin ) / ( resx ) );
        int calcHeight = lround ( ( bbox.ymax - bbox.ymin ) / ( resy ) );
        if ( calcWidth != width || calcHeight != height ) {
            LOGGER_ERROR ( "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" );
            LOGGER_ERROR ( "Height is " << height << " and calculation give " << calcHeight );
            LOGGER_ERROR ( "Width is " << width << " and calculation give " << calcWidth );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    return new LibpngImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, toROK4Photometric ( color_type ), Compression::PNG,
        file, png_ptr, info_ptr, row_pointers
    );
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibpngImage::LibpngImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    FILE* file, png_structp png_ptr, png_infop info_ptr, png_bytep* row_pointers ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),

    fp ( file ), png_ptr(png_ptr), info_ptr(info_ptr), row_pointers(row_pointers) {
        
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibpngImage::_getline ( T* buffer, int line ) {
    // le buffer est déjà alloue
    // Cas RGB : canaux entrelaces (TIFFTAG_PLANARCONFIG=PLANARCONFIG_CONTIG)
/*
    if ( compression == Compression::NONE || ( compression != Compression::NONE && rowsperstrip == 1 ) ) {
        // Cas Non compresse ou (compresse et 1 ligne/bande)
        if ( TIFFReadScanline ( tif,buffer,line,0 ) < 0 ) {
            LOGGER_DEBUG ( "Cannot read file " << TIFFFileName ( tif ) << ", line " << line );
        }
    } else {
        // Cas compresse et > 1 ligne /bande
        if ( line / rowsperstrip != current_strip ) {
            current_strip = line / rowsperstrip;
            if ( TIFFReadEncodedStrip ( tif,current_strip,strip_buffer,strip_size ) < 0 ) {
                LOGGER_DEBUG ( "Cannot read file " << TIFFFileName ( tif ) << ", line " << line );
            }
        }
        memcpy ( buffer,&strip_buffer[ ( line%rowsperstrip ) *width*channels],width*channels*sizeof ( uint8_t ) );
    }*/

    //png_read_row (png_ptr, row, NULL);

    //png_read_rows(

    //png_byte* row = ;
    
    for (int x = 0;  x < width; x++) {
        //LOGGER_DEBUG ("x = " << x);
        //png_byte* ptr = &(row_pointers[line][x*channels]);
        //LOGGER_DEBUG ("Pixel values: " << int(ptr[0]) << "," << int(ptr[1]) << "," << int(ptr[2]));
        
        for (int c = 0;  c < channels; c++) {
            //LOGGER_DEBUG ("c = " << c);
            buffer[x*channels + c] = (uint8_t) row_pointers[line][x*channels + c];
        }
    }

    //delete [] row;

    //buffer = (uint8_t*) row_pointers[line];
    return width*channels;
}

int LibpngImage::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer,line );
}

int LibpngImage::getline ( float* buffer, int line ) {
    
    // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
    uint8_t* buffer_t = new uint8_t[width*channels];
    getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
    
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- ECRITURE ------------------------------------------- */

int LibpngImage::writeImage ( Image* pIn ) {
    LOGGER_ERROR ( "Cannot write PNG image" );
    return -1;
}

