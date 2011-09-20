/**
 * \file mergeitiff.cpp
 * \brief Sous echantillonage de 4 images 
 * \author IGN
*
*/

#include "tiffio.h"
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <string.h>
#include <stdint.h>

void usage() {
	std::cerr << "Usage : merge4tiff -g gamma_correction -n nodata -c compression -r rowsperstrip -b background_image -i1 image1 -i2 image2 -i3 image3 -i4 image4 imageOut" << std::endl;
  std::cerr << "-n : this floating point value is used only for DTM."<< std::endl;
  std::cerr << "     For images (u_int8), the value this always 255 today (to be fix)"<< std::endl;
  std::cerr << "-b : the background image is mandatory for images" << std::endl;
  std::cerr << "-g : default gamma is 1.0 (have no effect)" << std::endl;
  std::cerr << "-c : compression should not be used in be4 context" << std::endl;
  std::cerr << "-r : should not be used in be4 context" << std::endl;
  std::cerr << std::endl << "Images spatial distribution :" << std::endl;
  std::cerr << "   image1 | image2" << std::endl;
  std::cerr << "   -------+-------" << std::endl;
  std::cerr << "   image3 | image4" << std::endl;
}

void error(std::string message) {
	std::cerr << message << std::endl;
	usage();
	exit(1);
}

void parseCommandLine(int argc, char* argv[],
		double& gamma,
		float& nodata,
		uint16_t& compression,
		uint32_t& rowsperstrip,
		char*& backgroundImage,
		char** inputImages,
		char*& outputImage)
{
  	gamma = 1.;
	nodata = 0.;
	compression = -1;
	rowsperstrip = -1;
	backgroundImage = 0;
	for (int i=0;i<4;i++) inputImages[i] = 0;
	outputImage = 0;

  	for(int i = 1; i < argc; i++) {
    		if(argv[i][0] == '-') {
      		switch(argv[i][1]) {
	        	case 'g': 
        	  		if(++i == argc) error("missing parameter in -g argument");
          			gamma = atof(argv[i]);
				if (gamma<=0.) error("invalid parameter in -c argument");
          			break;
	        	case 'n': 
        	  		if(++i == argc) error("missing parameter in -n argument");
          			nodata = atof(argv[i]);
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
			case 'b':
				if(++i == argc) error("missing parameter in -b argument");
				backgroundImage = argv[i];
				break;
			case 'i':
				if(++i == argc) error("missing parameter in -i argument");
				switch(argv[i-1][2]){
					case '1':
					inputImages[0]=argv[i];
					break;
					case '2':
                                        inputImages[1]=argv[i];
                                        break;
					case '3':
                                        inputImages[2]=argv[i];
                                        break;
					case '4':
                                        inputImages[3]=argv[i];
                                        break;
					default:
					break;
				}
				break;
      			}
    		}
		else
			outputImage=argv[i];
  	}

	if (outputImage==0) error ("missing output file");
  /* FIXME: NV: je commente ce test tant qu'une gestion correcte du paramètre -n n'est pas implémentée
  if  ( (inputImages[0]==0||inputImages[1]==0||inputImages[2]==0||inputImages[3]==0) && (backgroundImage==0 && nodata==0.))
		error("missing input data");
  */
}


