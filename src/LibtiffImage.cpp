#include "LibbtiffImage.h"



LibtiffImage::LibtiffImage(char* filename)
{
	width=height=channels=planarconfig=0;
	tif=TIFFOpen(filename, "r");
	if (tif==NULL)
		LOGGER_DEBUG( "Impossible d ouvrir " << filename);
	else
	{
		if (TIFFGetField(tif1, TIFFTAG_IMAGEWIDTH, &width)<1)
			LOGGER_DEBUG( "Impossible de lire la largeur de " << filename);
		if (TIFFGetField(tif1, TIFFTAG_IMAGEWIDTH, &height)<1)
			LOGGER_DEBUG( "Impossible de lire la hauteur de " << filename);
		if (TIFFGetField(tif1, TIFFTAG_SAMPLESPERPIXEL,&channels)<1)
			LOGGER_DEBUG( "Impossible de lire le nombre de canaux de " << filename);
		if (TIFFGetField(tif1, TIFFTAG_PLANARCONFIG,&planarconfig)<1)
                        LOGGER_DEBUG( "Impossible de lire la configuration des plans de " << filename);
	}
}

int getline(uint8_t* buffer, int line)
{
// le buffer est déjà alloue
// Cas RGB : canaux entralaces (TIFFTAG_PLANARCONFIG=PLANARCONFIG_CONTIG)
	TIFFReadScanLine(tif,buffer,line,0);	
	return width*channels;
}

bool isValid()
{
	return (width*height*channels!=0 && planarconfig!=PLANARCONFIG_CONTIG && tif!=NULL);
}

~LibtiffImage::LibtiffImage()
{
	if (tif)
		TIFFClose(tif);
}
