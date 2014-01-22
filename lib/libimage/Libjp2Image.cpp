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
 * \file Libjp2Image.cpp
 ** \~french
 * \brief Implémentation des classes Libjp2Image et Libjp2ImageFactory
 * \details
 * \li Libjp2Image : image physique, attaché à un fichier
 * \li Libjp2ImageFactory : usine de création d'objet Libjp2Image
 ** \~english
 * \brief Implement classes Libjp2Image and Libjp2ImageFactory
 * \details
 * \li Libjp2Image : physical image, linked to a file
 * \li Libjp2ImageFactory : factory to create Libjp2Image object
 */

#include "image_config.h"

#include "Libjp2Image.h"
#include "Logger.h"
#include "Utils.h"

#ifdef HAVE_OPENJPEG
#include "Jp2DriverOpenJpeg.h"
#endif

#ifdef HAVE_KAKADU
#include "Jp2DriverKakadu.h"
#endif

#ifdef HAVE_JASPER
#include "Jp2DriverJasper.h"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------ CONVERSIONS ----------------------------------------- */

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
Libjp2Image* Libjp2ImageFactory::createLibjp2ImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
	
    // FIXME : memmory leak !
    // if you call a new operator, so you must call too a delete operator !
    // => bug design...

	Libjp2Image *pImage;
	
	#ifdef HAVE_OPENJPEG
	
    pImage = (new Jp2DriverOpenJpeg(filename, bbox, resx, resy))->createLibjp2ImageToRead();
	
	if(! pImage) {
		LOGGER_ERROR("DRIVER OPENJPEG, failed to read image !");
		return NULL;		
	}
	
	#endif

	#ifdef HAVE_KAKADU
	
	pImage = (new Jp2DriverKakadu(filename, bbox, resx, resy))->createLibjp2ImageToRead();
	
	if(! pImage) {
		LOGGER_ERROR("DRIVER KAKADU, failed to read image !");
		return NULL;		
	}
	
	#endif

	#ifdef HAVE_JASPER
	
	pImage = (new Jp2DriverJasper(filename, bbox, resx, resy))->createLibjp2ImageToRead();
	
	if(! pImage) {
		LOGGER_ERROR("DRIVER JASPER, failed to read image !");
		return NULL;		
	}
	
	#endif
	
	return pImage;
	
}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

Libjp2Image::Libjp2Image (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    uint8_t * data ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),

    m_data(data) {
        
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int Libjp2Image::_getline ( T* buffer, int line ) {
    
    int pos = channels * width * line;
    for (int x = 0;  x < width * channels; x++) {
        buffer[x] = m_data[pos + x];
    }
    return width*channels;
}

int Libjp2Image::getline ( uint8_t* buffer, int line ) {
    return _getline ( buffer,line );
}

int Libjp2Image::getline ( float* buffer, int line ) {
    
    // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers (forcément pour du PNG)
    uint8_t* buffer_t = new uint8_t[width*channels];
    getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
    
}
