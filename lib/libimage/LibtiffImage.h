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
 * \file LibtiffImage.h
 ** \~french
 * \brief Définition des classes LibtiffImage et LibtiffImageFactory
 * \details
 * \li LibtiffImage : image physique, attaché à un fichier
 * \li LibtiffImageFactory : usine de création d'objet LibtiffImage
 ** \~english
 * \brief Define classes LibtiffImage and LibtiffImageFactory
 * \details
 * \li LibtiffImage : physical image, linked to a file
 * \li LibtiffImageFactory : factory to create LibtiffImage object
 */

#ifndef LIBTIFF_IMAGE_H
#define LIBTIFF_IMAGE_H

#include "Image.h"
#include "tiffio.h"
#include <string.h>

#define LIBTIFFIMAGE_MAX_FILENAME_LENGTH 512

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation d'une image physique
 * \details Une image physique est une vraie image dans ce sens où elle est rattachée à un fichier, que ce soit pour la lecture ou l'écriture de données au format TIFF. Seule cette classe ne va pas s'appuyer sur une autre image pour la lecture.
 *
 * Cette classe va utiliser la librairie TIFF afin de lire/écrire les données et de récupérer/fournir les informations sur les images.
 */
class LibtiffImage : public Image {

    friend class LibtiffImageFactory;

    private:
        /**
         * \~french \brief Image TIFF, servant d'interface entre le fichier et l'objet
         * \~english \brief TIFF image, used as interface between file and object
         */
        TIFF* tif;
        /**
         * \~french \brief Chemin du fichier image
         * \~english \brief Path to th image file
         */
        char* filename;
        /**
         * \~french \brief Nombre de bits par canal
         * \~english \brief Number of bits per sample
         */
        uint16_t bitspersample;
        /**
         * \~french \brief Photométrie des données (rgb, gray...)
         * \~english \brief Data photometric (rgb, gray...)
         */
        uint16_t photometric;
        /**
         * \~french \brief Compression des données (jpeg, packbits...)
         * \~english \brief Data compression (jpeg, packbits...)
         */
        uint16_t compression;
        /**
         * \~french \brief Format du canal, entier ou flottant
         * \~english \brief Sample format, integer or float
         */
        uint16_t sampleformat;
        /**
         * \~french \brief Taille de la bufferisation de l'image, en nombre de ligne
         * \~english \brief Image buffering size, in line number
         */
        uint16_t rowsperstrip;

        /**
         * \~french \brief Taille de la bufferisation de l'image, en nombre de canal
         * \~english \brief Image buffering size, in sample number
         */
        size_t strip_size;
        /**
         * \~french \brief Buffer de lecture, de taille strip_size
         * \~english \brief Read buffer, strip_size long
         */
        uint8_t* strip_buffer;
        /**
         * \~french \brief Buffer de lecture, de taille strip_size
         * \~english \brief Read buffer, strip_size long
         */
        uint16_t current_strip;

        template<typename T>
        int _getline(T* buffer, int line);

    protected:
        /** Constructeur */
        LibtiffImage( int width, int height, double resx, double resy, int channels, BoundingBox< double > bbox, TIFF* tif, char* name, int bitspersample, int sampleformat, int photometric, int compression, int rowsperstrip );

    public:

        /** D */
        int getline(uint8_t* buffer, int line);

        /** D */
        int getline(float* buffer, int line);

        void inline setfilename(char* str) {strcpy(filename,str);};
        inline char* getfilename() {return filename;}
        uint16_t inline getbitspersample() {return bitspersample;}
        uint16_t inline getphotometric() {return photometric;}
        uint16_t inline getcompression() {return compression;}
        uint16_t inline getrowsperstrip() {return rowsperstrip;}
        uint16_t inline getsampleformat() {return sampleformat;}
        
        /** Destructeur */
        ~LibtiffImage() {
            //std::cerr << "Delete LibtiffImage" << std::endl; /*TEST*/
            delete [] filename;
            delete [] strip_buffer;
            if (tif) TIFFClose(tif);
        }

        /** Fonction d'export des informations sur l'image (pour le débug) */
        void print() {
            LOGGER_INFO("");
            LOGGER_INFO("---------- LibTiffImage ------------");
            Image::print();
            LOGGER_INFO("\t- File name : " << filename);
            LOGGER_INFO("\t- Compression : " << compression);
            LOGGER_INFO("\t- Photometric : " << photometric);
            LOGGER_INFO("\t- Bits per sample : " << bitspersample);
            LOGGER_INFO("\t- Sample format : " << sampleformat);
            LOGGER_INFO("\t- Rows per strip : " << rowsperstrip);
            LOGGER_INFO("");
        }
};

class LibtiffImageFactory {
    public:
        LibtiffImage* createLibtiffImage(char* filename, BoundingBox<double> bbox, int calcWidth, int calcHeight, double resx, double resy);
        LibtiffImage* createLibtiffImage( char* filename, BoundingBox< double > bbox, int width, int height, double resx, double resy, int channels, uint16_t bitspersample, uint16_t sampleformat, uint16_t photometric, uint16_t compression, uint16_t rowsperstrip );
};


#endif

