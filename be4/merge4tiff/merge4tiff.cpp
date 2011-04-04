#include "tiffio.h"


#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string.h>

using namespace std;

void usage() {
  cerr << "Usage : merge4tiff image1 image2 image3 image4 imageOut" << endl;;
}

void error(string message) {
  cerr << message << endl;
  exit(1);
}


TIFF *INPUT[2][2];
TIFF *OUTPUT;


uint32 width, height, rowsperstrip = -1;
uint16 bitspersample, sampleperpixel, photometric, compression = -1, planarconfig, nb_extrasamples, sampleformat;
uint16 *extrasamples;
float nodata_alti = -99999;

uint8 MERGE[1024];

void init(int argc, char* argv[]) {
  char* files[5];
  int nb_file = 0;
  double gamma = 1.;
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
        case 'g': 
          if(++i == argc) error("missing parameter in -g argument");
          gamma = atof(argv[i]);
          break;
        case 'n': 
          if(++i == argc) error("missing parameter in -n argument");
          nodata_alti = atof(argv[i]);
          break;
        case 'h': 
          usage();
          exit(0);
        case 'c': // compression
          if(++i == argc) error("Error in -c option");
          if(strncmp(argv[i], "none",4) == 0) compression = COMPRESSION_NONE;
          else if(strncmp(argv[i], "zip",3) == 0) compression = COMPRESSION_ADOBE_DEFLATE;
          else if(strncmp(argv[i], "packbits",8) == 0) compression = COMPRESSION_PACKBITS;
          else if(strncmp(argv[i], "jpeg",4) == 0) compression = COMPRESSION_JPEG;
          else if(strncmp(argv[i], "lzw",3) == 0) compression = COMPRESSION_LZW;
          else compression = COMPRESSION_NONE;
          break;
        case 'r':
          if(++i == argc) error("missing parameter in -r argument");
          rowsperstrip = atoi(argv[i]);
          break;
      }
    }
    else {
      if(nb_file >= 5) error("Error : argument must specify four input files and one output file");
      files[nb_file++] = argv[i];
    }
  }
  if(nb_file != 5) error("Error : argument must specify four input files and one output file");
 
  for(int i = 0; i < 4; i++) {
    TIFF *input = TIFFOpen(files[i], "r");
    if(input == NULL) error("Unable to open input file: " + string(files[i]));
    INPUT[i/2][i%2] = input;

    if(i == 0) { // read the parameters of the first input file
      if( ! TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &width)                       ||
          ! TIFFGetField(input, TIFFTAG_IMAGELENGTH, &height)                     ||
          ! TIFFGetField(input, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
          ! TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &photometric)                ||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLEFORMAT, &sampleformat)     ||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel)||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_EXTRASAMPLES, &nb_extrasamples, &extrasamples))
        error("Error reading input file: " + string(files[i]));

      if(compression == (uint16)(-1)) TIFFGetField(input, TIFFTAG_COMPRESSION, &compression);
      if(rowsperstrip == (uint32)(-1)) TIFFGetField(input, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);

      if(planarconfig != 1) error("Sorry : only planarconfig = 1 is supported");
    }
    else { // check if the 3 others input files have compatible parameters      
      uint32 _width, _height;
      uint16 _bitspersample, _sampleperpixel, _photometric, _planarconfig, _sampleformat;
      if( ! TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &_width)                        ||
          ! TIFFGetField(input, TIFFTAG_IMAGELENGTH, &_height)                      ||
          ! TIFFGetField(input, TIFFTAG_BITSPERSAMPLE, &_bitspersample)             ||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_PLANARCONFIG, &_planarconfig)      ||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &_sampleperpixel) ||
          ! TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &_photometric)                 ||
          ! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLEFORMAT, &_sampleformat) )
        error("Error reading file " + string(files[i]));

        if(_width != width || _height != height || _bitspersample != bitspersample 
        || _planarconfig != planarconfig || _photometric != photometric || _sampleperpixel != sampleperpixel) 
          error("Error : all input files must have the same parameters (width, height, etc...)");
    }
  }

  //  cerr << gamma << endl;
  if (sampleformat == (int)1) {
    for(int i = 0; i <= 1020; i++) MERGE[i] = 255 - (uint8) round(pow(double(1020 - i)/1020., gamma) * 255.);
  //  for(int i = 0; i < 1024; i++) cerr << i << " " << int(MERGE[i]) << endl;
  }

  OUTPUT = TIFFOpen(files[4], "w");
  if(OUTPUT == NULL) error("Unable to open output file: " + string(files[4]));
  if(! TIFFSetField(OUTPUT, TIFFTAG_IMAGEWIDTH, width)               ||
     ! TIFFSetField(OUTPUT, TIFFTAG_IMAGELENGTH, height)             ||
     ! TIFFSetField(OUTPUT, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
     ! TIFFSetField(OUTPUT, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel) ||
     ! TIFFSetField(OUTPUT, TIFFTAG_PHOTOMETRIC, photometric)        ||
     ! TIFFSetField(OUTPUT, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
     ! TIFFSetField(OUTPUT, TIFFTAG_PLANARCONFIG, planarconfig)      ||
     ! TIFFSetField(OUTPUT, TIFFTAG_COMPRESSION, compression)        ||
     ! TIFFSetField(OUTPUT, TIFFTAG_SAMPLEFORMAT, sampleformat)      ||
     (nb_extrasamples && ! TIFFSetField(OUTPUT, TIFFTAG_EXTRASAMPLES, nb_extrasamples, extrasamples)))
        error("Error writting output file: " + string(files[4]));     
}

int float32_main() {

  int nbsamples = width * sampleperpixel;
  float  line1[2*nbsamples];
  float  line2[2*nbsamples];
  float  line_out[nbsamples];

  for(int y = 0; y < 2; y++)
    for(uint32 h = 0; h < height/2; h++) {

      if(  TIFFReadScanline(INPUT[y][0], line1, 2*h)               == -1
        || TIFFReadScanline(INPUT[y][1], line1 + nbsamples, 2*h)   == -1
        || TIFFReadScanline(INPUT[y][0], line2, 2*h+1)             == -1
        || TIFFReadScanline(INPUT[y][1], line2 + nbsamples, 2*h+1) == -1) error("Unable to read data");

      for(int pos_in = 0, pos_out = 0; pos_out < nbsamples; pos_in += sampleperpixel) {
        for(int j = sampleperpixel; j--; pos_in++) {
		  float val;
		  if (line1[pos_in] != nodata_alti && line1[pos_in + sampleperpixel] != nodata_alti && line2[pos_in] != nodata_alti && line2[pos_in + sampleperpixel] != nodata_alti) { // presence de nodata
			  val = (line1[pos_in] + line1[pos_in + sampleperpixel] + line2[pos_in] + line2[pos_in + sampleperpixel])/(float)4;
			}  else {
			  val = nodata_alti;
		  }
          line_out[pos_out++] = val;
		};
	  };
	
      if(TIFFWriteScanline(OUTPUT, line_out, y*height/2 + h) == -1) error("Unable to write data");

    }
  
  for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) TIFFClose(INPUT[i][j]);
  TIFFClose(OUTPUT);
  return 0;
};

