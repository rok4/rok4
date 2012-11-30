#include <cppunit/extensions/HelperMacros.h>

#include <string.h>
#include "TiffHeaderDataSource.h"
#include "TiffEncoder.h"
#include "RawImage.h"
#include "TiffHeader.h"

class CppUnitTiffHeaderDataSource : public CPPUNIT_NS::TestFixture {
        CPPUNIT_TEST_SUITE(CppUnitTiffHeaderDataSource);
        //CPPUNIT_TEST(rawHeaderConformity);
        CPPUNIT_TEST_SUITE_END();

public:
        void rawHeaderConformity();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CppUnitTiffHeaderDataSource);
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(CppUnitTiffHeaderDataSource, "CppUnitTiffHeaderDataSource");

void CppUnitTiffHeaderDataSource::rawHeaderConformity() {
    size_t pos = 142;
    uint8_t  TiffEncoderBuffer[pos];
    size_t  tiffHeaderSize;
    const uint8_t* tiffHeaderBuffer;
    RawImage* rawImage;
    TiffHeaderDataSource* fullTiffDS;


    int width = 256;
    int height = 256;
    int channels = 3;

    rawImage = new RawImage(width, height, channels, 0);
    DataStream* tiffStream = TiffEncoder::getTiffEncoder(rawImage, TIFF_RAW_INT8);
    tiffStream->read(TiffEncoderBuffer, pos);
    
    fullTiffDS = new TiffHeaderDataSource(0, TIFF_RAW_INT8, channels, width, height, width*height*channels);
    tiffHeaderBuffer = fullTiffDS->getData(tiffHeaderSize);
    CPPUNIT_ASSERT_MESSAGE("RAW INT8 RGB, Header Size incorrect", tiffHeaderSize == pos);
    pos--;
    for (; pos; pos--) {
        CPPUNIT_ASSERT_MESSAGE("RAW INT8 RGB,Header incorrect", TiffEncoderBuffer[pos] == *(tiffHeaderBuffer + pos));
    }

    pos = 142;

    memset(TiffEncoderBuffer, 0, pos);
    
    delete fullTiffDS;
    delete tiffStream;
    
    width = 256;
    height = 256;
    channels = 1;

    rawImage = new RawImage(width, height, channels, 0);
    tiffStream = TiffEncoder::getTiffEncoder(rawImage, TIFF_RAW_INT8);
    tiffStream->read(TiffEncoderBuffer, pos);

    fullTiffDS = new TiffHeaderDataSource(0, TIFF_RAW_INT8, channels, width, height, width*height*channels);
    tiffHeaderBuffer = fullTiffDS->getData(tiffHeaderSize);

    CPPUNIT_ASSERT_MESSAGE("RAW INT8 GRAY,Header Size incorrect", tiffHeaderSize == pos);
    pos--;
    for (; pos; pos--) {
        CPPUNIT_ASSERT_MESSAGE("RAW INT8 GRAY,Header incorrect", TiffEncoderBuffer[pos] == *(tiffHeaderBuffer + pos));
    }

    //pos = TiffHeader::headerSize;

}
