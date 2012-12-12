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

#include "LibtiffImage.h"
#include "Logger.h"
#include "Utils.h"

/**
Creation d'une LibtiffImage a partir d un fichier TIFF filename
retourne NULL en cas d erreur
*/

LibtiffImage* LibtiffImageFactory::createLibtiffImage( char* filename, BoundingBox< double > bbox, int calcWidth, int calcHeight, double resx, double resy )
{
    int width=0,height=0,channels=0,planarconfig=0,bitspersample=0,
        sampleformat=0,photometric=0,compression=0,rowsperstrip=0;
    TIFF* tif=TIFFOpen(filename, "r");
    
    if (tif == NULL) {
        LOGGER_ERROR( "Unable to open " << filename);
        return NULL;
    } else {
        
        if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width)<1) {
            LOGGER_ERROR( "Unable to read pixel width for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height)<1) {
            LOGGER_ERROR( "Unable to read pixel height for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,&channels)<1) {
            LOGGER_ERROR( "Unable to read number of samples per pixel for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_PLANARCONFIG,&planarconfig)<1) {
            LOGGER_ERROR( "Unable to read planar configuration for file " << filename);
            return NULL;
        }
        
        if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,&bitspersample)<1) {
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

    // Vérification de la cohérence entre les résolutions et bbox fournies et les dimensions (en pixel) de l'image
    if (calcWidth != width || calcHeight != height) {
        LOGGER_ERROR("Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent");
        LOGGER_ERROR("Height is " << height << " and calculation give " << calcHeight);
        LOGGER_ERROR("Width is " << width << " and calculation give " << calcWidth);
        return NULL;
    }
    
    return new LibtiffImage(width, height, resx, resy, channels, bbox, tif, filename, bitspersample, sampleformat,
                            photometric, compression, rowsperstrip);
}

/**
Creation d'une LibtiffImage en vue de creer un nouveau fichier TIFF
retourne NULL en cas d erreur
*/

LibtiffImage* LibtiffImageFactory::createLibtiffImage( char* filename, BoundingBox< double > bbox, int width, int height, double resx, double resy, int channels, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric, uint16_t compression, uint16_t rowsperstrip )
{
    if (width<0||height<0)
        return NULL;
        if (width*height*channels==0)
                return NULL;

    return new LibtiffImage(width,height,resx,resy,channels,bbox,NULL,filename,bitspersample,sampleformat,photometric,
                            compression,rowsperstrip);
}

LibtiffImage::LibtiffImage(int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, TIFF* tif,char* name, int bitspersample, int sampleformat, int photometric, int compression, int rowsperstrip) : Image(width,height,resx,resy,channels,bbox), tif(tif), bitspersample(bitspersample), sampleformat(sampleformat), photometric(photometric), compression(compression), rowsperstrip(rowsperstrip)
{
    filename = new char[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
    strcpy(filename,name);

    current_strip = -1;
    strip_size = width*channels*rowsperstrip;
    strip_buffer = new uint8_t[strip_size];
}

template<typename T>
int LibtiffImage::_getline(T* buffer, int line)
{
    // le buffer est déjà alloue
    // Cas RGB : canaux entrelaces (TIFFTAG_PLANARCONFIG=PLANARCONFIG_CONTIG)

    if (compression == COMPRESSION_NONE || (compression != COMPRESSION_NONE && rowsperstrip == 1) ) {
        // Cas Non compresse ou (compresse et 1 ligne/bande)
        if (TIFFReadScanline(tif,buffer,line,0) < 0) {
            LOGGER_DEBUG("Erreur de lecture du fichier TIFF " << TIFFFileName(tif) << " ligne " << line);
        }
    } else {
        // Cas compresse et > 1 ligne /bande
        if (line / rowsperstrip != current_strip) {
            current_strip = line / rowsperstrip;
            if (TIFFReadEncodedStrip(tif,current_strip,strip_buffer,strip_size) < 0) {
                LOGGER_DEBUG("Erreur de lecture du fichier TIFF "<<TIFFFileName(tif)<<" ligne "<<line);
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

