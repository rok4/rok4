#ifndef DECODER_H
#define DECODER_H

//#include <stdint.h>// pour uint8_t
//#include <cstddef> // pour size_t
//#include <string>

#include "Data.h"
#include "Image.h"
#include "Utils.h"


struct JpegDecoder {
	static const uint8_t* decode(DataSource* encData, size_t &size);
};

struct PngDecoder {
	static const uint8_t* decode(DataSource* encData, size_t &size);
};

struct InvalidDecoder {
	static const uint8_t* decode(DataSource* encData, size_t &size) {size = 0; return 0;}
};


/**
 *
 */
template <class Decoder>
class DataSourceDecoder : public DataSource {
	private:
		DataSource* encData;
		const uint8_t* decData;
		size_t decSize;

	public:
		DataSourceDecoder(DataSource* encData) : encData(encData), decData(0), decSize(0) {}

		~DataSourceDecoder() {
			delete decData;
			delete encData;
		}

		const uint8_t* getData(size_t &size) {
			LOGGER(DEBUG) << "==> "  << (intptr_t) encData << " " << (intptr_t) decData << " " << decSize << std::endl;
			if(!decData && encData) {
				decData = Decoder::decode(encData, decSize);
				if(!decData) {
					delete encData;
					encData = 0;
				}
			}
			size = decSize;
			LOGGER(DEBUG) << (intptr_t) encData << " " << (intptr_t) decData << " " << decSize << std::endl;
			return decData;
		}

		bool releaseData() {
			if(encData) encData->releaseData();
			decData = 0;
			delete decData;
		}

		std::string gettype() {return "image/bil";}
		int getHttpStatus() {return 200;}
};




class ImageDecoder : public Image {
	private: 
		DataSource* dataSource;

		int source_width;
		int source_height;
		int margin_top;
		int margin_left;
		const uint8_t* rawData;



		template<typename T> inline int getDataline(T* buffer, int line) {
			convert(buffer, rawData + ((margin_top + line) * source_width + margin_left) * channels, width * channels);

			//LOGGER(DEBUG) << line << std::endl;

			return width * channels;
		}

		template<typename T> inline int getNoDataline(T* buffer, int line) {
			//LOGGER(DEBUG) << "getnodata" << line << std::endl;
			memset(buffer, 0, width * channels * sizeof(T));
			//LOGGER(DEBUG) << "getnodata" << line << std::endl;
			return width * channels;
		}

		template<typename T> inline int _getline(T* buffer, int line) {

			//LOGGER(DEBUG) << "line " << line << " " << height << " " << width << " " << (intptr_t) rawData << " " << (intptr_t) dataSource << std::endl;
			if(rawData) { // Est ce que l'on a de la données.
				return getDataline(buffer, line);
				// TODO: libérer le datSource lorsque l'on lit la dernière ligne de l'image...
			}
			else if(dataSource) { // Non alors on essaye de la l'initialiser depuis dataSource
				size_t size;
				if(rawData = dataSource->getData(size))
					return getDataline(buffer, line);
				else {
					//LOGGER(DEBUG) << "rawdata= 0" << std::endl;
					delete dataSource;
					dataSource = 0;
				}
			}			
			return getNoDataline(buffer, line);
		}


	public:
		ImageDecoder(DataSource* dataSource, int source_width, int source_height, int channels,
				BoundingBox<double> bbox = BoundingBox<double>(0.,0.,0.,0.),
				int margin_left = 0, int margin_top = 0, int margin_right = 0, int margin_bottom = 0) :
			Image(source_width - margin_left - margin_right, source_height - margin_top - margin_bottom, channels, bbox),
			dataSource(dataSource),
			source_width(source_width),
			source_height(source_height),
			margin_top(margin_top),
			margin_left(margin_left),
			rawData(0) {}

		/* Implémentation de l'interface Image */
		inline int getline(uint8_t* buffer, int line) {return _getline(buffer, line);}
		inline int getline(float* buffer, int line)   {return _getline(buffer, line);}

		~ImageDecoder() {
			if(dataSource) {
				dataSource->releaseData();
				delete dataSource;
			}
		}


};


#endif


