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
/* -------------------------------------------- USINES -------------------------------------------- */

                                /* ----- Pour la lecture ----- */
LibtiffImage* LibtiffImageFactory::createLibtiffImageToRead(char* filename, BoundingBox< double > bbox, double resx, double resy)
{
    int width=0, height=0, channels=0, planarconfig=0, bitspersample=0, sampleformat=0, photometric=0, compression=0, rowsperstrip=0;
    TIFF* tif=TIFFOpen(filename, "r");
    
    if (tif == NULL) {
        LOGGER_ERROR( "Unable to open (to read) " << filename);
        return NULL;
    } else {
        // Lecture de l'en-tête pour récupérer les informations sur l'image
        if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width) < 1) {
            LOGGER_ERROR( "Unable to read pixel width for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height) < 1) {
            LOGGER_ERROR( "Unable to read pixel height for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,&channels) < 1) {
            LOGGER_ERROR( "Unable to read number of samples per pixel for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_PLANARCONFIG,&planarconfig) < 1) {
            LOGGER_ERROR( "Unable to read planar configuration for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,&bitspersample) < 1) {
            LOGGER_ERROR( "Unable to read number of bits per sample for file " << filename);
            return NULL;
        }

        if (TIFFGetField(tif, TIFFTAG_SAMPLEFORMAT,&sampleformat) < 1) {
            if (bitspersample == 8) {
                sampleformat = SAMPLEFORMAT_UINT;
            } else if (bitspersample == 32) {
                sampleformat = SAMPLEFORMAT_IEEEFP;
            } else {
                LOGGER_ERROR( "Unable to determine sample format for file " << filename);
                return NULL;
            }
        }
        
        if (TIFFGetField(tif, TIFFTAG_PHOTOMETRIC,&photometric) < 1) {
            LOGGER_ERROR( "Unable to read photometric for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_COMPRESSION,&compression) < 1) {
            LOGGER_ERROR( "Unable to read compression for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP,&rowsperstrip) < 1) {
            LOGGER_ERROR( "Unable to read number of rows per strip for file " << filename);
            return NULL;
        }
    }

    if (tif != NULL && width*height*channels != 0 && planarconfig != PLANARCONFIG_CONTIG) {
        LOGGER_ERROR( "Planar configuration have to be 'PLANARCONFIG_CONTIG' for file " << filename);
        return NULL;
    }

    SampleType ST = SampleType(bitspersample,sampleformat);

    if (! ST.isSupported() ) {
        LOGGER_ERROR("Supported sample format are 8-bit unsigned integer and 32-bit float");
        return NULL;
    }

    if (resx > 0 && resy > 0) {
        // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
        // Arrondi a la valeur entiere la plus proche
        int calcWidth = lround((bbox.xmax - bbox.xmin)/(resx));
        int calcHeight = lround((bbox.ymax - bbox.ymin)/(resy));
        if (calcWidth != width || calcHeight != height) {
            LOGGER_ERROR("Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent");
            LOGGER_ERROR("Height is " << height << " and calculation give " << calcHeight);
            LOGGER_ERROR("Width is " << width << " and calculation give " << calcWidth);
            return NULL;
        }
    }
    
    return new LibtiffImage(width, height, resx, resy, channels, bbox, tif, filename, ST, photometric, compression, rowsperstrip);
}

                                /* ----- Pour l'écriture ----- */
LibtiffImage* LibtiffImageFactory::createLibtiffImageToWrite(char* filename, BoundingBox<double> bbox, double resx, double resy,
                                                int width, int height, int channels, SampleType sampleType,
                                                uint16_t photometric, uint16_t compression, uint16_t rowsperstrip )
{
    if ( width <= 0 || height <= 0) {
        LOGGER_ERROR("One dimension is not valid for the output image " << filename << " : " << width << ", " << height);
        return NULL;
    }
    if (channels <= 0) {
        LOGGER_ERROR("Number of samples per pixel is not valid for the output image " << filename << " : " << channels);
        return NULL;
    }

    if (! sampleType.isSupported() ) {
        LOGGER_ERROR("Supported sample format are :\n" + sampleType.getHandledFormat());
        return NULL;
    }

    TIFF* tif = TIFFOpen(filename, "w");
    if (tif == NULL) {
        LOGGER_ERROR( "Unable to open (to write) " << filename);
        return NULL;
    }

    // Ecriture de l'en-tête pour récupérer les informations sur l'image
    if (TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width) < 1) {
        LOGGER_ERROR( "Unable to write pixel width for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_IMAGELENGTH, height) < 1) {
        LOGGER_ERROR( "Unable to write pixel height for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL,channels) < 1) {
        LOGGER_ERROR( "Unable to write number of samples per pixel for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_PLANARCONFIG,PLANARCONFIG_CONTIG) < 1) {
        LOGGER_ERROR( "Unable to write planar configuration for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE,sampleType.getBitsPerSample()) < 1) {
        LOGGER_ERROR( "Unable to write number of bits per sample for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_SAMPLEFORMAT,sampleType.getSampleFormat()) < 1) {
        LOGGER_ERROR( "Unable to write sample format for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_PHOTOMETRIC,photometric) < 1) {
        LOGGER_ERROR( "Unable to write photometric for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_COMPRESSION,compression) < 1) {
        LOGGER_ERROR( "Unable to write compression for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP,1) < 1) {
        LOGGER_ERROR( "Unable to write number of rows per strip for file " << filename);
        return NULL;
    }

    if (TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, RESUNIT_NONE) < 1) {
        LOGGER_ERROR( "Unable to write pixel resolution unit for file " << filename);
        return NULL;
    }

    return new LibtiffImage(width,height,resx,resy,channels,bbox,tif,filename,sampleType,photometric,compression,rowsperstrip);
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibtiffImage::LibtiffImage(int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox,
                           TIFF* tif,char* name, SampleType sampleType, int photometric, int compression,
                           int rowsperstrip) :
                           Image(width,height,resx,resy,channels,bbox), tif(tif), ST(sampleType), photometric(photometric), compression(compression), rowsperstrip(rowsperstrip)
{
    filename = new char[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    strcpy(filename,name);

    current_strip = -1;
    strip_size = width*channels*rowsperstrip;
    strip_buffer = new uint8_t[strip_size];
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibtiffImage::_getline(T* buffer, int line)
{
    // le buffer est déjà alloue
    // Cas RGB : canaux entrelaces (TIFFTAG_PLANARCONFIG=PLANARCONFIG_CONTIG)

    if (compression == COMPRESSION_NONE || (compression != COMPRESSION_NONE && rowsperstrip == 1) ) {
        // Cas Non compresse ou (compresse et 1 ligne/bande)
        if (TIFFReadScanline(tif,buffer,line,0) < 0) {
            LOGGER_DEBUG("Cannot read file " << TIFFFileName(tif) << ", line " << line);
        }
    } else {
        // Cas compresse et > 1 ligne /bande
        if (line / rowsperstrip != current_strip) {
            current_strip = line / rowsperstrip;
            if (TIFFReadEncodedStrip(tif,current_strip,strip_buffer,strip_size) < 0) {
                LOGGER_DEBUG("Cannot read file " << TIFFFileName(tif) << ", line " << line);
            }
        }
        memcpy(buffer,&strip_buffer[(line%rowsperstrip)*width*channels],width*channels*sizeof(uint8_t));
    }
    return width*channels;
}

int LibtiffImage::getline(uint8_t* buffer, int line)
{
    return _getline(buffer,line);
}    

int LibtiffImage::getline(float* buffer, int line)
{
    return _getline(buffer,line);
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- ECRITURE ------------------------------------------- */

int LibtiffImage::writeImage(Image* pIn)
{
    // Initialisation du buffer
    unsigned char* buf_u=0;
    float* buf_f=0;

    // Ecriture de l'image
    if (ST.isUInt8()){
        buf_u = (unsigned char*)_TIFFmalloc(width * channels * getBitsPerSample() / 8);
        for( int line = 0; line < height; line++) {
            pIn->getline(buf_u,line);
            if (TIFFWriteScanline(tif, buf_u, line, 0) < 0) {
                LOGGER_DEBUG("Cannot write file " << TIFFFileName(tif) << ", line " << line);
                return -1;
            }
        }
    } else if(ST.isFloat()){
        buf_f = (float*)_TIFFmalloc(width * channels * getBitsPerSample()/8);
        for( int line = 0; line < height; line++) {
            pIn->getline(buf_f,line);
            if (TIFFWriteScanline(tif, buf_f, line, 0) < 0) {
                LOGGER_DEBUG("Cannot write file " << TIFFFileName(tif) << ", line " << line);
                return -1;
            }
        }
    } else {
        LOGGER_DEBUG("Not handled sample format to write. Are handled: " << ST.getSampleFormat());
        return -1;
    }

    // Liberation
    if (buf_u) _TIFFfree(buf_u);
    if (buf_f) _TIFFfree(buf_f);
    
    return 0;
}
