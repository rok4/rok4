#ifndef _FILEDATASOURCE_
#define _FILEDATASOURCE_

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
	FileDataSource(const char* filename, const uint32_t posoff, const uint32_t possize, std::string type);
	const uint8_t* get_data(size_t &tile_size);

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
