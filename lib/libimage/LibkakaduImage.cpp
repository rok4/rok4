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
 * \file LibkakaduImage.cpp
 ** \~french
 * \brief Implémentation des classes LibkakaduImage et LibkakaduImageFactory
 * \details
 * \li LibkakaduImage : gestion d'une image au format JPEG2000, en lecture, utilisant la librairie openjpeg
 * \li LibkakaduImageFactory : usine de création d'objet LibkakaduImage
 ** \~english
 * \brief Implement classes LibkakaduImage and LibkakaduImageFactory
 * \details
 * \li LibkakaduImage : manage a JPEG2000 format image, reading, using the library openjpeg
 * \li LibkakaduImageFactory : factory to create LibkakaduImage object
 */


#include "LibkakaduImage.h"
#include "Logger.h"
#include "Utils.h"
#include <jp2.h>
#include <kdu_file_io.h>
#include <kdu_compressed.h>

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibkakaduImage* LibkakaduImageFactory::createLibkakaduImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
    
    int width = 0, height = 0, bitspersample = 0, channels = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;
    
    /************** INITIALISATION DES OBJETS KAKADU *********/
    
    kdu_compressed_source *input = NULL;
    kdu_simple_file_source file_in;
    jp2_family_src jp2_ultimate_src;
    jpx_source jpx_in;
    
    jp2_ultimate_src.open(filename,true);
    kdu_coords layer_size; // Zero unless `jpx_layer' exists.  Records the size
    // of the layer on its registration.  According to the JPX standard,
    // this is the size against which any `jp2_resolution' information is
    // to be interpreted -- interpretation is backward compatible with JP2.
    if (jpx_in.open(&jp2_ultimate_src,true) < 0) { // Not compatible with JP2 or JPX.  Try opening as a raw code-stream.
        LOGGER_ERROR ( "Unhandled format for the JPEG2000 file " << filename );
        jp2_ultimate_src.close();
        return NULL;/*
        jp2_ultimate_src.close();
        file_in.open(ifname,!no_seek);
        input = &file_in;
        raw_codestream = 0;*/
    }

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
    }

    return new LibkakaduImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, Compression::JPEG2000
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibkakaduImage::LibkakaduImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    opj_image_t *jp2ptr ) :

    Jpeg2000Image ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression )/*,

    jp2image ( jp2ptr )*/ {

}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

int LibkakaduImage::getline ( uint8_t* buffer, int line ) {
    
    //return width*channels;
    return -1;
}

int LibkakaduImage::getline ( float* buffer, int line ) {

    // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers (forcément pour du JPEG2000)
    uint8_t* buffer_t = new uint8_t[width*channels];
    int retour = getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return retour;
}

