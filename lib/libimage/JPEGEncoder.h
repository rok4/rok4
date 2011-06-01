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
	JPEGEncoder(Image* image);

	/** D */
	~JPEGEncoder();

	/**
	 * Lecture du flux JPEG
	 */
	size_t read(uint8_t *buffer, size_t size);

	bool eof() {return(cinfo.next_scanline >= cinfo.image_height);}

	std::string gettype() {return "image/jpeg";}
	
	int getHttpStatus() {return 200;}
};

#endif


