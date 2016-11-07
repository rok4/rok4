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
 * \li LibpngImage : gestion d'une image au format PNG, en lecture, utilisant la librairie libpng
 * \li LibpngImageFactory : usine de création d'objet LibpngImage
 ** \~english
 * \brief Implement classes LibpngImage and LibpngImageFactory
 * \details
 * \li LibpngImage : manage a PNG format image, reading, using the library libpng
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


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibpngImage* LibpngImageFactory::createLibpngImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {


    png_structp pngStruct;
    png_infop pngInfo;
    png_bytep * pngData;

    png_byte header[8];    // 8 is the maximum size that can be checked

    // Ouverture du fichier bianire en lecture
    FILE *file = fopen ( filename, "rb" );
    if ( !file ) {
        LOGGER_ERROR ( "Unable to open the file (to read) " << filename );
        return NULL;
    }
    
    // Vérification de l'en-tête (8 octets), signature du PNG
    size_t size = fread ( header, 1, 8, file );
    if ( png_sig_cmp ( header, 0, 8 ) ) {
        LOGGER_ERROR ( "Provided file is not recognized as a PNG file " << filename );
        return NULL;
    }

    /* initialize stuff */
    pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!pngStruct) {
        LOGGER_ERROR ("Cannot create the PNG reading structure for image " << filename);
        return NULL;
    }

    pngInfo = png_create_info_struct(pngStruct);
    if (!pngInfo) {
        LOGGER_ERROR ("Cannot create the PNG informations structure for image " << filename);
        return NULL;
    }

    if (setjmp(png_jmpbuf(pngStruct))) {
        LOGGER_ERROR ("Error during PNG image initialization " << filename);
        return NULL;
    }

    // Initialisation des strcutures de lecture du PNG
    png_init_io(pngStruct, file);
    png_set_sig_bytes(pngStruct, 8);
    png_read_info(pngStruct, pngInfo);

    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0, bitspersample = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UINT;
    png_byte color_type, bit_depth;

    width = png_get_image_width(pngStruct, pngInfo);
    height = png_get_image_height(pngStruct, pngInfo);
    color_type = png_get_color_type(pngStruct, pngInfo);
    bit_depth = png_get_bit_depth(pngStruct, pngInfo);
    bitspersample = int(bit_depth);
    
    /********************** CONTROLES **************************/
    
    if ( ! LibpngImage::canRead ( bitspersample, sf ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" );
        LOGGER_ERROR ( "\t for the image to read : " << filename );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::dimensionsAreConsistent(resx, resy, width, height, bbox)) {
            LOGGER_ERROR ( "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }
    
    // Passage dans un format utilisable par la libimage

    switch ( int(color_type) ) {
        case PNG_COLOR_TYPE_GRAY :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_GRAY");
            channels = 1;
            
            if (bit_depth < 8) {
                png_set_expand_gray_1_2_4_to_8 (pngStruct);
                bitspersample = 8;
            }
            break;
        }
        case PNG_COLOR_TYPE_GRAY_ALPHA :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_GRAY_ALPHA");
            channels = 2;
            break;            
        }
        case PNG_COLOR_TYPE_RGB :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_RGB");
            channels = 3;
            break;
        }
        case PNG_COLOR_TYPE_PALETTE :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_PALETTE");
            channels = 3;
            png_set_palette_to_rgb (pngStruct);
            break;
        }
        case PNG_COLOR_TYPE_RGB_ALPHA :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_RGB_ALPHA");
            channels = 4;
            break;
        }
        default :
        {
            LOGGER_ERROR("Cannot interpret the color type (" << int(color_type) << ") for the PNG image " << filename);
            return NULL;
        }
    }
    
    if (png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS)) {
        LOGGER_DEBUG("Convert tRNS to alpha sample for PNG image");        
        png_set_tRNS_to_alpha(pngStruct);
        channels+=1;
    }
    
    png_read_update_info (pngStruct, pngInfo);

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/

    if (setjmp(png_jmpbuf(pngStruct))) {
        LOGGER_ERROR ("Error reading PNG image " << filename);
        return NULL;
    }

    pngData = (png_bytep*) malloc(sizeof(png_bytep) * height);
    int rowbytes = png_get_rowbytes(pngStruct,pngInfo);
    for (int y = 0; y < height; y++) {
        pngData[y] = (png_byte*) malloc(rowbytes);
    }

    png_read_image(pngStruct, pngData);

    fclose ( file );
    png_destroy_read_struct(&pngStruct, &pngInfo, (png_infopp)NULL);
    
    /******************** CRÉATION DE L'OBJET ******************/
    
    return new LibpngImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, toROK4Photometric ( color_type ), Compression::PNG,
        pngData
    );
    
}

void ReadDataFromInputStream(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead) {

   png_voidp io_ptr = png_get_io_ptr(png_ptr);
   if(io_ptr == NULL) {
       LOGGER_ERROR("Can't get io pointer");
       return;
    }

   RawDataStream& inputStream = *(RawDataStream*)io_ptr;
   const size_t bytesRead = inputStream.read((uint8_t*)outBytes,(size_t)byteCountToRead);

   if((png_size_t)bytesRead != byteCountToRead) {
        LOGGER_ERROR("Can't get data from stream");
        return;
    }
}