void checkImages(
		uint32_t& width,
		uint32_t& height,
		uint32_t& rowsperstrip,
		uint16_t& bitspersample,
		uint16_t& sampleperpixel,
		uint16_t& sampleformat,
		uint16_t& photometric,
		uint16_t& compression,
		uint16_t& planarconfig,
		char* backgroundImage,
		char** inputImages,
		char* outputImage,
		TIFF* INPUT[2][2],
		TIFF*& BACKGROUND,
		TIFF*& OUTPUT
		){

	uint32_t _width,_height,_rowsperstrip;
	uint16_t _bitspersample,_sampleperpixel,_sampleformat,_photometric,_compression,_planarconfig; 	
	width=0;	

	for(int i = 0; i < 4; i++) {
		if (inputImages[i]==0){
			INPUT[i/2][i%2] = 0;
			continue;
		}
    		TIFF *input = TIFFOpen(inputImages[i], "r");
    		if(input == NULL) error("Unable to open input file: " + std::string(inputImages[i]));
    		INPUT[i/2][i%2] = input;

    		if(width == 0) { // read the parameters of the first input file
      			if( ! TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &width)                   ||
	          	! TIFFGetField(input, TIFFTAG_IMAGELENGTH, &height)                     ||
        	  	! TIFFGetField(input, TIFFTAG_BITSPERSAMPLE, &bitspersample)            ||
          		! TIFFGetFieldDefaulted(input, TIFFTAG_PLANARCONFIG, &planarconfig)     ||
	          	! TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &photometric)                ||
        	  	! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLEFORMAT, &sampleformat)     ||
          		! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &sampleperpixel))
        			error("Error reading input file: " + std::string(inputImages[i]));
      		if(compression == (uint16)(-1)) TIFFGetField(input, TIFFTAG_COMPRESSION, &compression);
      		if(rowsperstrip == (uint32)(-1)) TIFFGetField(input, TIFFTAG_ROWSPERSTRIP, &rowsperstrip);
      		if(planarconfig != 1) error("Sorry : only planarconfig = 1 is supported");
		if (width%2!=0 || height%2) error ("Sorry : only even dimensions for input images are supported");
    		}
		
    		else { // check if the others input files have compatible parameters      
      			uint32 _width, _height;
      			uint16 _bitspersample, _sampleperpixel, _photometric, _planarconfig, _sampleformat;
      			if( ! TIFFGetField(input, TIFFTAG_IMAGEWIDTH, &_width)                        ||
          			! TIFFGetField(input, TIFFTAG_IMAGELENGTH, &_height)                      ||
	          		! TIFFGetField(input, TIFFTAG_BITSPERSAMPLE, &_bitspersample)             ||
        	  		! TIFFGetFieldDefaulted(input, TIFFTAG_PLANARCONFIG, &_planarconfig)      ||
		          	! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLESPERPIXEL, &_sampleperpixel) ||
	  		        ! TIFFGetField(input, TIFFTAG_PHOTOMETRIC, &_photometric)                 ||
		          	! TIFFGetFieldDefaulted(input, TIFFTAG_SAMPLEFORMAT, &_sampleformat) )
		        		error("Error reading file " + std::string(inputImages[i]));

	        	if(_width != width || _height != height || _bitspersample != bitspersample 
        			|| _planarconfig != planarconfig || _photometric != photometric || _sampleperpixel != sampleperpixel) 
          			error("Error : all input files must have the same parameters (width, height, etc...)");
    		}
	}

	BACKGROUND=0;

	if (inputImages[0]&&inputImages[1]&&inputImages[2]&&inputImages[3])
		backgroundImage=0;

	if (backgroundImage){
		BACKGROUND=TIFFOpen(backgroundImage, "r");
		if (BACKGROUND==NULL) error("Unable to open background image: "+std::string(backgroundImage));
		if( ! TIFFGetField(BACKGROUND, TIFFTAG_IMAGEWIDTH, &_width)                        ||
	            ! TIFFGetField(BACKGROUND, TIFFTAG_IMAGELENGTH, &_height)                      ||
                    ! TIFFGetField(BACKGROUND, TIFFTAG_BITSPERSAMPLE, &_bitspersample)             ||
                    ! TIFFGetFieldDefaulted(BACKGROUND, TIFFTAG_PLANARCONFIG, &_planarconfig)      ||
                    ! TIFFGetFieldDefaulted(BACKGROUND, TIFFTAG_SAMPLESPERPIXEL, &_sampleperpixel) ||
                    ! TIFFGetField(BACKGROUND, TIFFTAG_PHOTOMETRIC, &_photometric)                 ||
                    ! TIFFGetFieldDefaulted(BACKGROUND, TIFFTAG_SAMPLEFORMAT, &_sampleformat) )
          	      error("Error reading file " + std::string(backgroundImage));
		if(_width != width || _height != height || _bitspersample != bitspersample
                	|| _planarconfig != planarconfig || _photometric != photometric || _sampleperpixel != sampleperpixel)
                        error("Error : all input files must have the same parameters (width, height, etc...)");
	}

  	OUTPUT = TIFFOpen(outputImage, "w");
	if(OUTPUT == NULL) error("Unable to open output file: " + std::string(outputImage));
  	if(! TIFFSetField(OUTPUT, TIFFTAG_IMAGEWIDTH, width)               ||
     		! TIFFSetField(OUTPUT, TIFFTAG_IMAGELENGTH, height)             ||
     		! TIFFSetField(OUTPUT, TIFFTAG_BITSPERSAMPLE, bitspersample)    ||
     		! TIFFSetField(OUTPUT, TIFFTAG_SAMPLESPERPIXEL, sampleperpixel) ||
     		! TIFFSetField(OUTPUT, TIFFTAG_PHOTOMETRIC, photometric)        ||
     		! TIFFSetField(OUTPUT, TIFFTAG_ROWSPERSTRIP, rowsperstrip)      ||
     		! TIFFSetField(OUTPUT, TIFFTAG_PLANARCONFIG, planarconfig)      ||
     		! TIFFSetField(OUTPUT, TIFFTAG_COMPRESSION, compression)        ||
     		! TIFFSetField(OUTPUT, TIFFTAG_SAMPLEFORMAT, sampleformat))
        	error("Error writting output file: " + std::string(outputImage));     
}

