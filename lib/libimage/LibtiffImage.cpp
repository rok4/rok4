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
 * \file LibtiffImage.cpp
 ** \~french
 * \brief Implémentation des classes LibtiffImage et LibtiffImageFactory
 * \details
 * \li LibtiffImage : image physique, attaché à un fichier
 * \li LibtiffImageFactory : usine de création d'objet LibtiffImage
 ** \~english
 * \brief Implement classes LibtiffImage and LibtiffImageFactory
 * \details
 * \li LibtiffImage : physical image, linked to a file
 * \li LibtiffImageFactory : factory to create LibtiffImage object
 */

#include "LibtiffImage.h"
#include "Logger.h"
#include "Utils.h"
#include "OneBitConverter.h"


/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */


static SampleFormat::eSampleFormat toROK4SampleFormat ( uint16_t sf ) {
    switch ( sf ) {
    case SAMPLEFORMAT_UINT :
        return SampleFormat::UINT;
    case SAMPLEFORMAT_IEEEFP :
        return SampleFormat::FLOAT;
    default :
        return SampleFormat::UNKNOWN;
    }
}

static uint16_t fromROK4SampleFormat ( SampleFormat::eSampleFormat sf ) {
    switch ( sf ) {
    case SampleFormat::UINT :
        return SAMPLEFORMAT_UINT;
    case SampleFormat::FLOAT :
        return SAMPLEFORMAT_IEEEFP;
    default :
        return 0;
    }
}

static Photometric::ePhotometric toROK4Photometric ( uint16_t ph ) {
    switch ( ph ) {
    case PHOTOMETRIC_MINISBLACK :
        return Photometric::GRAY;
    case PHOTOMETRIC_MINISWHITE :
        return Photometric::GRAY;
    case PHOTOMETRIC_RGB :
        return Photometric::RGB;
    case PHOTOMETRIC_YCBCR :
        return Photometric::YCBCR;
    case PHOTOMETRIC_MASK :
        return Photometric::MASK;
    default :
        return Photometric::UNKNOWN;
    }
}

static uint16_t fromROK4Photometric ( Photometric::ePhotometric ph ) {
    switch ( ph ) {
    case Photometric::GRAY :
        return PHOTOMETRIC_MINISBLACK;
    case Photometric::RGB :
        return PHOTOMETRIC_RGB;
    case Photometric::YCBCR :
        return PHOTOMETRIC_YCBCR;
    case Photometric::MASK :
        return PHOTOMETRIC_MINISBLACK;
    default :
        return 0;
    }
}

static Compression::eCompression toROK4Compression ( uint16_t comp ) {
    switch ( comp ) {
    case COMPRESSION_NONE :
        return Compression::NONE;
    case COMPRESSION_ADOBE_DEFLATE :
        return Compression::DEFLATE;
    case COMPRESSION_JPEG :
        return Compression::JPEG;
    case COMPRESSION_DEFLATE :
        return Compression::DEFLATE;
    case COMPRESSION_LZW :
        return Compression::LZW;
    case COMPRESSION_PACKBITS :
        return Compression::PACKBITS;
    default :
        return Compression::UNKNOWN;
    }
}

