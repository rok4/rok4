#ifndef FORMAT_H
#define FORMAT_H
#include <string>
#include <string.h>

//To declare a new format change the implementation too

enum eformat_data {
	UNKNOWN = 0,
	TIFF_RAW_INT8 = 1,
	TIFF_JPG_INT8 = 2,
	TIFF_PNG_INT8 = 3,
	TIFF_LZW_INT8 = 4,
	TIFF_RAW_FLOAT32 = 5,
	TIFF_LZW_FLOAT32 = 6
};

namespace format {
	eformat_data fromString(std::string strFormat);
	std::string toString(eformat_data format);
	std::string toMimeType(eformat_data format);
}
#endif //FORMAT_H