#include "FileDataSource.h"
#include <fcntl.h>
#include "Logger.h"
#include <cstdio>
#include "config.h"

FileDataSource::FileDataSource(const char* filename, const uint32_t posoff, const uint32_t possize, std::string type) : filename(filename), posoff(posoff), possize(possize), type(type)
{
	data=0;
	size=0;
}

/*
 * Fonction retournant les données de la tuile
 * Le fichier ne doit etre lu qu une seule fois
 * Indique le taille de la tuile (inconnue a priori)
 */
const uint8_t* FileDataSource::getData(size_t &tile_size) {

	// Test : A ENLEVER
	size=80000;
	uint8_t* t=new uint8_t[80000];
	memset(t,0,80000);
	return t;

	if (data)
	{
		tile_size=size;
		return data;
	}
	// Ouverture du fichier
	int fildes = open(filename.c_str(), O_RDONLY);
	if(fildes < 0) {
		// LOGGER_ERROR("Impossible d'ouvrir le fichier " << filename);
		return 0;
	}
	// Lecture de la position de la tuile dans le fichier
	uint32_t pos;
	if(pread(fildes, &pos, sizeof(pos), posoff) < 0) {
		LOGGER_ERROR( "Erreur lors de la lecture de la position de la tuile dans le fichier " << filename);
		close(fildes);
		return 0;
	}
	// Lecture de la taille de la tuile dans le fichier
	// Ne lire que 4 octets (la taille de tile_size est plateforme-dependante)
	uint32_t tmp;
	if(pread(fildes, &tmp, sizeof(uint32_t), possize) < 0) {
		LOGGER_ERROR( "Erreur lors de la lecture de la taille de la tuile dans le fichier " << filename);
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
	size_t read_size=pread(fildes, data, tile_size, pos);
	if (read_size!=tile_size) {
		LOGGER_ERROR( "Impossible de lire la tuile dans le fichier " << filename );
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
