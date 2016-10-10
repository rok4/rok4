#ifndef CONVERTEDCHANNELSIMAGE_H
#define CONVERTEDCHANNELSIMAGE_H

#include "Image.h"

class ConvertedChannelsImage: public Image {

private:
    Image* sourceImage;
    int channelsOut;
    int channelsIn;
    int alphaOut;

public:
    ConvertedChannelsImage(int channels, Image* sourceImage);
    ConvertedChannelsImage(int channels, Image* sourceImage, int alpha);
    ~ConvertedChannelsImage();
    int getline ( uint8_t *buffer, int line );
    int getline ( uint16_t *buffer, int line );
    int getline ( float *buffer, int line );
    int greyscale(int r, int g, int b);
    float greyscaleFloat(int r, int g, int b);

};

#endif // CONVERTEDCHANNELSIMAGE_H
