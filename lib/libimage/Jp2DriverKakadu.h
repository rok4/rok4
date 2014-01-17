#ifndef JP2DRIVERKAKADU_H
#define JP2DRIVERKAKADU_H

#endif // JP2DRIVERKAKADU_H

#include "Logger.h"
#include "BoundingBox.h"
#include "Libjp2Image.h"

class Jp2DriverKakadu {

    private:

        char* m_cFilename;

        double m_dResx;
        double m_dResy;

        double m_xmin, m_ymin, m_xmax, m_ymax;

    public:

        // constructeur
        Jp2DriverKakadu();
        Jp2DriverKakadu(char* filename, BoundingBox<double> bbox, double resx, double resy);

        // desctructeur
        ~Jp2DriverKakadu();

        // methodes
        Libjp2Image * createLibjp2ImageToRead() {
			LOGGER_ERROR("Not yet implemented !");
			return NULL;
		}
};

