#include "Tile.h"



template<class Decoder>
Tile<Decoder>::Tile(int tile_width, int tile_height, int left, int top, int right, int bottom, HttpResponse* data) : Image(tile_width - right - left, tile_height - top - bottom, Decoder::channels), tile_width(tile_width), tile_height(tile_height), data(data) {

  rawdata = Decoder::decode(data->data, data->size);



}


template<typename data_t, int channels>
int Tile<data_t, channels>::getline(data_t *buffer, int line, int offset, int nb_pixel) {
  assert(offset + nb_pixel <= this->width);
  if(!rawdata) if(!decode()) return 0;
  memcpy(buffer, rawdata + line * this->linesize + channels*offset, nb_pixel*channels*sizeof(data_t));
  return nb_pixel;
}



template<typename data_t, int channels>
Tile<data_t, channels>::~Tile() {
  if(rawdata) delete[] rawdata;
  if(data) FileManager::releasetile(data); // HttpResponse::data
}


template<typename data_t, int channels>
int RawTile<data_t, channels>::decode()  {
  this->rawdata = (data_t*) this->data;
  return (this->rawdata != 0);
}


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

/*
 * Décodeur jpeg utilisant la libjpeg
 */

template<typename data_t, int channels>
int JpegTile<data_t, channels>::decode() {
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

  cinfo.src->next_input_byte = this->data;
  cinfo.src->bytes_in_buffer = this->size;

  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  this->rawdata = new data_t[this->height * this->linesize];
  while (cinfo.output_scanline < (unsigned int) this->height) {    
    data_t* line = this->rawdata + cinfo.output_scanline * this->linesize;
    if(jpeg_read_scanlines(&cinfo, &line, 1) < 1) return 0;
  }

  jpeg_finish_decompress(&cinfo);
  delete cinfo.src;
  jpeg_destroy_decompress(&cinfo);
  return 1;
}


template<typename data_t, int channels>
JpegTile<data_t, channels>::JpegTile(int width, int height, const data_t nodata_color[channels]) : Tile<data_t, channels>(width, height, "image/jpeg") {


  RawTile<data_t, channels> *rawtile = new RawTile<data_t, channels>(width, height, nodata_color);
  JPEGEncoder *encoder = new JPEGEncoder(rawtile);
  size_t max_size = 1024 + width*height*channels*sizeof(data_t);
  this->data = new uint8_t[max_size];

  // On écrit l'image encodée dans data et on met à jour HttpResponse::size
  for(size_t sz = 1; sz; this->size += sz) { // Boucle tant que encoder.getdata renvoie des données.
    sz = encoder->getdata((uint8_t*)this->data, max_size - this->size);
  }
  assert(this->size < max_size);
  
  // On copie le rawdata de rawtile.
  this->rawdata = new data_t[width * height * channels];
  rawtile->getdata(this->rawdata, width * height * channels * sizeof(data_t));
  delete encoder;
}


template<typename data_t, int channels>
PngTile<data_t, channels>::PngTile(int width, int height, const data_t nodata_color[channels]) : Tile<data_t, channels>(width, height, "image/jpeg") {

  RawTile<data_t, channels> *rawtile = new RawTile<data_t, channels>(width, height, nodata_color);
  PNGEncoder *encoder = new PNGEncoder(rawtile);
  size_t max_size = 1024 + width*height*channels*sizeof(data_t);
  this->data = new uint8_t[max_size];

  // On écrit l'image encodée dans data et on met à jour HttpResponse::size
  for(size_t sz = 1; sz; this->size += sz) { // Boucle tant que encoder.getdata renvoie des données.
    sz = encoder->getdata((uint8_t*)this->data, max_size - this->size);
  }
  assert(this->size < max_size);
  
  // On copie le rawdata de rawtile.
  this->rawdata = new data_t[width * height * channels];
  rawtile->getdata(this->rawdata, width * height * channels * sizeof(data_t));
  delete encoder;
}




template<typename data_t, int channels>
int PngTile<data_t, channels>::decode() {
  std::cerr << "PngTile: decompress" << std::endl;
  z_stream zstream;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  zstream.opaque = Z_NULL;
  zstream.data_type = Z_BINARY;

  if(inflateInit(&zstream) != Z_OK) return -1;

  zstream.next_in = (uint8_t*)(this->data + 41); // 41 = 33 header + 8(chunk idat)
  zstream.avail_in = this->size;     // 45 = 41 + 4(crc)

  this->rawdata = new data_t[this->height * this->linesize];
//  zstream.next_out = (uint8_t*) rawdata;
//  zstream.avail_out = this->height * this->linesize * sizeof(typename pixel_t::data_t);

  uint8_t tmp;
  for(int h = 0; h < this->height; h++) {
    zstream.next_out = &tmp;
    zstream.avail_out = 1;
    if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) return -1;
    zstream.next_out = (uint8_t*) (this->rawdata + h*this->linesize);
    zstream.avail_out = this->linesize * sizeof(data_t);
    if(inflate(&zstream, Z_SYNC_FLUSH) != Z_OK) break;
  }
  inflateEnd(&zstream);
  return 1;
}

/*

template<typename data_t, int channels>
PngTile<data_t, channels>::~PngTile() {
  std::cerr << "delete PngTile" << std::endl;
  if(rawdata) delete[] rawdata;
  FileManager::releasetile(pngdata);
}
*/

/*

template<typename data_t, int channels>
size_t EmptyImage<data_t, channels>::getrawdata(typename pixel_t::data_t *buffer, size_t size, size_t offset) { // TODO: optimiser le constructeur pour détecter des couloeur grises.
  assert(offset + size <= this->height*this->linesize);
  if(line) { // 
    for(size_t pos = 0; pos < size;) {   
      size_t offset_line = (offset+pos) % this->linesize;
      size_t sz = std::min(this->linesize - offset_line, size - pos);
      memcpy(buffer + pos, line + offset_line, sz*sizeof(typename pixel_t::data_t));
      pos += sz;
    }
  }
  else memset(buffer, value, size); // Si pas de ligne remplir par l'octet value.
  return size;
}
*/

/*
template class RawTile<pixel_rgb>;
template class RawTile<pixel_float>;

//template class EmptyTile<pixel_rgb>;
//template class EmptyTile<pixel_gray>;
template class JpegTile<pixel_rgb>;
template class PngTile<pixel_gray>;
*/
