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
    case PHOTOMETRIC_RGB :
        return Photometric::RGB;
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

    int width=0, height=0, channels=0, planarconfig=0, bitspersample=0, sampleformat=0, photometric=0, compression=0, rowsperstrip=0;
    TIFF* tif = TIFFOpen ( filename, "r" );

    if ( tif == NULL ) {
        LOGGER_ERROR ( "Unable to open TIFF (to read) " << filename );
        return NULL;
    } else {
        // Lecture de l'en-tête pour récupérer les informations sur l'image
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

        if ( TIFFGetField ( tif, TIFFTAG_BITSPERSAMPLE,&bitspersample ) < 1 ) {
            LOGGER_ERROR ( "Unable to read number of bits per sample for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_SAMPLEFORMAT,&sampleformat ) < 1 ) {
            if ( bitspersample == 8 ) {
                sampleformat = SAMPLEFORMAT_UINT;
            } else if ( bitspersample == 32 ) {
                sampleformat = SAMPLEFORMAT_IEEEFP;
            } else {
                LOGGER_ERROR ( "Unable to determine sample format for file " << filename );
                return NULL;
            }
        }

        if ( TIFFGetField ( tif, TIFFTAG_PHOTOMETRIC,&photometric ) < 1 ) {
            LOGGER_ERROR ( "Unable to read photometric for file " << filename );
            return NULL;
        }

        if ( TIFFGetField ( tif, TIFFTAG_COMPRESSION,&compression ) < 1 ) {
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
            // On a des canaux en plus, si c'est de l'alpha, il doit être associé
            if ( extrasamples[0] == EXTRASAMPLE_UNASSALPHA ) {
                LOGGER_ERROR ( "Alpha sample is unassociated for the file " << filename );
                return NULL;
            }
        }
    }

    if ( tif != NULL && width*height*channels != 0 && planarconfig != PLANARCONFIG_CONTIG ) {
        LOGGER_ERROR ( "Planar configuration have to be 'PLANARCONFIG_CONTIG' for file " << filename );
        return NULL;
    }

    SampleFormat::eSampleFormat sf = toROK4SampleFormat ( sampleformat );

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

    return new LibtiffImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, toROK4Photometric ( photometric ), toROK4Compression ( compression ),
        tif, rowsperstrip
    );
}

/* ----- Pour l'écriture ----- */
LibtiffImage* LibtiffImageFactory::createLibtiffImageToWrite (
    char* filename, BoundingBox<double> bbox, double resx, double resy, int width, int height, int channels,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
    Compression::eCompression compression, uint16_t rowsperstrip ) {

    if ( width <= 0 || height <= 0 ) {
        LOGGER_ERROR ( "One dimension is not valid for the output image " << filename << " : " << width << ", " << height );
        return NULL;
    }
    if ( channels <= 0 ) {
        LOGGER_ERROR ( "Number of samples per pixel is not valid for the output image " << filename << " : " << channels );
        return NULL;
    }

    if ( ! SampleFormat::isHandledSampleType ( sampleformat, bitspersample ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sampleformat ) << " and " << bitspersample << " bits per sample" );
        return NULL;
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
        uint16_t extrasample = EXTRASAMPLE_ASSOCALPHA;
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

    if ( TIFFSetField ( tif, TIFFTAG_ROWSPERSTRIP,1 ) < 1 ) {
        LOGGER_ERROR ( "Unable to write number of rows per strip for file " << filename );
        return NULL;
    }

    if ( TIFFSetField ( tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE ) < 1 ) {
        LOGGER_ERROR ( "Unable to write pixel resolution unit for file " << filename );
        return NULL;
    }

    if ( resx < 0 || resy < 0 ) {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    return new LibtiffImage (
        width, height, resx, resy, channels, bbox, filename,
        sampleformat, bitspersample, photometric, compression,
        tif, rowsperstrip
    );
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibtiffImage::LibtiffImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric,
    Compression::eCompression compression, TIFF* tif, int rowsperstrip ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),

    tif ( tif ), rowsperstrip ( rowsperstrip ) {

    current_strip = -1;
    strip_size = width*channels*rowsperstrip;
    strip_buffer = new uint8_t[strip_size];
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibtiffImage::_getline ( T* buffer, int line ) {
    // le buffer est déjà alloue
    // Cas RGB : canaux entrelaces (TIFFTAG_PLANARCONFIG=PLANARCONFIG_CONTIG)

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
    }
    return width*channels;
}

int LibtiffImage::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer,line );
}

int LibtiffImage::getline ( float* buffer, int line ) {
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint8_t* buffer_t = new uint8_t[width*channels];
        getline ( buffer_t,line );
        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
        return width*channels;
    } else { // float
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

    // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        buf_u = ( unsigned char* ) _TIFFmalloc ( width * channels * getBitsPerSample() / 8 );
        for ( int line = 0; line < height; line++ ) {
            //LOGGER_INFO("line " << line);
            pIn->getline ( buf_u,line );
            if ( TIFFWriteScanline ( tif, buf_u, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        buf_f = ( float* ) _TIFFmalloc ( width * channels * getBitsPerSample() /8 );
        for ( int line = 0; line < height; line++ ) {
            pIn->getline ( buf_f,line );
            if ( TIFFWriteScanline ( tif, buf_f, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    }

    // Liberation
    if ( buf_u ) _TIFFfree ( buf_u );
    if ( buf_f ) _TIFFfree ( buf_f );

    return 0;
}

int LibtiffImage::writeImage ( uint8_t* buffer) {
    // Initialisation du buffer
    float* buf_f = 0;

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        buf_f = new float[height * width * channels];
        convert ( buf_f, buffer, height*width*channels );
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buf_f + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    }

    // Liberation
    if ( buf_f ) delete []  buf_f ;

    return 0;
}

int LibtiffImage::writeImage ( float* buffer) {
    // Initialisation du buffer
    unsigned char* buf_u=0;

    // Ecriture de l'image
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        buf_u = new uint8_t[height * width * channels];
        convert ( buf_u, buffer, height*width*channels );
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buf_u + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) {
        for ( int line = 0; line < height; line++ ) {
            if ( TIFFWriteScanline ( tif, buffer + line * width * channels, line, 0 ) < 0 ) {
                LOGGER_ERROR ( "Cannot write file " << TIFFFileName ( tif ) << ", line " << line );
                return -1;
            }
        }
    }

    // Liberation
    if ( buf_u ) delete [] buf_u;

    return 0;
}



