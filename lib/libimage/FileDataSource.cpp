#include "FileDataSource.h"
#include <fcntl.h>
#include "Logger.h"
#include <cstdio>
#include <errno.h>

// Taille maximum d'une tuile WMTS
#define MAX_TILE_SIZE 1048576

FileDataSource::FileDataSource(const char* filename, const uint32_t posoff, const uint32_t possize, std::string type) : filename(filename), posoff(posoff), possize(possize), type(type)
{
	data=0;
	size=0;
}

/*
 * Fonction retournant les données de la tuile
 * Le fichier ne doit etre lu qu une seule fois
 * Indique la taille de la tuile (inconnue a priori)
 */
const uint8_t* FileDataSource::getData(size_t &tile_size) {
	if (data)
	{
		tile_size=size;
		return data;
	}

	// Ouverture du fichier
	int fildes = open(filename.c_str(), O_RDONLY);
	if(fildes < 0) {
		LOGGER_DEBUG("Can't open file " << filename);
		return 0;
	}
	// Lecture de la position de la tuile dans le fichier
	uint32_t pos;
	size_t read_size;
	if(read_size=pread(fildes, &pos, sizeof(uint32_t), posoff) != 4) {
		LOGGER_ERROR("Erreur lors de la lecture de la position de la tuile dans le fichier " << filename);
		if (read_size<0)
			LOGGER_ERROR("Code erreur="<<errno);
		close(fildes);
		return 0;
	}
	// Lecture de la taille de la tuile dans le fichier
	// Ne lire que 4 octets (la taille de tile_size est plateforme-dependante)
	uint32_t tmp;
	if(read_size=pread(fildes, &tmp, sizeof(uint32_t), possize) != 4) {
		LOGGER_ERROR( "Erreur lors de la lecture de la taille de la tuile dans le fichier " << filename);
		if (read_size<0)
                        LOGGER_ERROR("Code erreur="<<errno);
		close(fildes);
		return 0;
	}
	tile_size=tmp;
	// La taille de la tuile ne doit pas exceder un seuil
	// Objectif : gerer le cas de fichiers TIFF non conformes aux specs du cache
	// (et qui pourraient indiquer des tailles de tuiles excessives)
	if(tile_size > MAX_TILE_SIZE)
	{
		LOGGER_ERROR( "Tuile trop volumineuse dans le fichier " << filename) ;
		close(fildes);
		return 0;
	}
	// Lecture de la tuile
	data = new uint8_t[tile_size];
	read_size=pread(fildes, data, tile_size, pos);
	if (read_size!=tile_size) {
		LOGGER_ERROR( "Impossible de lire la tuile dans le fichier " << filename );
		if (read_size<0)
                        LOGGER_ERROR("Code erreur="<<errno);
		delete[] data;
		close(fildes);
		return 0;
	}
	size=tile_size;
	close(fildes);
	return data;
}

/*
* Liberation du buffer
* @return true en cas de succes
*/
bool FileDataSource::releaseData()
{
        delete[] data;
        data = 0;
        return true;
}
