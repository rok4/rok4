#ifndef PALETTEDATASOURCE_H
#define PALETTEDATASOURCE_H

#include "Data.h"
#include "Palette.h"

class PaletteDataSource : public DataSource
{
private:
	DataSource* dataSource;
	Palette* palette;
	//bool transparent;
	//uint8_t PLTE[3*256+12];
	size_t dataSize;
	uint8_t* data;
public:
	/** 
	 * Constructeur. 
	 * @param dataSource la source de l'image PNG
	 * @param palette une palette de couleur compatible PNG 
	 */
	//PaletteDataSource(DataSource* dataSource, bool transparent=false, const uint8_t rgb[3]=BLACK);
	PaletteDataSource(DataSource* dataSource, Palette* palette);
	
	inline bool releaseData()                   {return dataSource->releaseData();}
	inline std::string getType()                {return dataSource->getType();}
	inline int getHttpStatus()                  {return dataSource->getHttpStatus();}
	virtual const uint8_t* getData(size_t& size);
	virtual ~PaletteDataSource();
};

#endif // PALETTEDATASOURCE_H