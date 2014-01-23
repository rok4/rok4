#ifndef JP2DRIVERJASPER_H
#define JP2DRIVERJASPER_H

#endif // JP2DRIVERJASPER_H

#include "Logger.h"
#include "BoundingBox.h"
#include "Libjp2Image.h"

class Jp2DriverJasper {

    public:
        Libjp2Image * createLibjp2ImageToRead(char* filename, BoundingBox<double> bbox, double resx, double resy);
};
