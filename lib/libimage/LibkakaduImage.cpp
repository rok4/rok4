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
//#include "jp2.h"
#include "jpx.h"
#include "kdu_stripe_decompressor.h"
#include "kdu_file_io.h"

/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------- LOGGERS DE KAKADU --------------------------------------- */

class kdu_stream_message : public kdu_message {
    public: // Member classes
        kdu_stream_message(std::ostream *stream) { this->stream = stream; }
        void put_text(const char *string) { (*stream) << string; }
        void flush(bool end_of_message=false) { stream->flush(); }
    private: // Data
        std::ostream *stream;
};

static kdu_stream_message cout_message(&std::cout);
static kdu_stream_message cerr_message(&std::cerr);
static kdu_message_formatter pretty_cout(&cout_message);
static kdu_message_formatter pretty_cerr(&cerr_message);


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
LibkakaduImage* LibkakaduImageFactory::createLibkakaduImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
    
    int width = 0, height = 0, bitspersample = 0, channels = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;
    
    /************** INITIALISATION DES OBJETS KAKADU *********/
    
    // Custom messaging services
    kdu_customize_warnings(&pretty_cout);
    kdu_customize_errors(&pretty_cerr);

    // Construct code-stream object
    
    kdu_compressed_source *input = NULL;
    kdu_simple_file_source file_in;
    jpx_codestream_source jpx_stream;
    jpx_layer_source jpx_layer;
    jpx_source jpx_in;
    jp2_resolution resolution;
    jp2_colour colour;
    jp2_family_src jp2_ultimate_src;
    jp2_palette palette;
    kdu_coords layer_size;
    jp2_channels ch;
    jp2_ultimate_src.open(filename);
    
    /************** RECUPERATION DES INFORMATIONS **************/
    
    if (jpx_in.open(&jp2_ultimate_src,true) < 0)
    { // Not compatible with JP2 or JPX.  Try opening as a raw code-stream.
        /*jp2_ultimate_src.close();
        file_in.open(filename);
        input = &file_in;*/
        LOGGER_ERROR ("Not compatible with JP2 or JPX : JPEG2000 file " << filename);
        return NULL;
    } else {
        jpx_layer = jpx_in.access_layer(0);
        if (!jpx_layer) { 
            LOGGER_ERROR ("Cannot access to the first layer in the JPEG2000 file " << filename);
            return NULL;
        }
        
        ch = jpx_layer.access_channels();
        resolution = jpx_layer.access_resolution();
        colour = jpx_layer.access_colour(0);
        layer_size = jpx_layer.get_layer_size();
        
        channels = ch.get_num_colours();
        width = layer_size.get_x();
        height = layer_size.get_y();
        sf = SampleFormat::UINT;
        ph = Photometric::RGB;
        bitspersample = 8;
        
        int cmp, plt, stream_id;
        ch.get_colour_mapping(0,cmp,plt,stream_id);
        jpx_stream = jpx_in.access_codestream(stream_id);
        
        palette = jpx_stream.access_palette();
        
        input = jpx_stream.open_stream();
    }
    
    kdu_codestream codestream;
    codestream.create(input);
    
    codestream.set_fussy(); // Set the parsing error tolerance.
    
    kdu_dims dims;
    codestream.get_dims(0,dims);
    
    codestream.apply_input_restrictions(0,channels,0,0,NULL);
    
    /********************** CONTROLES **************************/

    if ( ! LibkakaduImage::canRead ( bitspersample, sf ) ) {
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
    
    // Now decompress the image in one hit, using `kdu_stripe_decompressor'
    kdu_byte *kduData = new kdu_byte[(int) dims.area()*channels];
    kdu_stripe_decompressor decompressor;
    decompressor.start(codestream);
    int stripe_heights[3] = {dims.size.y,dims.size.y,dims.size.y};
    decompressor.pull_stripe(kduData,stripe_heights);
    decompressor.finish();
    // As an alternative to the above, you can decompress the image samples in
    // smaller stripes, writing the stripes to disk as they are produced by
    // each call to `decompressor.pull_stripe'.  For a much richer
    // demonstration of the use of `kdu_stripe_decompressor', take a look at
    // the "kdu_buffered_expand" application.

    // Write image buffer to file and clean up
    codestream.destroy();
    input->close(); // Not really necessary here.
    
    /******************** CRÉATION DE L'OBJET ******************/

    return new LibkakaduImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, Compression::JPEG2000,
        kduData
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibkakaduImage::LibkakaduImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    kdu_byte* kduData ) :

    Jpeg2000Image ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),

    data ( kduData ) {

}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

int LibkakaduImage::getline ( uint8_t* buffer, int line ) {
    memcpy(buffer, (uint8_t*) data + line * width * channels , width * channels );
    return width*channels;
}

int LibkakaduImage::getline ( float* buffer, int line ) {

    // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers (forcément pour du JPEG2000)
    uint8_t* buffer_t = new uint8_t[width*channels];
    int retour = getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return retour;
}

