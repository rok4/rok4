/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <geop_services@geoportail.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file checkImage.cpp
 * \author Institut national de l'information géographique et forestière
 * \~french \brief Controle d'une image avec la LibImage
 * \~french \details Orienté maintenance !
 */

#include <stdio.h>
#include <iostream>
#include <stdint.h>

#include "FileImage.h"
#include "Logger.h"
#include "../be4version.h"

int main ( int argc, char **argv ) {

        // Initialisation des Loggers
        Logger::setOutput ( STANDARD_OUTPUT_STREAM_FOR_ERRORS );

        Accumulator* acc = new StreamAccumulator();

        Logger::setAccumulator ( INFO , acc );
        Logger::setAccumulator ( WARN , acc );
        Logger::setAccumulator ( ERROR, acc );
        Logger::setAccumulator ( FATAL, acc );
        Logger::setAccumulator ( DEBUG, acc );

        std::ostream &logw = LOGGER ( DEBUG );
        logw.precision ( 16 );
        logw.setf ( std::ios::fixed,std::ios::floatfield );

        // Version
        LOGGER_INFO("Version " << BE4_VERSION);

        char *cFilenameIn  = NULL;
        char *cFilenameOut = NULL;

        // Argument
        if ( argc == 1 ) {
            LOGGER_INFO("Test image decoding : PNG/JP2/TIFF to TIFF (use libImage).");
            LOGGER_INFO("usage : ./checkImage input.(png|jp2|tif) [output.tif]");
            return ( 0 );
        }

        else if ( argc == 2 || argc == 3) {
            cFilenameIn  = new char[4096];
            strcpy(cFilenameIn, argv[1]);
            LOGGER_INFO("File name in  : " << cFilenameIn);

            if ( argc == 3 ) {
                cFilenameOut = new char[4096];
                strcpy(cFilenameOut, argv[2]);
                LOGGER_INFO("File name out : " << cFilenameOut);
            }
        }

        else {
            LOGGER_ERROR("Too many parameters !");
            return ( 1 );
        }

        // Test sur le fichier
        FILE *pFile;
        pFile = fopen(cFilenameIn, "r");
        if (! pFile) {
            LOGGER_ERROR("Can not open file !");
            return ( 3 );
        }

        // Taille du fichier
        fseek(pFile, 0, SEEK_END);
        int file_length = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);
        unsigned char *src = (unsigned char *) malloc(file_length);

        if (fread(src, 1, file_length, pFile) != (size_t) file_length) {
            free(src);
            fclose(pFile);
            LOGGER_ERROR("fread return a number of element different from the expected.");
            return ( 4 );
        }

        fclose(pFile);

        LOGGER_INFO("File size : " << file_length);

        // Decodeur de l'image JP2/PNG/TIFF
        FileImageFactory factory;
        FileImage* pImageIn  = factory.createImageToRead(cFilenameIn, BoundingBox<double>(0,0,0,0), -1, -1);

        if (! pImageIn) {
            LOGGER_ERROR("Can not read data with this driver !");
            return ( 5 );
        }

        LOGGER_DEBUG("GET Information INPUT ...");
        pImageIn->print();

        // Encodeur de l'image en TIFF
        if (cFilenameOut != NULL) {

            FileImage* pImageOut = factory.createImageToWrite(cFilenameOut,
                                                              pImageIn->getBbox(),
                                                              pImageIn->getResX(),
                                                              pImageIn->getResY(),
                                                              pImageIn->getWidth(),
                                                              pImageIn->getHeight(),
                                                              pImageIn->channels,
                                                              SampleFormat::UINT,
                                                              pImageIn->getBitsPerSample(),
                                                              Photometric::RGB,
                                                              Compression::NONE);

            if (! pImageOut) {
                LOGGER_ERROR("Can not write data with this driver !");
                return ( 6 );
            }

            if (pImageOut->writeImage(pImageIn) < 0) {
                LOGGER_ERROR("Can not write image !");
                return ( 7 );
            }

            LOGGER_DEBUG("GET Information OUTPUT ...");
            pImageOut->print();

            // clean
            delete pImageOut;
        }

        LOGGER_INFO("All seems ok !");

        // clean
        delete pImageIn;

        return 0;
}
