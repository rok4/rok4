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
 * \file BilzImage.cpp
 ** \~french
 * \brief Implémentation des classes BilzImage et BilzImageFactory
 * \details
 * \li BilzImage : gestion d'une image au format PNG, en lecture
 * \li BilzImageFactory : usine de création d'objet BilzImage
 ** \~english
 * \brief Implement classes BilzImage and BilzImageFactory
 * \details
 * \li BilzImage : manage a (Z)BIL format image, reading
 * \li BilzImageFactory : factory to create BilzImage object
 */

#include "BilzImage.h"
#include "Logger.h"
#include "Utils.h"


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
BilzImage* BilzImageFactory::createBilzImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
        
    
    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0, bitspersample = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;
    Compression::eCompression comp = Compression::NONE;
    
    if (! readHeaderFile(filename, &width, &height, &channels, &bitspersample)) {
        LOGGER_ERROR ( "Cannot read header associated to (Z)Bil image " << filename );
        return NULL;        
    }
    
    if (width == 0 || height == 0 || channels == 0 || bitspersample == 0) {
        LOGGER_ERROR ( "Missing or invalid information in the header associated to (Z)Bil image " << filename );
        return NULL;         
    }
    
    switch (bitspersample) {
        case 32 :
            sf = SampleFormat::FLOAT;
            break;
        case 16 :
        case 8 :
            sf = SampleFormat::UINT;
            break;
        default :
            LOGGER_ERROR ( "Unhandled number of bits per sample (" << bitspersample << ") for image " << filename );
            return NULL;
    }
    
    switch (channels) {
        case 1 :
        case 2 :
            ph = Photometric::GRAY;
            break;
        case 3 :
        case 4 :
            ph = Photometric::RGB;
            break;
        default :
            LOGGER_ERROR ( "Unhandled number of samples pixel (" << channels << ") for image " << filename );
            return NULL;
    }
    
    /********************** CONTROLES **************************/
    
    if ( ! BilzImage::canRead ( bitspersample, sf ) ) {
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

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/
    
    FILE *file = fopen ( filename, "rb" );
    if ( !file ) {
        LOGGER_ERROR ( "Unable to open the file (to read) " << filename );
        return NULL;
    }
    
    int rawdatasize = width * height * channels * bitspersample / 8;
    uint8_t* bilData = new uint8_t[rawdatasize];
    
    size_t readdatasize = fread ( bilData, 1, rawdatasize, file );
    
    fclose ( file );

    if (readdatasize < rawdatasize ) {
        LOGGER_DEBUG("Data in bil image are compressed (smaller than expected). We try to uncompressed them (deflate).");
        comp = Compression::DEFLATE;
        
        uint8_t* tmpData = new uint8_t[readdatasize];
        memcpy(tmpData, bilData, readdatasize);
        
        if (! uncompressedData(tmpData, readdatasize, bilData, rawdatasize)) {
            LOGGER_ERROR ( "Cannot uncompressed data for file " << filename );
            LOGGER_ERROR ( "Only deflate compression is supported");
            return NULL;
        }
        
        delete [] tmpData;
    }
    
    
    
    /******************** CRÉATION DE L'OBJET ******************/
    
    return new BilzImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, comp,
        bilData
    );
    
}


/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

BilzImage::BilzImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    uint8_t* bilData ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),

    data(bilData) {
        
    //tmpbuffer = new uint8_t[width * pixelSize];
    
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int BilzImage::_getline ( T* buffer, int line ) {

    T buffertmp[width * channels];
    
    // Si on a un seul canal, il n'y a rien à faire (simple copie depuis le buffer data). Pour gagner du temps, on le teste
    if (channels == 1) {
        memcpy(buffertmp, data + line * width * pixelSize, width * pixelSize);
    } else {
    
        int samplesize = bitspersample / 8;
        
        for (int s = 0; s < channels; s++) {
            uint8_t* deb = data + line * width * pixelSize;
            for (int p = 0; p < width; p++) {
                memcpy(buffertmp + p * pixelSize + s * samplesize, deb + p * samplesize, samplesize );
            }
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

int BilzImage::getline ( uint8_t* buffer, int line ) {
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
    return 0;
}

int BilzImage::getline ( uint16_t* buffer, int line ) {
    
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
    return 0;
}

int BilzImage::getline ( float* buffer, int line ) {
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
    return 0;
}

