#include "PaletteDataSource.h"
//#include "PaletteConfig.h"
#include "byteswap.h"
#include "zlib.h"
#include <string.h>

//TODO modification du header : 25 | Colour type  passage de 0 à 2
//TODO ajout de la palette

//

/*
PaletteDataSource::PaletteDataSource(DataSource *dataSource,bool transparent,const uint8_t rgb[3]) : dataSource(dataSource), transparent(transparent){
	
	buildPLTE(PLTE,transparent,rgb) ;
	
	// On récupère le contenu du fichier 
	size_t tmp_size;
	const uint8_t* tmp = dataSource->getData(tmp_size);
	dataSize = tmp_size+(transparent?3*256+12+256+12:3*256+12);
	size_t pos = 33;
	// Taille en sortie = taille en entrée +  3*256+12 (PLTE) + 256+12 (tRNS) 
	data = new uint8_t[dataSize];
	// Copie de l'entete
	memcpy(data,tmp, pos);
	data[25] = 3; // mode palette
	//Mise à jour du crc du Header:
	uint32_t crch = crc32(0, Z_NULL, 0);
	crch = crc32(crch, data+8+4, 13+4);
	*((uint32_t*) (data+8+8+13)) = bswap_32(crch);
	memcpy(data+pos,PLTE,sizeof(PLTE));
	pos += sizeof(PLTE);
	if (transparent) {
		memcpy(data+pos,tRNS,sizeof(tRNS));
		pos += sizeof(tRNS);
	}
// 	//Copie des données
	memcpy(data+pos, tmp +33, tmp_size - 33);
	
}
*/
PaletteDataSource::PaletteDataSource(DataSource *dataSource,Palette* mpalette) : dataSource(dataSource){
	palette = mpalette;
	if (! mpalette) {
		this->palette = new Palette();
 	}
	if(palette->getPalettePNGSize()!=0){
		// On récupère le contenu du fichier 
		size_t tmp_size;
		const uint8_t* tmp = dataSource->getData(tmp_size);
		dataSize = tmp_size + palette->getPalettePNGSize();
		size_t pos = 33;
		// Taille en sortie = taille en entrée +  3*256+12 (PLTE) + 256+12 (tRNS) 
		data = new uint8_t[dataSize];
		// Copie de l'entete
		memcpy(data,tmp, pos);
		data[25] = 3; // mode palette
		//Mise à jour du crc du Header:
		uint32_t crch = crc32(0, Z_NULL, 0);
		crch = crc32(crch, data+8+4, 13+4);
		*((uint32_t*) (data+8+8+13)) = bswap_32(crch);
		//Copie de la palette
		memcpy(data+pos,palette->getPalettePNG(),palette->getPalettePNGSize());
		pos += palette->getPalettePNGSize();
		//Copie des données
		memcpy(data+pos, tmp +33, tmp_size - 33);
	}else {
		dataSize=0;
		data=NULL;
	}
	
}


const uint8_t* PaletteDataSource::getData(size_t& size)
{
	if(palette->getPalettePNGSize()!=0){
		size = dataSize;
		return data;
	}
	return dataSource->getData(size);
}

PaletteDataSource::~PaletteDataSource()
{
	dataSource->releaseData();
	delete dataSource;
	if(data)
		delete data;
}


