#ifndef JP2DRIVERJASPER_H
#define JP2DRIVERJASPER_H

#endif // JP2DRIVERJASPER_H

#include "Logger.h"
#include "BoundingBox.h"
#include "Libjp2Image.h"

class Jp2DriverJasper {

    private:

        char* m_cFilename;

        double m_dResx;
        double m_dResy;

        double m_xmin, m_ymin, m_xmax, m_ymax;

    public:

        // constructeur
        Jp2DriverJasper();
        Jp2DriverJasper(char* filename, BoundingBox<double> bbox, double resx, double resy);

        // desctructeur
        ~Jp2DriverJasper();

        // methodes
        Libjp2Image * createLibjp2ImageToRead() {
			LOGGER_ERROR("Not yet implemented !");
			return NULL;
		}
};

