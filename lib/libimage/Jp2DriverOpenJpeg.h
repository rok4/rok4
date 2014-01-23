#ifndef JP2DRIVEROPENJPEG_H
#define JP2DRIVEROPENJPEG_H

#endif // JP2DRIVEROPENJPEG_H

#include "BoundingBox.h"
#include "Libjp2Image.h"

class Jp2DriverOpenJpeg {

    public:

        /**
         * @brief createLibjp2ImageToRead
         * @param filename
         * @param bbox
         * @param resx
         * @param resy
         * @return
        */
        Libjp2Image * createLibjp2ImageToRead(char* filename, BoundingBox<double> bbox, double resx, double resy);

        /**
         * \~french
         * \brief callback
         */
// FIXME : how does it work ?
//
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

