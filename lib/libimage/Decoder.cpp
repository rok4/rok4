
#include "Decoder.h"
#include <setjmp.h>
#include "Logger.h"

#include "jpeglib.h"
#include "zlib.h"
#include "byteswap.h"


/* 
 * Fonctions déclarées pour la libjpeg
 */
void init_source (jpeg_decompress_struct *cinfo) {}

boolean fill_input_buffer (jpeg_decompress_struct *cinfo){return false;}

void skip_input_data (jpeg_decompress_struct *cinfo, long num_bytes) {
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


const uint8_t* JpegDecoder::decode(DataSource* source, size_t &size) {
	size = 0;
	if(!source) return 0;

	size_t encSize;
	const uint8_t* encData = source->getData(encSize);

	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;

	// Initialisation de cinfo
	jpeg_create_decompress(&cinfo);

	cinfo.src = new jpeg_source_mgr;
	cinfo.src->init_source = init_source;
	cinfo.src->fill_input_buffer = fill_input_buffer;
	cinfo.src->skip_input_data = skip_input_data;
	cinfo.src->resync_to_restart = jpeg_resync_to_restart; // use default method 
	cinfo.src->term_source = term_source;

	cinfo.src->next_input_byte = encData;
	cinfo.src->bytes_in_buffer = encSize;

	// Initialisation de la gestion des erreurs
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;

	// Initialisation du contexte retourne par setjmp pour my_error_exit
	if (setjmp(jerr.setjmp_buffer)) {
		delete cinfo.src;		
		jpeg_destroy_decompress(&cinfo);
		return 0;
	}

	uint8_t* raw_data = 0;

	// Lecture
	if (jpeg_read_header(&cinfo, TRUE)==JPEG_HEADER_OK) {
		
		int linesize = cinfo.image_width * cinfo.num_components;
		size = linesize * cinfo.image_height;
		raw_data = new uint8_t[size];

		// TODO: définir J_COLOR_SPACE out_color_space en fonction du nombre de canal ? 
		// Vérifier que le jpeg monocanal marche ???

		jpeg_start_decompress(&cinfo);
		while (cinfo.output_scanline < cinfo.image_height) {
			uint8_t *line = raw_data + cinfo.output_scanline * linesize;

			if(jpeg_read_scanlines(&cinfo, &line, 1) < 1) {
				jpeg_finish_decompress(&cinfo);
				delete cinfo.src;				
				jpeg_destroy_decompress(&cinfo);				
				LOGGER_ERROR("Probleme lecture tuile Jpeg");
				delete[] raw_data;
				size = 0;
				return 0;
			}

		}

		jpeg_finish_decompress(&cinfo);
	}
	else 
		LOGGER_ERROR("Erreur de lecture en tete jpeg");


	// Destruction de cinfo
	delete cinfo.src;
	jpeg_destroy_decompress(&cinfo);
	return raw_data;
}

/**
 * Decodage de donnee PNG
 */
const uint8_t* PngDecoder::decode(DataSource* source, size_t &size) {

//	LOGGER(DEBUG) << (intptr_t) source << std::endl;
	size = 0;
	if(!source) return 0;

	size_t encSize;
	const uint8_t* encData = source->getData(encSize);

	if(!encData) return 0;

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
		return 0;
	}

	int height = bswap_32(*((const uint32_t*)(encData + 16)));
	int width = bswap_32(*((const uint32_t*)(encData + 20)));
	// TODO: vérifier cohérence header... 
	int channels;

//	TODO if(encData[24] != 8) ERROR

	switch(encData[25]) {
		case 0: channels = 1; break;
		case 2: channels = 3; break;
		default:; // TODO ERROR;
	}

	uint8_t* raw_data = new uint8_t[height * width * channels];
	int linesize = width * channels;

	zstream.next_in = (uint8_t*)(encData + 41); // 41 = 33 header + 8(chunk idat)
	zstream.avail_in = encSize - 57;     // 57 = 41 + 4(crc) + 12(IEND)

	// Decompression du flux ligne par ligne
	uint8_t tmp;
	for(int h = 0; h < height; h++) {
		zstream.next_out = &tmp;
		zstream.avail_out = 1;
		// Decompression 1er octet de la ligne (=0 dans le cache)
		if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) {
			LOGGER_ERROR("Decompression PNG : probleme png decompression au debut de la ligne " << h);
			delete[] raw_data;
			return 0;			
		}
		// Decompression des pixels de la ligne
		zstream.next_out = (uint8_t*) (raw_data + h*linesize);
		zstream.avail_out = linesize * sizeof(uint8_t);
		if(int err = inflate(&zstream, Z_SYNC_FLUSH)) {

			if(err == Z_STREAM_END && h == height-1) break; // fin du fichier OK.

			LOGGER_ERROR("Decompression PNG : probleme png decompression des pixels de la ligne " << h << " " << err);
			delete[] raw_data;
			return 0;
		}
	}
	// Destruction du flux
	if (inflateEnd(&zstream)!=Z_OK) {
		LOGGER_ERROR("Decompression PNG : probleme de liberation du flux");
		delete[] raw_data;
		return 0;		
	}

	size = width * height * channels;

	return raw_data;
}