int merge4float32(uint32_t width, uint32_t height, uint16_t sampleperpixel,float nodata, TIFF* BACKGROUND, TIFF* INPUT[2][2], TIFF* OUTPUT) {
	int nbsamples = width * sampleperpixel;
	uint8  line_background[nbsamples];
  	float  line1[2*nbsamples];
  	float  line2[2*nbsamples];
  	float  line_out[nbsamples];
	int left,right;

	memset (line_background, nodata, nbsamples); 

  	for(int y = 0; y < 2; y++){
		if (INPUT[y][0]) left=0; else left=nbsamples/2;
                if (INPUT[y][1]) right=nbsamples; else right=nbsamples/2;
    		for(uint32 h = 0; h < height/2; h++) {
			if (BACKGROUND)
                                if (TIFFReadScanline(BACKGROUND, line_background,y*height/2 + h)==-1) error("Unable to read data");
                        if (INPUT[y][0])
                                if (TIFFReadScanline(INPUT[y][0], line1, 2*h)==-1) error("Unable to read data");
                        if (INPUT[y][1])
                                if (TIFFReadScanline(INPUT[y][1], line1 + nbsamples, 2*h)==-1) error("Unable to read data");
                        if (INPUT[y][0])
                                if (TIFFReadScanline(INPUT[y][0], line2, 2*h+1)==-1) error("Unable to read data");
                        if (INPUT[y][1])
                                if (TIFFReadScanline(INPUT[y][1], line2 + nbsamples, 2*h+1)==-1) error("Unable to read data");
                        memcpy(line_out,line_background,nbsamples);
      			for(int pos_in = 2*left, pos_out = left; pos_out < right; pos_in += sampleperpixel) 
        			for(int j = sampleperpixel; j--; pos_in++) {
		  			if (line1[pos_in] != nodata && line1[pos_in + sampleperpixel] != nodata && line2[pos_in] != nodata && line2[pos_in + sampleperpixel] != nodata)
			  			line_out[pos_out++] = (line1[pos_in] + line1[pos_in + sampleperpixel] + line2[pos_in] + line2[pos_in + sampleperpixel])/(float)4;
					else
			  			line_out[pos_out++] = nodata;
				}
			if(TIFFWriteScanline(OUTPUT, line_out, y*height/2 + h) == -1) error("Unable to write data");
    		}
	}
	if (BACKGROUND) TIFFClose(BACKGROUND); 
	for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) TIFFClose(INPUT[i][j]);
	TIFFClose(OUTPUT);
	return 0;
};

