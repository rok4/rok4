#include "Tile.h"
#include "jpeglib.h"
#include "zlib.h"
#include <setjmp.h>

/* 
 * Fonctions déclarées pour la libjpeg
 */
void init_source (jpeg_decompress_struct *cinfo) {}
boolean fill_input_buffer (jpeg_decompress_struct *cinfo){return false;}
void skip_input_data (jpeg_decompress_struct *cinfo, long num_bytes) {
	std::cerr << "c" << std::endl;
  	if (num_bytes > 0) {
    		cinfo->src->next_input_byte += (size_t) num_bytes;
    		cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
  	}
}
void term_source (jpeg_decompress_struct *cinfo) {}

// Gestion des erreurs fatales pour la libjpeg

/*
* Structure destinee a gerer les erreurs de la libjepg
*/

struct my_error_mgr {
  	struct jpeg_error_mgr pub;    /* "public" fields */

  	jmp_buf setjmp_buffer;        /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  	my_error_ptr myerr = (my_error_ptr) cinfo->err;

  	/* Always display the message. */
  	/* We could postpone this until after returning, if we chose. */
  	(*cinfo->err->output_message) (cinfo);

  	/* Return control to the setjmp point */
  	longjmp(myerr->setjmp_buffer, 1);
}

/*
* Decodage de donnee JPEG
*/

void JpegDecoder::decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize) { 
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	// Initialisation de cinfo
	jpeg_create_decompress(&cinfo);

	cinfo.src = new jpeg_source_mgr;
	cinfo.src->init_source = init_source;
	cinfo.src->fill_input_buffer = fill_input_buffer;
	cinfo.src->skip_input_data = skip_input_data;
	cinfo.src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
	cinfo.src->term_source = term_source;

	cinfo.src->next_input_byte = encoded_data;
	cinfo.src->bytes_in_buffer = encoded_size;

	// Initialisation de la gestion des erreurs
	cinfo.err = jpeg_std_error(&jerr.pub);
  	jerr.pub.error_exit = my_error_exit; 

	// Initialisation du contexte retourne par setjmp pour my_error_exit
  	if (setjmp(jerr.setjmp_buffer)) {
	// TODO : couleur nodata
	// Les dalles nodata sont en blanc
		memset(raw_data,255,linesize*height);
    		jpeg_destroy_decompress(&cinfo);
    		return;
  	}

	// Lecture
	if (jpeg_read_header(&cinfo, TRUE)==JPEG_HEADER_OK) {
    		jpeg_start_decompress(&cinfo);

		while (cinfo.output_scanline < (unsigned int) height) {
			uint8_t *line = raw_data + cinfo.output_scanline * linesize;
			if(jpeg_read_scanlines(&cinfo, &line, 1) < 1) {
				LOGGER_ERROR("Probleme lecture tuile Jpeg");
				break;
			}
		}
	}
	else
		LOGGER_ERROR("Erreur de lecture en tete jpeg");
	jpeg_finish_decompress(&cinfo);

	// Destruction de cinfo
	delete cinfo.src;
	jpeg_destroy_decompress(&cinfo);
}

/*
* Decodage de donnee PNG
* En cas d'erreur, le buffer raw_data n'est pas rempli jusqu'au bout
*/
void PngDecoder::decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize)
{
	// Initialisation du flux
  	z_stream zstream;
  	zstream.zalloc = Z_NULL;
  	zstream.zfree = Z_NULL;
  	zstream.opaque = Z_NULL;
  	zstream.data_type = Z_BINARY;
	int zinit;
  	if( (zinit=inflateInit(&zstream)) != Z_OK)
	{
		if (zinit==Z_MEM_ERROR)
			LOGGER_ERROR("Decompression PNG : pas assez de memoire");
		else if (zinit==Z_VERSION_ERROR)
			LOGGER_ERROR("Decompression PNG : versions de zlib incompatibles");
		else if (zinit==Z_STREAM_ERROR)
			LOGGER_ERROR("Decompression PNG : parametres invalides");
		else
			LOGGER_ERROR("Decompression PNG : echec");
		return;
 	}

  	zstream.next_in = (uint8_t*)(encoded_data + 41); // 41 = 33 header + 8(chunk idat)
  	zstream.avail_in = encoded_size;     // 45 = 41 + 4(crc)

	// Decompression du flux ligne par ligne
  	uint8_t tmp;
  	for(int h = 0; h < height; h++) {
    		zstream.next_out = &tmp;
    		zstream.avail_out = 1;
		// Decompression 1er octet de la ligne (=0 dans le cache)
    		if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) {
			LOGGER_ERROR("Decompression PNG : probleme png decompression au debut de la ligne " << h);
			break;
		}
		// Decompression des pixels de la ligne
    		zstream.next_out = (uint8_t*) (raw_data + h*linesize);
    		zstream.avail_out = linesize * sizeof(uint8_t);
    		if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) {
			LOGGER_ERROR("Decompression PNG : probleme png decompression des pixels de la ligne " << h);
			break;
		}
  	}
	// Destruction du flux
  	if (inflateEnd(&zstream)!=Z_OK)
		LOGGER_ERROR("Decompression PNG : probleme de liberation du flux");
}

/*
* Constructeur
*/

Tile::Tile(int tile_width, int tile_height, int channels, DataSource* datasource, int left, int top, int right, int bottom, int coding)
   : Image(tile_width - left - right, tile_height - top - bottom, channels), datasource(datasource), tile_width(tile_width), tile_height(tile_height), left(left), top(top), coding(coding)
{ 
    	raw_data = new uint8_t[tile_width * tile_height * channels];
   	size_t encoded_size;
    	const uint8_t* encoded_data = datasource->get_data(encoded_size);

    	if (coding==RAW_UINT8 || coding==RAW_FLOAT)
		RawDecoder::decode(encoded_data, encoded_size, raw_data);
    	else if (coding==JPEG_UINT8)
		JpegDecoder::decode(encoded_data, encoded_size, raw_data,tile_height,tile_width*channels);
    	else if (coding==PNG_UINT8)
        	PngDecoder::decode(encoded_data, encoded_size, raw_data,tile_height,tile_width*channels);
    	else
		LOGGER_ERROR("Codage de tuile inconnu");
}
