#ifndef TILE_H
#define TILE_H

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
}       TILE_CODING;


class RawDecoder {
public:
	static void decode(const uint8_t* encoded_data, size_t encoded_size, uint8_t* raw_data) {
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

class Tile : public Image {
private:
	// Source de donnees : correspond aux octets de la tuile dans le fichier du cache
	DataSource* datasource;
	// Source de donnees nodata
	DataSource* noDataSource;
	// Donnees raw : correspond a la tuile decompressee (permet un acces direct aux pixels)
	uint8_t* raw_data;

	int tile_width;
	int tile_height;

	int left;
	int top;
	int coding;

public:
	inline void setDataSource(DataSource* source) {datasource=source;}
	inline DataSource* getDataSource() {return datasource;}
	inline DataSource* getNoDataSource() {return noDataSource;}
	int getline(uint8_t* buffer, int line);
	int getline(float* buffer, int line);
	inline int getTileWidth() {return tile_width;}
	inline int getTileHeight() {return tile_height;}

	Tile(int tile_width, int tile_height, int channels, DataSource* datasource, DataSource* noDataSource, int left, int top, int right, int bottom, int coding);

	~Tile() {
		delete[] raw_data;
		getDataSource()->release_data();
	}
};


#endif