static uint16_t fromROK4Compression ( Compression::eCompression comp ) {
    switch ( comp ) {
    case Compression::NONE :
        return COMPRESSION_NONE;
    case Compression::DEFLATE :
        return COMPRESSION_ADOBE_DEFLATE;
    case Compression::JPEG :
        return COMPRESSION_JPEG;
    case Compression::PNG :
        return COMPRESSION_ADOBE_DEFLATE;
    case Compression::LZW :
        return COMPRESSION_LZW;
    case Compression::PACKBITS :
        return COMPRESSION_PACKBITS;
    default :
        return 0;
    }
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibtiffImage* LibtiffImageFactory::createLibtiffImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {

    int width=0, height=0, channels=0, planarconfig=0, bitspersample=0, sf=0, ph=0, comp=0, rowsperstrip=0;
    TIFF* tif = TIFFOpen ( filename, "r" );
    bool associatedAlpha = false;
    
    /************** RECUPERATION DES INFORMATIONS **************/

    if ( tif == NULL ) {
        LOGGER_ERROR ( "Unable to open TIFF (to read) " << filename );
        return NULL;
    }
    
    if ( TIFFGetField ( tif, TIFFTAG_IMAGEWIDTH, &width ) < 1 ) {
        LOGGER_ERROR ( "Unable to read pixel width for file " << filename );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_IMAGELENGTH, &height ) < 1 ) {
        LOGGER_ERROR ( "Unable to read pixel height for file " << filename );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_SAMPLESPERPIXEL,&channels ) < 1 ) {
        LOGGER_ERROR ( "Unable to read number of samples per pixel for file " << filename );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_PLANARCONFIG,&planarconfig ) < 1 ) {
        LOGGER_ERROR ( "Unable to read planar configuration for file " << filename );
        return NULL;
    }

    if ( planarconfig != PLANARCONFIG_CONTIG && channels != 1 ) {
        LOGGER_ERROR ( "Planar configuration have to be 'PLANARCONFIG_CONTIG' for file " << filename );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_BITSPERSAMPLE,&bitspersample ) < 1 ) {
        LOGGER_ERROR ( "Unable to read number of bits per sample for file " << filename );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_SAMPLEFORMAT,&sf ) < 1 ) {
        if ( bitspersample == 8 ) {
            sf = SAMPLEFORMAT_UINT;
        } else if ( bitspersample == 16 ) {
            sf = SAMPLEFORMAT_UINT;
        } else if ( bitspersample == 32 ) {
            sf = SAMPLEFORMAT_IEEEFP;
        } else if ( bitspersample == 1 ) {
            sf = SAMPLEFORMAT_UINT;
        } else {
            LOGGER_ERROR ( "Unable to determine sample format from the number of bits per sample (" << bitspersample << ") for file " << filename );
            return NULL;
        }
    }

    if ( TIFFGetField ( tif, TIFFTAG_PHOTOMETRIC,&ph ) < 1 ) {
        LOGGER_ERROR ( "Unable to read photometric for file " << filename );
        return NULL;
    }
    
    if (toROK4Photometric ( ph ) == 0) {
        LOGGER_ERROR ( "Not handled photometric (PALETTE ?) for file " << filename );
        return NULL;            
    }

    if ( TIFFGetField ( tif, TIFFTAG_COMPRESSION,&comp ) < 1 ) {
        LOGGER_ERROR ( "Unable to read compression for file " << filename );
        return NULL;
    }

    if ( TIFFGetField ( tif, TIFFTAG_ROWSPERSTRIP,&rowsperstrip ) < 1 ) {
        LOGGER_ERROR ( "Unable to read number of rows per strip for file " << filename );
        return NULL;
    }

    uint16_t extrasamplesCount;
    uint16_t* extrasamples;
    if ( TIFFGetField ( tif, TIFFTAG_EXTRASAMPLES, &extrasamplesCount, &extrasamples ) > 0 ) {
        // On a des canaux en plus, si c'est de l'alpha (le premier extra), et qu'il est associé,
        // on le précise pour convertir à la volée lors de la lecture des lignes
        if ( extrasamples[0] == EXTRASAMPLE_ASSOCALPHA ) {
            LOGGER_INFO ( "Alpha sample is associated for the file " << filename << ". We will convert for reading");
            associatedAlpha = true;
        }
    }
    
    /********************** CONTROLES **************************/

    if ( ! LibtiffImage::canRead ( bitspersample, toROK4SampleFormat ( sf ) ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( toROK4SampleFormat ( sf ) ) << " and " << bitspersample << " bits per sample" );
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
    
    /******************** CRÉATION DE L'OBJET ******************/

    return new LibtiffImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, comp,
        tif, rowsperstrip, associatedAlpha
    );
}


/* ----- Pour l'écriture ----- */

