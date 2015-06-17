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


#include "Jpeg2000_library_config.h"
#include "LibkakaduImage.h"
#include "Logger.h"
#include "Utils.h"
#include "kdu_region_decompressor.h"

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
    
    int width = 0, height = 0, bitspersample = 0, channels = 0, rowsperstrip = 16;
    SampleFormat::eSampleFormat sf = SampleFormat::UINT;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;

    /************** RECUPERATION DES INFORMATIONS **************/
    
    // Create appropriate output file
    kdu_compressed_source *input = NULL;
    kdu_simple_file_source file_in;
    jp2_family_src jp2_src;
    jp2_source jp2_in;
    
    jp2_input_box box;
    jp2_src.open(filename);
  
    if (box.open(&jp2_src) && (box.get_box_type() == jp2_signature_4cc) ) {
        input = &jp2_in;
        if (! jp2_in.open(&jp2_src)) {
            LOGGER_ERROR("Unable to open with Kakadu the JPEG2000 image " << filename);
            return NULL;
        }
        jp2_in.read_header();
    } else {
        // Try opening as a raw code-stream.
        input = &file_in;
        file_in.open(filename);
    }
    
    kdu_codestream codestream;
    codestream.create(input);
    
    // On récupère les information de la première composante, puis on vérifiera que toutes les composantes ont bien les mêmes
    channels = codestream.get_num_components(true);
    bitspersample = codestream.get_bit_depth(0);
    kdu_dims dims;
    codestream.get_dims(0,dims,true);
    width = dims.size.get_x();
    height = dims.size.get_y();
    
    for (int i = 0; i < channels; i++) {
        codestream.get_dims(i,dims,true);
        if (dims.size.get_x() != width || dims.size.get_y() != height) {
            LOGGER_ERROR("All components don't own the same dimensions in JPEG2000 file");
                LOGGER_ERROR("file : " << filename);
            return NULL;
        }
        if (codestream.get_bit_depth(i) != bitspersample) {
            LOGGER_ERROR("All components don't own the same bit depth in JPEG2000 file ");
            LOGGER_ERROR("file : " << filename);
            return NULL;
        }
    }
    
    if (jp2_in.access_palette().get_num_entries() != 0) {
        LOGGER_ERROR("JPEG2000 image with palette not handled");
        LOGGER_ERROR("file : " << filename);
        return NULL;
    }
    
    if (jp2_in.access_channels().get_num_colours() != channels) {
        LOGGER_DEBUG("num_colours != channels");
        LOGGER_DEBUG(jp2_in.access_channels().get_num_colours() << " != " << channels);
        LOGGER_DEBUG("file : " << filename);
    }
    
    switch(channels) {
        case 1:
            ph = Photometric::GRAY;
            break;
        case 2:
            ph = Photometric::GRAY;
            break;
        case 3:
            ph = Photometric::RGB;
            break;
        case 4:
            ph = Photometric::RGB;
            break;
        default:
            LOGGER_ERROR("Cannot determine photometric from the number of samples " << channels);
            LOGGER_ERROR("file : " << filename);
            return NULL;
    }
    
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
   
    codestream.destroy();
    input->close(); // Not really necessary here.
    box.close();

    /******************** CRÉATION DE L'OBJET ******************/

    /* L'objet LibkakaduImage devra pouvoir se charger lui-meme de charger les bandes.
     * Pour cela un controle devra determiner si la bande que l'on cherche a traiter est la bande active ou pas
     * Si necessaire on devra donc charger une bande de 16 lignes.
     * Il faut penser a generer les tableaux de type stripe_heights[channels]
     */
    return new LibkakaduImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, Compression::JPEG2000,
        rowsperstrip
    );

}

/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

LibkakaduImage::LibkakaduImage (
    int width, int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    int rps ) :

    Jpeg2000Image ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression ),
    rowsperstrip(rps)
    
