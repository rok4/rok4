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
    
    int width = 0, height = 0, bitspersample = 0, channels = 0, rowsperstrip = 200;
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
    LibkakaduImage* o_LibkakaduImage = new LibkakaduImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, Compression::JPEG2000,
        rowsperstrip
    );
    o_LibkakaduImage->init();
    return o_LibkakaduImage;
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

}

/* ------------------------------------------------------------------------------------------------ */
/* ---------------------------------------- INITIALISATION ---------------------------------------- */

bool LibkakaduImage::init() {
    /******************** miscellaneous *******************/

    /******************* m_kdu_thread_env *****************/

    m_env_ref=NULL;

    int num_threads = atoi(KDU_THREADING);

    if (num_threads > 0)
    {
        m_env.create();
        for (int nt=1; nt < num_threads; nt++)
            if (!m_env.add_thread())
                num_threads = nt; // Unable to create all the threads requested
        m_env_ref = &m_env;
    }

    /******************** m_codestream ********************/
    jp2_ultimate_src.open(filename);

    if (!m_Source.open(&jp2_ultimate_src))
    {
    }
    m_Source.read_header();
    m_codestream.create(&m_Source, m_env_ref);
    m_codestream.set_persistent();


    /************** Custom messaging services *************/

    // Custom messaging services
    kdu_customize_warnings(&pretty_cout);
    kdu_customize_errors(&pretty_cerr);

    /************* strip_buffer & current_strip ***********/

    try {
        strip_buffer = new kdu_byte[rowsperstrip * width * channels];
    } catch (std::bad_alloc e) {
        LOGGER_ERROR("Memory allocation error while creating strip buffer.");
        return false;
    }  
    current_strip = -1;

}






/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

bool LibkakaduImage::_loadstrip() {

    int C0 = 0;
    int L0 = rowsperstrip * current_strip;
    int nBandSpace = 1;
    int NLines = __min(rowsperstrip, height - rowsperstrip * current_strip);
    
    
    int max_layers = 0;
    int discard_levels = 0;
   
    
    kdu_dims dims,mdims;
    // Position de l'origine en coord fichier (cad pleine resolution)
    dims.pos=kdu_coords(C0, L0);
    // Taille de la zone en coord fichier (cad pleine resolution)
    dims.size=kdu_coords(width,NLines);
    m_codestream.map_region(0,dims,mdims);
    
    m_codestream.apply_input_restrictions(0,channels,discard_levels,max_layers,&mdims,KDU_WANT_OUTPUT_COMPONENTS);
    
    
    
    int n, num_components = m_codestream.get_num_components(true);
    kdu_dims *comp_dims = new kdu_dims[num_components];
    for (n=0; n < num_components; n++)
        m_codestream.get_dims(n,comp_dims[n],true);
    
    kdu_region_decompressor decompressor;
    kdu_channel_mapping kchannels;
    kchannels.configure(m_codestream);
    int single_component=0;//ignore sauf sur mapping est nul
    
    decompressor.start(
        m_codestream,&kchannels,single_component, discard_levels,
        0, mdims, kdu_coords(1,1),kdu_coords(1,1),true,KDU_WANT_OUTPUT_COMPONENTS,false,m_env_ref,0
    );        
    
    kdu_dims new_region, incomplete_region=mdims;
    
    kdu_byte ** channel_bufs=new kdu_byte*[num_components];
    int row_gap = 0;
    int suggested_increment = 0;
    
    for(n=0;n<num_components;++n)
    {
        channel_bufs[n] = strip_buffer + n*nBandSpace;
    }
    row_gap = comp_dims[0].size.x;
    suggested_increment = comp_dims[0].size.x*comp_dims[0].size.y;
    
    kdu_coords buffer_origin;
    buffer_origin.x = incomplete_region.pos.x;
    buffer_origin.y = incomplete_region.pos.y;
    
    while(decompressor.process(
    channel_bufs, false, pixelSize,
                                buffer_origin, row_gap, suggested_increment, suggested_increment,
                                incomplete_region, new_region, 8, true
                            ) && ! incomplete_region.is_empty())
    {
        
    }
    
    delete[] channel_bufs;
    decompressor.finish();
    
    delete[] comp_dims;         
}

template<typename T>
int LibkakaduImage::_getline ( T* buffer, int line ) {
    

    if ( line / rowsperstrip != current_strip ) {
    
        // Les données n'ont pas encore été lue depuis l'image (strip pas en mémoire).
        current_strip = line / rowsperstrip;
        
        _loadstrip();    
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

