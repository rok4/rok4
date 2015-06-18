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
    
    /************** INITIALISATION DES OBJETS KAKADU *********/

    // Custom messaging services
    kdu_customize_warnings(&pretty_cout);
    kdu_customize_errors(&pretty_cerr);
  

  
    strip_buffer = new kdu_byte[rowsperstrip * width * channels];

    current_strip = -1;
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

bool LibkakaduImage::_loadstrip() {

    int C0 = 0;
    int L0 = rowsperstrip * current_strip;
    int deZoom = 1;
    void *buffer = strip_buffer;
    int nPixelSpace = pixelSize;
    int nLineSpace = pixelSize * width;
    int nBandSpace = 1;
    int NLines = __min(rowsperstrip, height - rowsperstrip * current_strip);
    
    jp2_source m_Source;
    jp2_family_src jp2_ultimate_src;
    jp2_ultimate_src.open(filename);
        
    if (!m_Source.open(&jp2_ultimate_src))
    {
    }
    m_Source.read_header();
    kdu_codestream codestream;
    codestream.create(&m_Source);
        
    int dz = deZoom;
    int max_layers = 0;
    int discard_levels = 0;
    while(((1 << discard_levels) & deZoom)==0) ++discard_levels;
    int minDwtLevels = codestream.get_min_dwt_levels();
    int reDeZoom = 0;
    if (discard_levels>minDwtLevels)
    {
            reDeZoom = (1<<(discard_levels-minDwtLevels));
            discard_levels=minDwtLevels;
            dz = (1 << discard_levels);
    }
    
    int * precisions = new int[channels];
    bool *is_signed = new bool[channels];
    // ToDo tenir compte du bufferType      
    for(size_t i=0;i<(size_t)channels;++i) 
    {
            precisions[i]=8;
            is_signed[i]=false;
    }
    
    kdu_dims dims,mdims;
    // Position de l'origine en coord fichier (cad pleine resolution)
    dims.pos=kdu_coords(C0, L0);
    // Taille de la zone en coord fichier (cad pleine resolution)
    dims.size=kdu_coords(width,NLines);
    codestream.map_region(0,dims,mdims);
    
    codestream.apply_input_restrictions(0,channels,discard_levels,max_layers,&mdims,KDU_WANT_OUTPUT_COMPONENTS);
    
    kdu_thread_env env, *env_ref=NULL;
    
    int num_threads = atoi(KDU_THREADING);
    
    if (num_threads > 0)
    {
            env.create();
            for (int nt=1; nt < num_threads; nt++)
                if (!env.add_thread())
                    num_threads = nt; // Unable to create all the threads requested
            env_ref = &env;
    }
    
    int n, num_components = codestream.get_num_components(true);
    kdu_dims *comp_dims = new kdu_dims[num_components];
    for (n=0; n < num_components; n++)
            codestream.get_dims(n,comp_dims[n],true);
    
    kdu_region_decompressor decompressor;
    kdu_channel_mapping kchannels;
    kchannels.configure(codestream);
    int single_component=0;//ignore sauf sur mapping est nul
    
    kdu_dims region=decompressor.get_rendered_image_dims(codestream,&kchannels,single_component,discard_levels,kdu_coords(1,1),kdu_coords(1,1),KDU_WANT_OUTPUT_COMPONENTS);
    
    
    //if (reDeZoom==0)
    {
            region.pos.x=C0/deZoom;
            region.pos.y=L0/deZoom;
            region.size.x=width/deZoom;
            region.size.y=NLines/deZoom;
    }
    
    decompressor.start(
        codestream,&kchannels,single_component, discard_levels,
        0, region, kdu_coords(1,1),kdu_coords(1,1),true,KDU_WANT_OUTPUT_COMPONENTS,false,env_ref,0
    );        
    
    kdu_dims new_region, incomplete_region=region;
    
    char ** channel_bufs=new char*[num_components];
    int row_gap = 0;
    int suggested_increment = 0;
    if (reDeZoom==0)
    {
            for(n=0;n<num_components;++n)
            {
                    channel_bufs[n] = (char*)buffer + n*nBandSpace;
            }
            //row_gap = nLineSpace*ImageIO::TypeSize(bufferType);
            //suggested_increment = comp_dims[0].size.x*comp_dims[0].size.y;
            row_gap = comp_dims[0].size.x;
            suggested_increment = comp_dims[0].size.x*comp_dims[0].size.y;
            
    }
    
    kdu_coords buffer_origin;
    buffer_origin.x = incomplete_region.pos.x;
    buffer_origin.y = incomplete_region.pos.y;
    
    while(decompressor.process(
                                (kdu_byte**)channel_bufs, false, nPixelSpace,
                                buffer_origin, row_gap, suggested_increment, suggested_increment,
                                incomplete_region, new_region, 8, true
                              ) && ! incomplete_region.is_empty())
    {
        
    }
    
    delete[] channel_bufs;
    decompressor.finish();
    
    if (env.exists())
            env.destroy();
    
    if (env.exists())
            env.destroy();
    
    delete[] precisions;
    delete[] is_signed;
    delete[] comp_dims;
    
    codestream.destroy();
    m_Source.close();           
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

