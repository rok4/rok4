#include "Tile.h"
#include "jpeglib.h"
#include "zlib.h"

/* 
 * Fonction déclarée pour la libjpeg
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


void JpegDecoder::decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize) { 
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  cinfo.src = new jpeg_source_mgr;
  cinfo.src->init_source = init_source;
  cinfo.src->fill_input_buffer = fill_input_buffer;
  cinfo.src->skip_input_data = skip_input_data;
  cinfo.src->resync_to_restart = jpeg_resync_to_restart; /* use default method */
  cinfo.src->term_source = term_source;

  cinfo.src->next_input_byte = encoded_data;
  cinfo.src->bytes_in_buffer = encoded_size;

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  //rawdata = new typename pixel_t::data_t[this->height * this->linesize];
  while (cinfo.output_scanline < (unsigned int) height) {
    uint8_t *line = raw_data + cinfo.output_scanline * linesize;
    if(jpeg_read_scanlines(&cinfo, &line, 1) < 1)
        {
        LOGGER_DEBUG("Probleme lecture tuile Jpeg");
        break;
        }
  }

  jpeg_finish_decompress(&cinfo);
  delete cinfo.src;
  jpeg_destroy_decompress(&cinfo);


    LOGGER_DEBUG( " decode " << (int) encoded_size );
}

void PngDecoder::decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize) {
  LOGGER_DEBUG( "PngTile: decompress");
  z_stream zstream;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  zstream.opaque = Z_NULL;
  zstream.data_type = Z_BINARY;

  if(inflateInit(&zstream) != Z_OK)
	LOGGER_ERROR("probleme png decompression");

  zstream.next_in = (uint8_t*)(encoded_data + 41); // 41 = 33 header + 8(chunk idat)
  zstream.avail_in = encoded_size;     // 45 = 41 + 4(crc)

//  this->rawdata = new typename pixel_t::data_t[this->height * this->linesize];
//  zstream.next_out = (uint8_t*) rawdata;
//  zstream.avail_out = this->height * this->linesize * sizeof(typename pixel_t::data_t);

  uint8_t tmp;
  for(int h = 0; h < height; h++) {
    zstream.next_out = &tmp;
    zstream.avail_out = 1;
    if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK)
	LOGGER_ERROR("probleme png decompression");
    zstream.next_out = (uint8_t*) (raw_data + h*linesize);
    zstream.avail_out = linesize * sizeof(uint8_t);
    if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) break;
  }
  inflateEnd(&zstream);
}


