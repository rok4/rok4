#include "TiffHeaderDataSource.h"
#include <string.h>

#include "TiffHeader.h"

TiffHeaderDataSource::TiffHeaderDataSource(DataSource* dataSource,
        eformat_data format, int channel,
        int width, int height, size_t tileSize) :
        dataSource(dataSource),
        format(format), channel(channel),
        width(width), height(height) , tileSize(tileSize)
{
    size_t header_size = TiffHeader::headerSize(channel);
    const uint8_t* tmp;
    dataSize = header_size;
    if (dataSource) {
        tmp = dataSource->getData(tileSize);
        dataSize+= tileSize;
    }

    data = new uint8_t[dataSize];

    switch (format) {
    case TIFF_RAW_INT8:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_RAW_INT8_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_RAW_INT8_GRAY, header_size);
        }
        else if (channel == 3) {
            LOGGER_DEBUG("TIFF_HEADER_RAW_INT8_RGB");
            memcpy(data, TiffHeader::TIFF_HEADER_RAW_INT8_RGB, header_size);
        }
        else if (channel == 4) {
            LOGGER_DEBUG("TIFF_HEADER_RAW_INT8_RGBA");
            memcpy(data, TiffHeader::TIFF_HEADER_RAW_INT8_RGBA, header_size);
        }
        break;
    case TIFF_LZW_INT8:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_LZW_INT8_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_LZW_INT8_GRAY, header_size);
        }
        else if (channel == 3) {
            LOGGER_DEBUG("TIFF_HEADER_LZW_INT8_RGB");
            memcpy(data, TiffHeader::TIFF_HEADER_LZW_INT8_RGB, header_size);
        }
        else if (channel == 4) {
            LOGGER_DEBUG("TIFF_HEADER_LZW_INT8_RGBA");
            memcpy(data, TiffHeader::TIFF_HEADER_LZW_INT8_RGBA, header_size);
        }
        break;
    case TIFF_ZIP_INT8:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_ZIP_INT8_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_ZIP_INT8_GRAY, header_size);
        }
        else if (channel == 3) {
            LOGGER_DEBUG("TIFF_HEADER_ZIP_INT8_RGB");
            memcpy(data, TiffHeader::TIFF_HEADER_ZIP_INT8_RGB, header_size);
        }
        else if (channel == 4) {
            LOGGER_DEBUG("TIFF_HEADER_ZIP_INT8_RGBA");
            memcpy(data, TiffHeader::TIFF_HEADER_ZIP_INT8_RGBA, header_size);
        }
        break;
    case TIFF_PKB_INT8:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_PKB_INT8_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_PKB_INT8_GRAY, header_size);
        }
        else if (channel == 3) {
            LOGGER_DEBUG("TIFF_HEADER_PKB_INT8_RGB");
            memcpy(data, TiffHeader::TIFF_HEADER_PKB_INT8_RGB, header_size);
        }
        else if (channel == 4) {
            LOGGER_DEBUG("TIFF_HEADER_PKB_INT8_RGBA");
            memcpy(data, TiffHeader::TIFF_HEADER_PKB_INT8_RGBA, header_size);
        }
        break;
    
    case TIFF_RAW_FLOAT32:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_RAW_FLOAT32_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_RAW_FLOAT32_GRAY, header_size);
        }
        break;
    case TIFF_LZW_FLOAT32:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_LZW_FLOAT32_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_LZW_FLOAT32_GRAY, header_size);
        }
    case TIFF_ZIP_FLOAT32:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_ZIP_FLOAT32_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_ZIP_FLOAT32_GRAY, header_size);
        }
    case TIFF_PKB_FLOAT32:
        if (channel == 1) {
            LOGGER_DEBUG("TIFF_HEADER_PKB_FLOAT32_GRAY");
            memcpy(data, TiffHeader::TIFF_HEADER_PKB_FLOAT32_GRAY, header_size);
        }
        break;
    }
    *((uint32_t*)(data+18))  = width;
    *((uint32_t*)(data+30))  = height;
    *((uint32_t*)(data+102)) = height;
    *((uint32_t*)(data+114)) = tileSize;

    if (dataSource) {
        memcpy(data+header_size, tmp, tileSize);
    }
}




const uint8_t* TiffHeaderDataSource::getData(size_t& size)
{
    size = dataSize;
    return data;
}

TiffHeaderDataSource::~TiffHeaderDataSource()
{
    if (dataSource) {
        dataSource->releaseData();
        delete dataSource;
    }
    if (data)
        delete[] data;
}
