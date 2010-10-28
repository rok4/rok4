#ifndef _JPEGENCODER_
#define _JPEGENCODER_

#include "Data.h"
#include "Image.h"
#include "jpeglib.h"

/** D */
class JPEGEncoder : public DataStream {  
private:
	Image *image;

	int status;
	uint8_t *linebuffer;

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	static void init_destination (jpeg_compress_struct *cinfo) {return;}
	static boolean empty_output_buffer (jpeg_compress_struct *cinfo) {return false;}
	static void term_destination (jpeg_compress_struct *cinfo) {return;}

public:
	/** D */
	JPEGEncoder(Image* image) : image(image), status(-1) {
		cinfo.err = jpeg_std_error(&jerr);
		jpeg_create_compress(&cinfo);
		cinfo.dest = new jpeg_destination_mgr;

		cinfo.dest->init_destination = init_destination;
		cinfo.dest->empty_output_buffer = empty_output_buffer;
		cinfo.dest->term_destination = term_destination;
		cinfo.dest->next_output_byte = 0;
		cinfo.dest->free_in_buffer = 0;

		cinfo.image_width = image->width;
		cinfo.image_height = image->height;
		cinfo.input_components = image->channels;
		if(image->channels == 3) cinfo.in_color_space = JCS_RGB;
		else if(image->channels == 1) cinfo.in_color_space = JCS_GRAYSCALE;
		//  else ERROR !!!!!

		jpeg_set_defaults(&cinfo);

		linebuffer = new uint8_t[image->width*image->channels];
	}

	/** D */
	~JPEGEncoder() {
		std::cerr << "delete JPEGEncoder" << std::endl;
		jpeg_destroy_compress(&cinfo);
		delete[] linebuffer;
		delete image;
	}

	/**
	 * Lecture du flux JPEG
	 */
	size_t read(uint8_t *buffer, size_t size) {
		assert(size >= 1024);

		// On initialise le buffer d'écriture de la libjpeg
		cinfo.dest->next_output_byte = buffer;
		cinfo.dest->free_in_buffer = size;

		// Première passe : on initialise la compression (écrit déjà quelques données)
		if(status < 0 && cinfo.dest->free_in_buffer >= 1024) {
			jpeg_start_compress(&cinfo, true);
			status = 0;
		}
		while(cinfo.next_scanline < cinfo.image_height && cinfo.dest->free_in_buffer >= 1024) {
			image->getline(linebuffer, cinfo.next_scanline);
			if(jpeg_write_scanlines(&cinfo, &linebuffer, 1) < 1) break;
		}

		if(status == 0 && cinfo.next_scanline >= cinfo.image_height && cinfo.dest->free_in_buffer >= 100) {
			jpeg_finish_compress(&cinfo);
			status = 1;
		}

		return (size - cinfo.dest->free_in_buffer);
	}

	bool eof() {return(cinfo.next_scanline >= cinfo.image_height);}

	std::string gettype() {return "image/jpeg";}
};

#endif


