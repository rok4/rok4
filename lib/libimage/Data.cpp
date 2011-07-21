#include "Data.h"
#include <cstring> // pour memcpy
#include "Logger.h"

/** 
 * Constructeur. 
 * Le paramètre dataStream est complètement lu. Il est donc inutilisable par la suite.
 */

// TODO : peut être optimisé, à mettre au propre
BufferedDataSource::BufferedDataSource(DataStream& dataStream) : type(dataStream.getType()), httpStatus(dataStream.getHttpStatus()), dataSize(0) {
	// On initialise data à une taille arbitraire de 32Ko.
	size_t maxSize = 32768;
	data = new uint8_t[maxSize];

	while(!dataStream.eof()) { // On lit le DataStream jusqu'au bout
		size_t size = dataStream.read(data + dataSize, maxSize - dataSize);
		dataSize += size;
		if(size == 0 || dataSize == maxSize) { // On alloue 2 fois plus de place si on en manque.
			maxSize *= 2;
			uint8_t* tmp = new uint8_t[maxSize];
			memcpy(tmp, data, dataSize);
			delete[] data;
			data = tmp;
		}
	}

	// On réalloue exactement la taille nécessaire pour ne pas perdre de place
	uint8_t* tmp = new uint8_t[dataSize];
	memcpy(tmp, data, dataSize);
	delete[] data;
	data = tmp;
}

