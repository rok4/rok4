#include "format.h"

namespace format {
const char *eformat_name[] = {
	"UNKNOWN",
        "TIFF_RAW_INT8",
        "TIFF_JPG_INT8",
        "TIFF_PNG_INT8",
        "TIFF_LZW_INT8",
        "TIFF_RAW_FLOAT32",
        "TIFF_LZW_FLOAT32"
};

const int eformat_size = 6;

const char *eformat_mime[] = {
	"UNKNOWN",
	"image/tiff",
	"image/jpeg",
	"image/png",
	"image/tiff",
	"image/x-bil;bits=32",
	"image/tiff"
};



eformat_data fromString(std::string strFormat) {
	int i;
	for (i=eformat_size; i ; --i) {
		if (strFormat.compare(eformat_name[i])==0)
			break;
	}
	return static_cast<eformat_data>(i);
}

std::string toString(eformat_data format)
{
	return std::string(eformat_name[format]);
}

std::string toMimeType(eformat_data format)
{
	return std::string(eformat_mime[format]);
}



}