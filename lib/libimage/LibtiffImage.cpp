#include "LibtiffImage.h"
#include "Logger.h"
#include "Utils.h"

/**
Creation d'une LibtiffImage a partir d un fichier TIFF filename
retourne NULL en cas d erreur
*/

LibtiffImage* libtiffImageFactory::createLibtiffImage(char* filename, BoundingBox<double> bbox)
{
	int width=0,height=0,channels=0,planarconfig=0,bitspersample=0,photometric=0,compression=0,rowsperstrip=0; 
        TIFF* tif=TIFFOpen(filename, "r");
        if (tif==NULL)
        {
                LOGGER_DEBUG( "Impossible d ouvrir " << filename);
		return NULL;
        }
        else
        {
                if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &width)<1)
		{
                        LOGGER_DEBUG( "Impossible de lire la largeur de " << filename);
			return NULL;
		}
                if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &height)<1)
		{
                        LOGGER_DEBUG( "Impossible de lire la hauteur de " << filename);
			return NULL;
		}
                if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL,&channels)<1)
		{
                        LOGGER_DEBUG( "Impossible de lire le nombre de canaux de " << filename);
			return NULL;
		}
                if (TIFFGetField(tif, TIFFTAG_PLANARCONFIG,&planarconfig)<1)
		{
                        LOGGER_DEBUG( "Impossible de lire la configuration des plans de " << filename);
			return NULL;
		}
		if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE,&bitspersample)<1)
                {
                        LOGGER_DEBUG( "Impossible de lire le nombre bits par canal de " << filename);
                        return NULL;
                }
                if (TIFFGetField(tif, TIFFTAG_PHOTOMETRIC,&photometric)<1)
                {
                        LOGGER_DEBUG( "Impossible de lire la photometrie de " << filename);
                        return NULL;
                }
		if (TIFFGetField(tif, TIFFTAG_COMPRESSION,&compression)<1)
                {
                        LOGGER_DEBUG( "Impossible de lire la compression de " << filename);
                        return NULL;
                }
		if (TIFFGetField(tif, TIFFTAG_ROWSPERSTRIP,&rowsperstrip)<1)
                {
                        LOGGER_DEBUG( "Impossible de lire le nombre de lignes par bande de " << filename);
                        return NULL;
                }
        }

	if (width*height*channels!=0 && planarconfig!=PLANARCONFIG_CONTIG && tif!=NULL)
		return NULL;

	return new LibtiffImage(width,height,channels,bbox,tif,filename,bitspersample,photometric,compression,rowsperstrip);
}

/**
Creation d'une LibtiffImage en vue de creer un nouveau fichier TIFF
retourne NULL en cas d erreur
*/

LibtiffImage* libtiffImageFactory::createLibtiffImage(char* filename, BoundingBox<double> bbox, int width, int height, int channels, uint16_t bitspersample, uint16_t photometric, uint16_t compressioni, uint16_t rowsperstrip)
{
	if (width<0||height<0)
		return NULL;
        if (width*height*channels==0)
                return NULL;

	return new LibtiffImage(width,height,channels,bbox,NULL,filename,bitspersample,photometric,compressioni,rowsperstrip);
}

LibtiffImage::LibtiffImage(int width,int height, int channels, BoundingBox<double> bbox, TIFF* tif,char* name, int bitspersample, int photometric, int compression, int rowsperstrip) : Image(width,height,channels,bbox), tif(tif), bitspersample(bitspersample), photometric(photometric), compression(compression), rowsperstrip(rowsperstrip)
{
	filename = new char[LIBTIFFIMAGE_MAX_FILENAME_LENGTH];
	strcpy(filename,name);

	current_strip=-1;
	strip_size=width*channels*rowsperstrip;
	strip_buffer=new uint8_t[strip_size];
}

template<typename T>
int LibtiffImage::_getline(T* buffer, int line)
{
// le buffer est déjà alloue
// Cas RGB : canaux entrelaces (TIFFTAG_PLANARCONFIG=PLANARCONFIG_CONTIG)

// Cas Non compresse ou (compresse et 1 ligne/bande)
        if (compression==COMPRESSION_NONE || (compression!=COMPRESSION_NONE && rowsperstrip==1) ){
                if (TIFFReadScanline(tif,buffer,line,0)<0)
                        LOGGER_DEBUG("Erreur de lecture du fichier TIFF "<<TIFFFileName(tif)<<" ligne "<<line);
        }

// Cas compresse et > 1 ligne /bande
        else{
                if (line/rowsperstrip!=current_strip){
                        current_strip=line/rowsperstrip;
                        if (TIFFReadEncodedStrip(tif,current_strip,strip_buffer,strip_size)<0)
                                LOGGER_DEBUG("Erreur de lecture du fichier TIFF "<<TIFFFileName(tif)<<" ligne "<<line);
                }
                memcpy(buffer,&strip_buffer[(line%rowsperstrip)*width*channels],width*channels*sizeof(uint8_t));
        }
        return width*channels;
}

int LibtiffImage::getline(uint8_t* buffer, int line)
{
	return _getline(buffer,line);
}	

int LibtiffImage::getline(float* buffer, int line)
{
        return _getline(buffer,line);
}

LibtiffImage::~LibtiffImage()
{
//	if (tif)
//		TIFFClose(tif);
//	LOGGER_DEBUG("Destructeur LibtiffImage");
	delete [] filename;
}