{    
    // Create appropriate output file
    kdu_compressed_source *input = NULL;
    kdu_simple_file_source file_in;
    jp2_family_src jp2_src;
    jp2_source jp2_in;
    
    jp2_input_box box;
    jp2_src.open(filename);
  
    if (box.open(&jp2_src) && (box.get_box_type() == jp2_signature_4cc) ) {
        input = &jp2_in;
        jp2_in.open(&jp2_src);
        jp2_in.read_header();
    } else {
        input = &file_in;
        file_in.open(filename);
    }
    
    m_codestream.create(input);
    m_codestream.set_fussy(); // Set the parsing error tolerance.
    m_codestream.apply_input_restrictions(0, channels, 0, 0, NULL);
    
    int num_threads = atoi(KDU_THREADING);
    
    kdu_thread_env env;
    m_kdu_env_ref = NULL;
    if (num_threads > 0) {
        env.create();
        for (int nt=1; nt < num_threads; nt++) {
            if (!env.add_thread()) {
                LOGGER_WARN("Unable to create all the wanted threads. Number of threads reduced from " << num_threads << " to " << nt);
                num_threads = nt; // Unable to create all the threads requested
            }
        }
        m_kdu_env_ref = &env;
    } else {
      LOGGER_WARN("Null or negative number of threads... Leaving kdu_thread_env object uninitialised.");
    }
  
    /************** INITIALISATION DES OBJETS KAKADU *********/

    // Custom messaging services
    kdu_customize_warnings(&pretty_cout);
    kdu_customize_errors(&pretty_cerr);
  
    strip_buffer = new kdu_byte[rowsperstrip * width * channels];

    current_strip = -1;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

template<typename T>
int LibkakaduImage::_getline ( T* buffer, int line ) {
    if (m_kdu_env_ref == NULL || !m_kdu_env_ref->exists() || m_kdu_env_ref->get_num_threads() < 1) {
      LOGGER_ERROR("Threading environment isn't initialised !");
      return 0;
    }
    if (buffer == NULL) {
      LOGGER_ERROR("Image line output buffer isn't initialised !");
      return 0;
    }
 
    if ( line / rowsperstrip != current_strip ) {
    
        // Les données n'ont pas encore été lue depuis l'image (strip pas en mémoire).
        current_strip = line / rowsperstrip;
        
        kdu_dims stripeDims, mappedStripeDims;
        stripeDims.pos = kdu_coords(0, current_strip*rowsperstrip);
        stripeDims.size = kdu_coords(width,rowsperstrip);
        
        m_codestream.map_region(-1, stripeDims, mappedStripeDims);
        
        kdu_channel_mapping chan_mapping;
        chan_mapping.configure(m_codestream);

        bool decompressorStartTest;
        decompressorStartTest = m_decompressor.start(
            m_codestream, &chan_mapping,
            0 /* single_component=0 : not a single component case */,
            0 /* discard_level=0 : we work with the highest resolution image */,
            1 /* max_layer=1 : we work ONLY with the highest resolution image */,
            mappedStripeDims, kdu_coords(1,1), kdu_coords(1,1), true, KDU_WANT_OUTPUT_COMPONENTS,
            false, m_kdu_env_ref, NULL
        );                   

        kdu_dims new_region, incomplete_region = mappedStripeDims;
        
        int channel_offsets[channels];
        
        int pixel_gap, row_gap, suggested_increment, max_region_pixels, precision_bits = 8, expand_monochrome = 0, fill_alpha = 0;
        kdu_coords buffer_origin;
        bool measure_row_gap_in_pixels = true;

        for (int channel_offset = 0; channel_offset < channels; channel_offset++) {
            channel_offsets[channel_offset] = channel_offset;
        }
        
        pixel_gap = channels;
        buffer_origin.x = mappedStripeDims.pos.x;
        buffer_origin.y = mappedStripeDims.pos.y;
        row_gap = width;
        suggested_increment = width*channels;
        max_region_pixels = 16*width;

        int dbgIncrement = 0;
        bool processInProgress = true;
        if (incomplete_region.is_empty()) {
            LOGGER_ERROR("Kakadu region decompressor's incomplete region is empty before processing. This should not happen.");
            return 0;
        } else {
            
            while( processInProgress && !incomplete_region.is_empty()) {

                LOGGER_DEBUG("Boucle decompressor.process, itération n°" << dbgIncrement );
                try {
                    processInProgress = m_decompressor.process(
                        strip_buffer, channel_offsets, pixel_gap,
                        buffer_origin, row_gap, suggested_increment, max_region_pixels, incomplete_region,
                        new_region, precision_bits, measure_row_gap_in_pixels, expand_monochrome, fill_alpha
                    ); //segfault ici
                } catch (std::exception e) {
                    LOGGER_ERROR("An exception occured: " << e.what());
                    return 0;
                } catch (kdu_exception e) {
                    LOGGER_ERROR("A kakadu exception occured: " << e);
                    return 0;
                } catch (...) {
                    LOGGER_ERROR("An unidentified default exception occured.");
                    return 0;
                }
                
                dbgIncrement++;
                LOGGER_DEBUG("Fin de l'itération.");
            }
        }    

        bool readSuccess;
        readSuccess = m_decompressor.finish();

        //int size = TIFFReadEncodedStrip ( tif, current_strip, strip_buffer, -1 );
        if (! readSuccess) {
            LOGGER_ERROR ( "Cannot read stripe number " << current_strip << " of image " << filename );
            return 0;
        }
    
    }
    
    memcpy ( buffer, (uint8_t*) strip_buffer + ( line%rowsperstrip ) * width * pixelSize, width * pixelSize );
    
    return width*channels;
}

int LibkakaduImage::getline ( uint8_t* buffer, int line ) {
  
  if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
    int r = _getline ( buffer,line );
    return r;
  } else if ( bitspersample == 32 && sampleformat == SampleFormat::FLOAT ) { // float
    /* On ne convertit pas les nombres flottants en entier sur 8 bits (aucun intérêt)
     * On va copier le buffer flottant sur le buffer entier, de même taille en octet (4 fois plus grand en "nombre de cases")*/
    float floatline[width * channels];
    _getline ( floatline, line );
    memcpy ( buffer, floatline, width*channels*sizeof(float) );
    return width*channels*sizeof(float);
  }
}

int LibkakaduImage::getline ( float* buffer, int line ) {
  if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
    // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
    uint8_t* buffer_t = new uint8_t[width*channels];
    
    // Ne pas appeler directement _getline pour bien faire les potentielles conversions (alpha ou 1 bit)
    getline ( buffer_t,line );
    convert ( buffer,buffer_t,width*channels );
    delete [] buffer_t;
    return width*channels;
  } else { // float
    return _getline ( buffer, line );
  }
}

