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
 * \file Jpeg2000Image.cpp
 ** \~french
 * \brief Implémentation des classes Jpeg2000Image et Jpeg2000ImageFactory
 * \details
 * \li Jpeg2000Image : gestion d'une image au format JPEG2000, en lecture, faisant abstraction de la librairie utilisée
 * \li Jpeg2000ImageFactory : usine de création d'objet Jpeg2000Image
 ** \~english
 * \brief Implement classes Jpeg2000Image and Jpeg2000ImageFactory
 * \details
 * \li Jpeg2000Image : manage a JPEG2000 format image, reading, making transparent the used library
 * \li Jpeg2000ImageFactory : factory to create Jpeg2000Image object
 */

#include "Jpeg2000Image.h"
#include "Logger.h"
#include "Utils.h"
#include "Jpeg2000_library_config.h"

#ifdef USE_KAKADU
#include "LibkakaduImage.h"
#else
#include "LibopenjpegImage.h"
#endif

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
Jpeg2000Image* Jpeg2000ImageFactory::createJpeg2000ImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
    
#ifdef USE_KAKADU

    LOGGER_DEBUG("Driver le JPEG2000 : KAKADU");

    LibkakaduImageFactory DRVKDU;
    return DRVKDU.createLibkakaduImageToRead(filename, bbox, resx, resy);

#else

    LOGGER_DEBUG("Driver le JPEG2000 : OPENJPEG");

    LibopenjpegImageFactory DRVOJ;
    return DRVOJ.createLibopenjpegImageToRead(filename, bbox, resx, resy);
    
#endif

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

Jpeg2000Image::Jpeg2000Image (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression, false )

{
        
}

