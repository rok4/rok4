#ifndef TILE_H
#define TILE_H
/*
#include <iostream>
#include "Logger.h"
#include "Utils.h"
#include "Image.h"
#include "Data.h"


typedef enum {
	RAW_UINT8,
	RAW_FLOAT,
	JPEG_UINT8,
	PNG_UINT8
} TILE_CODING;


class RawDecoder {
public:
	static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize) {
		memcpy(raw_data, encoded_data, encoded_size);
	}
};

class JpegDecoder {
public:
	static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize);
};

class PngDecoder {
public:
	static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data, int height, int linesize);
};


*/

/** Classe Abstraite regroupant les classes abstraites Image et DataSource **/

/*

class Tile : public Image, public DataSource {
	public:
  Tile(int width, int height, int channels, BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.)) : Image(width, height, channels, bbox) {}
	virtual ~Tile() {};
};
*/


/**
 * Tuile fabriquée à partir de sa version encodée (PNG, JPEG, RAW) fournie dans un dataSource.
 * Si la source est invalide la tuile deviendra une tuile nodata fournie.
 */
/*
template<class Decoder>
class EncodedTile : public Tile {
private:

	// Status pour déterminer quelle source de données il faut utiliser.
	// UNKNOWN	(valeur initiale) la validité de encodedSource n'est pas encore connue. 
	// ENCODED encodedSource est valide et non décodé
	// DECODED encodedSource est valide et décodé
	// NODATA  encodedSource est invalide => utiliser noDataSource
	enum {UNKNOWN, ENCODED, DECODED, NODATA} status;


	// Source de donnees : correspond aux octets de la tuile dans le fichier du cache
	// Si pointeur = 0 ou si encodedSource->getData() == 0 alors noDataSource est utlisé
	// Cette source est spécifique à la tuile est le peut être utilisées qu'une seule fois.
	DataSource* encodedSource;

	// Source de donnees nodata
	// Cette source est typiquement fournie par le Level et a vocation à être partagée avec d'autres Tiles.
	Tile& noDataTile;

	// Donnees raw : correspond a la tuile decompressee (permet un acces direct aux pixels)
	// 0 si les données ne sont pas encore décompressées. La décompression se fait à la demande
	uint8_t* rawData;

	int tile_width;
	int tile_height;

	int left;
	int top;
*/
	/* retourne la source de données à utiliser en fonction du status */
/*
	inline DataSource& getDataSource() {
		switch(status) {
			case UNKNOWN: 
				initStatus();
				if(status == NODATA) return noDataTile;
			case ENCODED:
			case DECODED:
							return *encodedSource;
			case NODATA: 
							return noDataTile;
		}
	}
*/
  
	/**
	 * Initialise le status en cas de status inconnu 
	 * En sortie, status est soit ENCODED soit NODATA
	 */
/*
	void initStatus() {
		if(status == UNKNOWN) {
			if(encodedSource) {
				size_t size;
				if(encodedSource->getData(size)) {
					status = ENCODED;
				} else {
					delete encodedSource;
					encodedSource = 0;
					status = NODATA;
				}
			} else {
				status = NODATA;
			}
		}
	}

	bool decode() {
		if(status == ENCODED) {
			size_t size;			
			rawData = new uint8_t[tile_width * tile_height * channels];		
			const uint8_t* encodedData = getData(size);
//			if( TODO prendre en compte les cas d'erreur de décodage...
					Decoder::decode(encodedData, size, rawData, tile_height, tile_width*channels);
//					)	{
				status = DECODED;*/
/*			} else {
				status = NODATA;
				delete[] rawData;
				rawData = 0;
			} */
/*		}
	}


	template<typename T> 
		inline int _getline(T* buffer, int line) {
			switch(status) {
				case UNKNOWN: 
					initStatus();
					if(status == NODATA) return noDataTile.getline(buffer, line);
				case ENCODED: 
					decode();
					if(status == NODATA) return noDataTile.getline(buffer, line);
				case DECODED:
					convert(buffer, rawData + ((top + line) * tile_width + left) * channels, width * channels);
					return width * channels;
				case NODATA:
					return noDataTile.getline(buffer, line);					
			}
		}

public:

	
	EncodedTile(int tile_width, int tile_height, int channels, DataSource* encodedSource, Tile& noDataTile, int left, int top, int right, int bottom) :
		Tile(tile_width - left - right, tile_height - top - bottom, channels),
		encodedSource(encodedSource),
		noDataTile(noDataTile),
		tile_width(tile_width),
		tile_height(tile_height),
		left(left),
		top(top),
		rawData(0),
		status(UNKNOWN) {}

	~EncodedTile() {
		delete encodedSource;
		delete[] rawData;
	}
*/
	/* Implémentation de l'interface Image 
	inline int getline(uint8_t* buffer, int line) {return _getline(buffer, line);}
	inline int getline(float* buffer, int line)   {return _getline(buffer, line);}

	Implémentation de l'interface DataSource 
	inline const uint8_t* getData(size_t &size) {return getDataSource().getData(size);}
	inline bool releaseData()                   {return getDataSource().releaseData();}	// TODO: il faut peut etre pas releaser le noDataTile.
	inline std::string gettype()                {return getDataSource().gettype();}
	inline int getHttpStatus()                  {return getDataSource().getHttpStatus();}

};



#include <vector>

template<typename data_t, class Encoder>
class MonochromaticTile : public Tile {
	private:
		std::vector<data_t> color;
		BufferedDataSource *dataSource;

  public:
	MonochromaticTile(int width, int height, int channels, std::vector<data_t> color) :
		Tile(width, height, channels),
		color(color) {
			Encoder encoder(new ImageCopy(*this));
			dataSource = new BufferedDataSource(encoder);
		}
  ~MonochromaticTile() {delete dataSource;}

	 Implémentation de l'interface Image 
	inline int getline(uint8_t* buffer, int line) {
		for(int i = 0; i < width; i++) 		
			convert(buffer + channels*i, &color[0], channels);
		return channels*width;
	}
	inline int getline(float* buffer, int line) {
		for(int i = 0; i < width; i++) 
			convert(buffer + channels*i, &color[0], channels);
		return channels*width;
	}

	* Implémentation de l'interface DataSource *
	inline const uint8_t* getData(size_t &size) {return dataSource->getData(size);}
	inline bool releaseData()                   {return dataSource->releaseData();}
	inline std::string gettype()                {return dataSource->gettype();}
	inline int getHttpStatus()                  {return dataSource->getHttpStatus();}

};

/*
#include "PNGEncoder.h"
MonochromaticTile<uint8_t, PNGEncoder> bubu(10, 10, 3, std::vector<uint8_t>(0,3));

*/
#endif