int merge4uint8(uint32_t width, uint32_t height, uint16_t sampleperpixel,double gamma, TIFF* BACKGROUND, TIFF* INPUT[2][2], TIFF* OUTPUT) {
	uint8 MERGE[1024];
	for(int i = 0; i <= 1020; i++) MERGE[i] = 255 - (uint8) round(pow(double(1020 - i)/1020., gamma) * 255.);

	int nbsamples = width * sampleperpixel;
	uint8  line_background[nbsamples];
	uint8  line1[2*nbsamples];
	uint8  line2[2*nbsamples];
	uint8  line_out[nbsamples];
	int left,right;
	
	// FIXME: il faudrait initialiser la ligne de fond avec la couleur nodata 
	//        du parametre, mais il est en float....
	memset (line_background, 255, nbsamples); 
	
  	for(int y = 0; y < 2; y++){
		if (INPUT[y][0]) left=0; else left=nbsamples/2;
		if (INPUT[y][1]) right=nbsamples; else right=nbsamples/2;
    		for(uint32 h = 0; h < height/2; h++) {
			if (BACKGROUND)
				if (TIFFReadScanline(BACKGROUND, line_background,y*height/2 + h)==-1) error("Unable to read data");
			if (INPUT[y][0])
				if (TIFFReadScanline(INPUT[y][0], line1, 2*h)==-1) error("Unable to read data");
		        if (INPUT[y][1])
				if (TIFFReadScanline(INPUT[y][1], line1 + nbsamples, 2*h)==-1) error("Unable to read data");
		        if (INPUT[y][0])
				if (TIFFReadScanline(INPUT[y][0], line2, 2*h+1)==-1) error("Unable to read data");
		        if (INPUT[y][1])
				if (TIFFReadScanline(INPUT[y][1], line2 + nbsamples, 2*h+1)==-1) error("Unable to read data");
			memcpy(line_out,line_background,nbsamples);

	      		for(int pos_in = 2*left, pos_out = left; pos_out < right; pos_in += sampleperpixel)
        			for(int j = sampleperpixel; j--; pos_in++) 
          				line_out[pos_out++] = MERGE[((int)line1[pos_in] + (int)line1[pos_in + sampleperpixel]) + ((int)line2[pos_in] + (int)line2[pos_in + sampleperpixel])];

      			if(TIFFWriteScanline(OUTPUT, line_out, y*height/2 + h) == -1) error("Unable to write data");
    		}
	}
  	if (BACKGROUND) TIFFClose(BACKGROUND);
	for(int i = 0; i < 2; i++) for(int j = 0; j < 2; j++) if (INPUT[i][j]) TIFFClose(INPUT[i][j]);
  	TIFFClose(OUTPUT);
  	return 0;
}

int main(int argc, char* argv[]) {
	double gamma;
	float nodata;
	uint32_t width,height,rowsperstrip;
	uint16_t compression,bitspersample,sampleperpixel,sampleformat,photometric,planarconfig;
	char* backgroundImage;
	char* inputImages[4];
	char* outputImage;
	TIFF* INPUT[2][2];
	TIFF* BACKGROUND;
	TIFF* OUTPUT;

	parseCommandLine(argc, argv, gamma,nodata,compression,rowsperstrip,backgroundImage,inputImages,outputImage);

	checkImages(width,height,rowsperstrip,bitspersample,sampleperpixel,sampleformat,photometric,compression,planarconfig,backgroundImage,inputImages,outputImage,INPUT,BACKGROUND,OUTPUT);

	// Cas MNT
  	if (sampleformat == 3 && bitspersample == 32)
    		return merge4float32(width,height,sampleperpixel,nodata,BACKGROUND,INPUT,OUTPUT);
	// Cas images
	else if (sampleformat == 1 && bitspersample == 8)
    		return merge4uint8(width,height,sampleperpixel,gamma,BACKGROUND,INPUT,OUTPUT);
	else
		error("sampleformat/bitspersample not rsdupported");
}

