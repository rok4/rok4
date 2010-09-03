#include "TiffReader.h"
#include "TiledTiffWriter.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "tiffio.h"
#include <jpeglib.h>

void usage() {
  std::cerr << "usage : 2pivot input_file -c [none/png/jpg] -p [gray/rgb] -t [sizex] [sizey] -b [8/32] output_file";
}

int main(int argc, char **argv) {
  char* input = 0, *output = 0;
  uint32_t tilewidth = 256, tilelength = 256;
  uint16_t compression = COMPRESSION_NONE;
  uint16_t photometric = PHOTOMETRIC_RGB;
  uint32_t bitspersample = 8;
  int quality = -1;

  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
        case 'c': // compression
          if(++i == argc) {std::cerr << "Error in -c option" << std::endl; exit(2);}
          if(strncmp(argv[i], "none",4) == 0) compression = COMPRESSION_NONE;
          else if(strncmp(argv[i], "png",3) == 0) {
            compression = COMPRESSION_PNG;
            if(argv[i][3] == ':') quality = atoi(argv[i]+4);
          }
          else if(strncmp(argv[i], "jpeg",4) == 0) {
            compression = COMPRESSION_JPEG;
            if(argv[i][4] == ':') quality = atoi(argv[i]+5);
          }
          else compression = COMPRESSION_NONE;
          break;
	case 'p': // photometric
          if(++i == argc) {std::cerr << "Error in -p option" << std::endl; exit(2);}          
          if(strncmp(argv[i], "gray",4) == 0) photometric = PHOTOMETRIC_MINISBLACK;
          else if(strncmp(argv[i], "rgb",3) == 0) photometric = PHOTOMETRIC_RGB;
          else photometric = PHOTOMETRIC_RGB;
          break;

        case 't':
          if(i+2 >= argc) {std::cerr << "Error in -t option" << std::endl; exit(2);}
          tilewidth = atoi(argv[++i]);
          tilelength = atoi(argv[++i]);
          break;

	case 'b':
          if(i+1 >= argc) {std::cerr << "Error in -b option" << std::endl; exit(2);}
	  bitspersample = atoi(argv[++i]);
	  break;
        default: usage();
      }
    }
    else {
      if(input == 0) input = argv[i];
      else if(output == 0) output = argv[i];
      else {std::cerr << "argument must specify one input file and one output file" << std::endl; exit(2);}
    }
  }

  if(output == 0) {std::cerr << "argument must specify one input file and one output file" << std::endl; exit(2);}
  if(photometric == PHOTOMETRIC_MINISBLACK && compression == COMPRESSION_JPEG) {std::cerr << "Gray jpeg not supported" << std::endl; exit(2);}


  TiffReader R(input);
  uint32_t width = R.getWidth();
  uint32_t length = R.getLength();  
  TiledTiffWriter W(output, width, length, photometric, compression, quality, tilewidth, tilelength,bitspersample);

  if(width % tilewidth || length % tilelength) {std::cerr << "Image size must be a multiple of tile size" << std::endl; exit(2);}  
  int tilex = width / tilewidth;
  int tiley = length / tilelength;
 
  uint8_t* data=new uint8_t[tilelength*tilewidth*R.getSampleSize()];

  for(int y = 0; y < tiley; y++) for(int x = 0; x < tilex; x++) {
    R.getWindow(x*tilewidth, y*tilelength, tilewidth, tilelength, data);
    if(W.WriteTile(x, y, data) < 0) {std::cerr << "Error while writting tile (" << x << "," << y << ")" << std::endl; return 2;}
  }

  R.close();
  if(W.close() < 0) {std::cerr << "Error while writting index" << std::endl; return 2;}
  return 0;
}

