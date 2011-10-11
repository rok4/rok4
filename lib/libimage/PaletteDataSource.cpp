#include "PaletteDataSource.h"
#include "byteswap.h"
#include "zlib.h"
#include <string.h>

//TODO modification du header : 25 | Colour type  passage de 0 à 2
//TODO ajout de la palette

//


PaletteDataSource::PaletteDataSource(DataSource *dataSource,bool transparent,const uint8_t rgb[3]) : dataSource(dataSource), transparent(transparent){
	
	// Remplissage de la palette
	PLTE[0] = 0;   PLTE[1] = 0;   PLTE[2] = 3;   PLTE[3] = 0;
	PLTE[4] = 'P'; PLTE[5] = 'L'; PLTE[6] = 'T'; PLTE[7] = 'E';
	if(transparent) {
		for(int i = 0; i < 256; i++) {
			memcpy(PLTE + 8 + 3*i, rgb, 3);
		}
	}
	else for(int i = 0; i < 256; i++) {
		PLTE[3*i+8]  = i + ((255 - i)*rgb[0] + 127) / 255;
		PLTE[3*i+9]  = i + ((255 - i)*rgb[1] + 127) / 255;
		PLTE[3*i+10] = i + ((255 - i)*rgb[2] + 127) / 255;
	}
	uint32_t crc = crc32(0, Z_NULL, 0);
	crc = crc32(crc, PLTE + 4, 3*256+4);
	*((uint32_t*) (PLTE + 256*3 + 8)) = bswap_32(crc);
	
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
	//Copie des données
	memcpy(data+pos, tmp +33, tmp_size - 33);
	
}

const uint8_t* PaletteDataSource::getData(size_t& size)
{
	size = dataSize;
	return data;
}

PaletteDataSource::~PaletteDataSource()
{
	dataSource->releaseData();
	delete dataSource;
	delete data;
}


