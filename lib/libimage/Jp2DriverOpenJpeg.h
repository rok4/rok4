#ifndef JP2DRIVEROPENJPEG_H
#define JP2DRIVEROPENJPEG_H

#endif // JP2DRIVEROPENJPEG_H

#include <string>

#include "BoundingBox.h"
#include "Libjp2Image.h"

class Jp2DriverOpenJpeg {

    private:

        std::string m_cFilename;

        double m_dResx;
        double m_dResy;

        double m_xmin, m_ymin, m_xmax, m_ymax;

    public:

        // constructeur
        Jp2DriverOpenJpeg();
        Jp2DriverOpenJpeg(char* filename, BoundingBox<double> bbox, double resx, double resy);

        // desctructeur
        ~Jp2DriverOpenJpeg();

        // methodes
        Libjp2Image * createLibjp2ImageToRead();

       /**
        * \~french
        * \brief caracteristique de l'image
        */
        void information();

        /**
         * \~french
         * \brief callback
         */
//        void info_callback(const char *msg, void *client_data) {
//            (void)client_data;
//            LOGGER_INFO(msg);
//        }
//        void warning_callback(const char *msg, void *client_data) {
//            (void)client_data;
//            LOGGER_WARN(msg);
//        }
//        void error_callback(const char *msg, void *client_data) {
//            (void)client_data;
//            LOGGER_ERROR(msg);
//        }

};