/* ----- Pour la lecture d'un buffer ----- */
LibpngImage* LibpngImageFactory::createLibpngImageToReadFromBuffer ( RawDataStream* rawStream, BoundingBox< double > bbox, double resx, double resy ) {

    png_structp pngStruct;
    png_infop pngInfo;
    png_bytep * pngData;

    png_byte header[8];    // 8 is the maximum size that can be checked

    // Vérification de l'en-tête (8 octets), signature du PNG
    size_t read = rawStream->read(header,8);
    if (read < 0) {
        LOGGER_ERROR ( "Can't read stream");
        return NULL;
    }
    if ( png_sig_cmp ( header, 0, 8 ) ) {
        LOGGER_ERROR ( "Provided buffer is not recognized as a PNG source ");
        return NULL;
    }

    /* initialize stuff */
    pngStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!pngStruct) {
        LOGGER_ERROR ("Cannot create the PNG reading structure for image ");
        return NULL;
    }

    pngInfo = png_create_info_struct(pngStruct);
    if (!pngInfo) {
        LOGGER_ERROR ("Cannot create the PNG informations structure for image ");
        return NULL;
    }

    if (setjmp(png_jmpbuf(pngStruct))) {
        LOGGER_ERROR ("Error during PNG image initialization ");
        return NULL;
    }

    // Initialisation des strcutures de lecture du PNG
    png_set_read_fn(pngStruct, rawStream, ReadDataFromInputStream);
    png_set_sig_bytes(pngStruct, 8);
    png_read_info(pngStruct, pngInfo);

    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0, bitspersample = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UINT;
    png_byte color_type, bit_depth;

    width = png_get_image_width(pngStruct, pngInfo);
    height = png_get_image_height(pngStruct, pngInfo);
    color_type = png_get_color_type(pngStruct, pngInfo);
    bit_depth = png_get_bit_depth(pngStruct, pngInfo);
    bitspersample = int(bit_depth);

    /********************** CONTROLES **************************/

    if ( ! LibpngImage::canRead ( bitspersample, sf ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" );
        LOGGER_ERROR ( "\t for the image to read : ");
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::dimensionsAreConsistent(resx, resy, width, height, bbox)) {
            LOGGER_ERROR ( "Resolutions, bounding box and real dimensions for image are not consistent" );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    // Passage dans un format utilisable par la libimage

    switch ( int(color_type) ) {
        case PNG_COLOR_TYPE_GRAY :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_GRAY");
            channels = 1;

            if (bit_depth < 8) {
                png_set_expand_gray_1_2_4_to_8 (pngStruct);
                bitspersample = 8;
            }
            break;
        }
        case PNG_COLOR_TYPE_GRAY_ALPHA :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_GRAY_ALPHA");
            channels = 2;
            break;
        }
        case PNG_COLOR_TYPE_RGB :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_RGB");
            channels = 3;
            break;
        }
        case PNG_COLOR_TYPE_PALETTE :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_PALETTE");
            channels = 3;
            png_set_palette_to_rgb (pngStruct);
            break;
        }
        case PNG_COLOR_TYPE_RGB_ALPHA :
        {
            LOGGER_DEBUG("Initial PNG color type PNG_COLOR_TYPE_RGB_ALPHA");
            channels = 4;
            break;
        }
        default :
        {
            LOGGER_ERROR("Cannot interpret the color type (" << int(color_type) << ") for the PNG image ");
            return NULL;
        }
    }

    if (png_get_valid(pngStruct, pngInfo, PNG_INFO_tRNS)) {
        LOGGER_DEBUG("Convert tRNS to alpha sample for PNG image");
        png_set_tRNS_to_alpha(pngStruct);
        channels+=1;
    }

    png_read_update_info (pngStruct, pngInfo);

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/

    if (setjmp(png_jmpbuf(pngStruct))) {
        LOGGER_ERROR ("Error reading PNG image ");
        return NULL;
    }

    pngData = (png_bytep*) malloc(sizeof(png_bytep) * height);
    int rowbytes = png_get_rowbytes(pngStruct,pngInfo);
    for (int y = 0; y < height; y++) {
        pngData[y] = (png_byte*) malloc(rowbytes);
    }

    png_read_image(pngStruct, pngData);

    png_destroy_read_struct(&pngStruct, &pngInfo, (png_infopp)NULL);

    /******************** CRÉATION DE L'OBJET ******************/

    return new LibpngImage (
        width, height, resx, resy, channels, bbox, "",
        sf, bitspersample, toROK4Photometric ( color_type ), Compression::PNG,
        pngData
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibpngImage::LibpngImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    png_bytep* pngData ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression, ExtraSample::ALPHA_UNASSOC ),

    data(pngData) {
        
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibpngImage::_getline ( T* buffer, int line ) {

    T buffertmp[width * channels];

    for (int x = 0;  x < width * channels; x++) {
        buffertmp[x] = data[line][x];
    }

    /******************** SI PIXEL CONVERTER ******************/

    if (converter) {
        converter->convertLine(buffer, buffertmp);
    } else {
        memcpy(buffer, buffertmp, pixelSize * width);
    }
    
    return width * getChannels();
}


int LibpngImage::getline ( uint8_t* buffer, int line ) {
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

int LibpngImage::getline ( uint16_t* buffer, int line ) {
    
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

int LibpngImage::getline ( float* buffer, int line ) {
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