LibtiffImage* LibtiffImageFactory::createLibtiffImageToWrite (
    char* filename, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
    Compression::eCompression compression, uint16_t rowsperstrip ) {

    if (compression == Compression::JPEG && photometric == Photometric::RGB)
        photometric = Photometric::YCBCR;

    if (compression != Compression::JPEG && photometric == Photometric::YCBCR)
        photometric = Photometric::RGB;

    if ( width <= 0 || height <= 0 ) {
        LOGGER_ERROR ( "One dimension is not valid for the output image " << filename << " : " << width << ", " << height );
        return NULL;
    }
    
    if ( channels <= 0 ) {
        LOGGER_ERROR ( "Number of samples per pixel is not valid for the output image " << filename << " : " << channels );
        return NULL;
    }
    
    if ( ! LibtiffImage::canWrite ( bitspersample, sampleformat ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sampleformat ) << " and " << bitspersample << " bits per sample" );
        LOGGER_ERROR ( "\t for the image to write : " << filename );
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
    }

    TIFF* tif = TIFFOpen ( filename, "w" );
    if ( tif == NULL ) {
        LOGGER_ERROR ( "Unable to open TIFF (to write) " << filename );
        return NULL;
    }

    // Ecriture de l'en-tête pour récupérer les informations sur l'image
    if ( TIFFSetField ( tif, TIFFTAG_IMAGEWIDTH, width ) < 1 ) {
        LOGGER_ERROR ( "Unable to write pixel width for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_IMAGELENGTH, height ) < 1 ) {
        LOGGER_ERROR ( "Unable to write pixel height for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_SAMPLESPERPIXEL,channels ) < 1 ) {
        LOGGER_ERROR ( "Unable to write number of samples per pixel for file " << filename );
        return NULL;
    }

    if ( channels == 4 || channels == 2 ) {
        uint16_t extrasample = EXTRASAMPLE_UNASSALPHA;
        if ( TIFFSetField ( tif, TIFFTAG_EXTRASAMPLES,1,&extrasample ) < 1 ) {
            LOGGER_ERROR ( "Unable to write number of extra samples for file " << filename );
            return NULL;
        }
    }

    if ( TIFFSetField ( tif, TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG ) < 1 ) {
        LOGGER_ERROR ( "Unable to write planar configuration for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_BITSPERSAMPLE, bitspersample ) < 1 ) {
        LOGGER_ERROR ( "Unable to write number of bits per sample for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_SAMPLEFORMAT, fromROK4SampleFormat ( sampleformat ) ) < 1 ) {
        LOGGER_ERROR ( "Unable to write sample format for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_PHOTOMETRIC, fromROK4Photometric ( photometric ) ) < 1 ) {
        LOGGER_ERROR ( "Unable to write photometric for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_COMPRESSION, fromROK4Compression ( compression ) ) < 1 ) {
        LOGGER_ERROR ( "Unable to write compression for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_ROWSPERSTRIP,rowsperstrip ) < 1 ) {
        LOGGER_ERROR ( "Unable to write number of rows per strip for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE ) < 1 ) {
        LOGGER_ERROR ( "Unable to write pixel resolution unit for file " << filename );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::dimensionsAreConsistent(resx, resy, width, height, bbox)) {
            LOGGER_ERROR ( "Resolutions, bounding box and dimensions for image (to write)'" << filename << "' are not consistent" );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    return new LibtiffImage (
        width, height, resx, resy, channels, bbox, filename,
        sampleformat, bitspersample, photometric, compression,
        tif, rowsperstrip, false
    );
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEURS ---------------------------------------- */

LibtiffImage::LibtiffImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    int sf, int bps, int ph,
    int comp, TIFF* tif, int rowsperstrip, bool associatedalpha) :

    FileImage ( width, height, resx, resy, channels, bbox, name, toROK4SampleFormat( sf ),
                bps, toROK4Photometric( ph ), toROK4Compression( comp ), associatedalpha
              ),

    tif ( tif ), rowsperstrip ( rowsperstrip ) {
    
    // Ce constructeur permet de déterminer si la conversion de 1 à 8 bits est nécessaire, et de savoir si 0 est blanc ou noir
    
    if ( bps == 1 && sampleformat == SampleFormat::UINT) {
        // On fera la conversion en entiers sur 8 bits à la volée.
        // Cette image sera donc comme une image sur 8 bits.
        // On change donc les informations, en précisant que la conversion doit être faite à la lecture.
        LOGGER_DEBUG ( "We have 1-bit samples for the file " << filename << ". We will convert for reading into 8-bit samples");
        bitspersample = 8;
        pixelSize = channels;
        if (ph == PHOTOMETRIC_MINISWHITE) oneTo8bits = 1;
        else if (ph == PHOTOMETRIC_MINISBLACK) oneTo8bits = 2;
        else {
            LOGGER_WARN("Image '" << filename << "' has 1-bit sample and is not PHOTOMETRIC_MINISWHITE or PHOTOMETRIC_MINISWHITE ?");
            oneTo8bits = 0;
        }
    } else {
        oneTo8bits = 0;
    }

    current_strip = -1;
    int stripSize = width*rowsperstrip*pixelSize;
    strip_buffer = new uint8_t[stripSize];
    
    if (oneTo8bits) {
        // On a besoin d'un buffer supplémentaire pour faire la conversion à la volée à la lecture
        oneTo8bits_buffer = new uint8_t[stripSize];
    }
}

LibtiffImage::LibtiffImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
    Compression::eCompression compression, TIFF* tif, int rowsperstrip, bool associatedalpha) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression, associatedalpha ),

    tif ( tif ), rowsperstrip ( rowsperstrip ) {
        
    oneTo8bits = 0;

    current_strip = -1;
    int stripSize = width*rowsperstrip*pixelSize;
    strip_buffer = new uint8_t[stripSize];
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibtiffImage::_getline ( T* buffer, int line ) {
    // buffer doit déjà être alloué, et assez grand
    
    if ( line / rowsperstrip != current_strip ) {
        
        // Les données n'ont pas encore été lue depuis l'image (strip pas en mémoire).
        current_strip = line / rowsperstrip;
        int size = TIFFReadEncodedStrip ( tif, current_strip, strip_buffer, -1 );
        if ( size < 0 ) {
            LOGGER_ERROR ( "Cannot read strip number " << current_strip << " of image " << filename );
            return 0;
        }
        
        if (oneTo8bits == 1) {
            OneBitConverter::minwhiteToGray(oneTo8bits_buffer, strip_buffer, size);
        } else if (oneTo8bits == 2) {
            OneBitConverter::minblackToGray(oneTo8bits_buffer, strip_buffer, size);
        }
    }
    
    if (oneTo8bits) {
        memcpy ( buffer, oneTo8bits_buffer + ( line%rowsperstrip ) * width * pixelSize, width * pixelSize );
    } else {
        memcpy ( buffer, strip_buffer + ( line%rowsperstrip ) * width * pixelSize, width * pixelSize );
    }
    
    return width*channels;
}

int LibtiffImage::getline ( uint8_t* buffer, int line ) {
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        int r = _getline ( buffer,line );
        if (associatedalpha) unassociateAlpha ( buffer );
        return r;
    } else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) { // uint16
        /* On ne convertit pas les entiers 16 bits en entier sur 8 bits (aucun intérêt)
         * On va copier le buffer entier 16 bits sur le buffer entier, de même taille en octet (2 fois plus grand en "nombre de cases")*/
        uint16_t int16line[width * channels];
        _getline ( int16line, line );
        memcpy ( buffer, int16line, width*pixelSize );
        return width*pixelSize;
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) { // float
        /* On ne convertit pas les nombres flottants en entier sur 8 bits (aucun intérêt)
         * On va copier le buffer flottant sur le buffer entier, de même taille en octet (4 fois plus grand en "nombre de cases")*/
        float floatline[width * channels];
        _getline ( floatline, line );
        memcpy ( buffer, floatline, width*pixelSize );
        return width*pixelSize;
    }
}

int LibtiffImage::getline ( uint16_t* buffer, int line ) {
    
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en entier 16 bits mais l'image lue est sur 8 bits : on convertit
        uint8_t* buffer_t = new uint8_t[width*channels];
        _getline ( buffer_t,line );
        convert ( buffer,buffer_t,width*channels);
        delete [] buffer_t;
        return width*channels;
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

int LibtiffImage::getline ( float* buffer, int line ) {
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint8_t* buffer_t = new uint8_t[width*channels];
        
        // Ne pas appeler directement _getline pour bien faire les potentielles conversions (alpha ou 1 bit)
        getline ( buffer_t,line );
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        return width*channels;
    } else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) { // uint16
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint16_t* buffer_t = new uint16_t[width*channels];
        _getline ( buffer_t,line );
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        return width*channels;     
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) { // float
        return _getline ( buffer, line );
    }
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- ECRITURE ------------------------------------------- */

int LibtiffImage::writeImage ( Image* pIn ) {

    // Contrôle de la cohérence des 2 images : dimensions
    if ( width != pIn->getWidth() || height != pIn->getHeight() ) {
        LOGGER_ERROR ( "Image we want to write has not consistent dimensions with the output image" );
        return -1;
    }

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        uint8_t* buf_u = ( unsigned char* ) _TIFFmalloc ( width * channels );
        for ( int line = 0; line < height; line++ ) {
            pIn->getline ( buf_u,line );
            if ( TIFFWriteScanline ( tif, buf_u, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
        _TIFFfree ( buf_u );

    } else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) {
        uint16_t* buf_t = ( uint16_t* ) _TIFFmalloc ( width * pixelSize );
        for ( int line = 0; line < height; line++ ) {
            pIn->getline ( buf_t,line );
            if ( TIFFWriteScanline ( tif, buf_t, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
        _TIFFfree ( buf_t );
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        float* buf_f = ( float* ) _TIFFmalloc ( width * channels * sizeof(float) );
        for ( int line = 0; line < height; line++ ) {
            pIn->getline ( buf_f,line );
            if ( TIFFWriteScanline ( tif, buf_f, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
        _TIFFfree ( buf_f );
    }

    return 0;
}

int LibtiffImage::writeImage ( uint8_t* buffer) {
    
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }

    } else {
        LOGGER_ERROR ( "Image to write (from a buffer) has not 8-bit uint samples : " << filename);
        print();
        return -1;        
    }
    
    /* else if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) {
        uint16_t* buf_t = new uint16_t[width * channels];
        for ( int line = 0; line < height; line++ ) {
            convert ( buf_t, buffer + line * width * channels, width * channels );
            if ( TIFFWriteScanline ( tif, buf_t, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }

        delete []  buf_t ;
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        float* buf_f = new float[width * channels];
        for ( int line = 0; line < height; line++ ) {
            convert ( buf_f, buffer + line * width * channels, width * channels );
            if ( TIFFWriteScanline ( tif, buf_f, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }

        delete []  buf_f ;
    }*/

    return 0;
}

int LibtiffImage::writeImage ( uint16_t* buffer) {
    
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }

    } else {
        LOGGER_ERROR ( "Image to write (from a buffer) has not 16-bit uint samples : " << filename);
        print();
        return -1;        
    }

    return 0;
}

int LibtiffImage::writeImage ( float* buffer) {
    
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    /*if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        uint8_t* buf_u = new uint8_t[height * width * channels];
        convert ( buf_u, buffer, height*width*channels );
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buf_u + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }

        delete [] buf_u;

    } else */
    
    if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    } else {
        LOGGER_ERROR ( "Image to write (from a buffer) has not 32-bit float samples : " << filename);
        print();
        return -1;
    }

    return 0;
}

int LibtiffImage::writeLine ( uint8_t* buffer, int line) {
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        if ( TIFFWriteScanline ( tif, buffer, line, 0 ) < 0 ) {
            LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
            return -1;
        }

    } else {
        LOGGER_ERROR ( "Image to write (line by line) has not 8-bit uint samples : " << filename);
        print();
        return -1;        
    }
    
    
    /* else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        float* buf_f = new float[height * width * channels];
        convert ( buf_f, buffer, height*width*channels );

        if ( TIFFWriteScanline ( tif, buf_f, line, 0 ) < 0 ) {
            LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
            return -1;
        }

        delete []  buf_f ;
    }*/

    return 0;
}

int LibtiffImage::writeLine ( uint16_t* buffer, int line) {
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    if ( bitspersample == 16 && sampleformat == SampleFormat::UINT ) {
        if ( TIFFWriteScanline ( tif, buffer, line, 0 ) < 0 ) {
            LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
            return -1;
        }

    } else {
        LOGGER_ERROR ( "Image to write (line by line) has not 16-bit uint samples : " << filename);
        print();
        return -1;        
    }

    return 0;
}

int LibtiffImage::writeLine ( float* buffer, int line) {
    // Si l'image à écrire n'a pas des canaux en entiers sur 8 bits, on sort en erreur

    // Ecriture de l'image
    /*if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        uint8_t* buf_u = new uint8_t[height * width * channels];
        convert ( buf_u, buffer, height*width*channels );

        if ( TIFFWriteScanline ( tif, buf_u, line, 0 ) < 0 ) {
            LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
            return -1;
        }

        delete [] buf_u;

    } else */
    
    if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        if ( TIFFWriteScanline ( tif, buffer, line, 0 ) < 0 ) {
            LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
            return -1;
        }
    } else {
        LOGGER_ERROR ( "Image to write (line by line) has not 32-bit float samples : " << filename);
        print();
        return -1;
    }

    return 0;
}

