#ifndef JP2DRIVERKAKADU_H
#define JP2DRIVERKAKADU_H

#endif // JP2DRIVERKAKADU_H

#include "Logger.h"
#include "BoundingBox.h"
#include "Libjp2Image.h"

class Jp2DriverKakadu {

    public:

        Libjp2Image * createLibjp2ImageToRead(char* filename, BoundingBox<double> bbox, double resx, double resy);
};