int uint8_main() {

  int nbsamples = width * sampleperpixel;
  uint8  line1[2*nbsamples];
  uint8  line2[2*nbsamples];
  uint8  line_out[nbsamples];

  for(int y = 0; y < 2; y++)
    for(uint32 h = 0; h < height/2; h++) {

      if(  TIFFReadScanline(INPUT[y][0], line1, 2*h)               == -1
        || TIFFReadScanline(INPUT[y][1], line1 + nbsamples, 2*h)   == -1
        || TIFFReadScanline(INPUT[y][0], line2, 2*h+1)             == -1
        || TIFFReadScanline(INPUT[y][1], line2 + nbsamples, 2*h+1) == -1) error("Unable to read data");

      for(int pos_in = 0, pos_out = 0; pos_out < nbsamples; pos_in += sampleperpixel) 
        for(int j = sampleperpixel; j--; pos_in++) 
          line_out[pos_out++] = MERGE[((int)line1[pos_in] + (int)line1[pos_in + sampleperpixel]) + ((int)line2[pos_in] + (int)line2[pos_in + sampleperpixel])];

      if(TIFFWriteScanline(OUTPUT, line_out, y*height/2 + h) == -1) error("Unable to write data");
    }
  
  for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) TIFFClose(INPUT[i][j]);
  TIFFClose(OUTPUT);
  return 0;
}

int main(int argc, char* argv[]) {

  init(argc, argv);
  if (sampleformat == 3 && bitspersample == 32) { // le contenu est en flottant 32bits
    return float32_main();
  } else if (sampleformat == 1 && bitspersample == 8) { // le contenu est en entier 8bits
    return uint8_main();
  } else if (sampleformat == 1 && bitspersample == 32) { // cas des MTDs
    error("Les dalles de masque ne sont pas encore gérées par merge4tiff.");
    return -1;
  } else { 
    error("Le format des données (bitspersample et sampleformat) n'est pas encore géré par merge4tiff.");  
    return -1;
  };

}

