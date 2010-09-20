#ifndef _FILEDATASOURCE_
#define _FILEDATASOURCE_

#include <fcntl.h>
#include "Logger.h"
#include <cstdio>
#include "config.h"
#include "Data.h"

/*
* Classe qui lit les tuiles d'un fichier tuilé.
*/

class FileDataSource : public DataSource {
	private:
	std::string filename;
	const uint32_t posoff;		// Position dans le fichier des 4 octets indiquant la position de la tuile dans le fichier
	const uint32_t possize;		// Position dans le fichier des 4 octets indiquant la taille de la tuile dans le fichier
	uint8_t* data;
	size_t size;
	std::string type;
	public:
	FileDataSource(const char* filename, const uint32_t posoff, const uint32_t possize, std::string type) : filename(filename), posoff(posoff), possize(possize), type(type)
	{
		data=0;
		size=0;
	}

	/*
	* Fonction retournant les données de la tuile
	* Le fichier ne doit etre lu qu une seule fois
	* Indique le taille de la tuile (inconnue a priori)
	*/
    	const uint8_t* get_data(size_t &tile_size) {
      		if (data)
		{
			tile_size=size;
			return data;
		}

		// Ouverture du fichier
      		int fildes = open(filename.c_str(), O_RDONLY);
      		if(fildes < 0) {
        		LOGGER_ERROR("Impossible d'ouvrir le fichier " << filename);
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
      		if(pread(fildes, &tile_size, sizeof(tile_size), possize) < 0) {
        		LOGGER_ERROR( "Erreur lors de la lecture de la taille de la tuile dans le fichier " << filename);
        		close(fildes);
        	return 0;
      		}
		// La taille de la tuile ne doit pas exceder un seuil
		// Objectif : gerer le cas de fichiers TIFF non conformes aux specs du cache
		// (et qui pourraient indiquer des tailles de tuiles excessives)
      		if(tile_size > MAX_TILE_SIZE)
		{
			LOGGER_ERROR( "Tuile trop volumineuse dans le fichier" << filename) ;
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
	* @ return le type MIME de la source de donnees
	*/
	std::string gettype()
	{
		return type;
	}
	
	/*
	* Liberation du buffer
	* @return true en cas de succes
	*/
	bool release_data()
	{
		if (data)
			delete[] data;
		return true;
	}
	
};

#endif
