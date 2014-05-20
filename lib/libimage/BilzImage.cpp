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
 * \file BilzImage.cpp
 ** \~french
 * \brief Implémentation des classes BilzImage et BilzImageFactory
 * \details
 * \li BilzImage : gestion d'une image au format PNG, en lecture
 * \li BilzImageFactory : usine de création d'objet BilzImage
 ** \~english
 * \brief Implement classes BilzImage and BilzImageFactory
 * \details
 * \li BilzImage : manage a (Z)BIL format image, reading
 * \li BilzImageFactory : factory to create BilzImage object
 */

#include "BilzImage.h"
#include "Logger.h"
#include "Utils.h"
#include "zlib.h"

/* ------------------------------------------------------------------------------------------------ */
/* --------------------------------------- LECTEUR DE HEADER -------------------------------------- */

/**
 * \~french
 * \brief Extrait les informations du header associé à l'image (Z)BIL
 * \details Le chemin fourni est celui de l'image. Un fichier dont seule l'extension diffère (hdr ou HDR) est cherché (erreur si aucun des deux n'existe). Ce dernier est lu (simple fichier texte) et les lignes suivantes sont interprétées :
 * \li NCOLS    <INTEGER>
 * \li NROWS    <INTEGER>
 * \li NBITS    <INTEGER>
 * \li NBANDS   <INTEGER>
 * \param[in] imagefilename image (Z)BIL dont le header doit être lu
 * \param[out] width largeur de l'image (Z)BIL
 * \param[out] height hauteur de l'image (Z)BIL
 * \param[out] samplesperpixel nombre de canaux de l'image (Z)BIL
 * \param[out] bitspersample taille en bit d'un canal de l'image (Z)BIL
 * \return TRUE en cas de succes, FALSE sinon
 */
bool readHeaderFile(char* imagefilename, int* width, int* height, int* samplesperpixel, int* bitspersample) {
    
    char * pch;
    pch = strrchr ( imagefilename,'.' );
    int basenamelength = pch - imagefilename;
    
    char headerfilename[IMAGE_MAX_FILENAME_LENGTH];
    memset(headerfilename, 0, IMAGE_MAX_FILENAME_LENGTH);
    memmove(headerfilename, imagefilename, basenamelength);
    
    memcpy(headerfilename+basenamelength, ".hdr", 4);
    
    std::ifstream header;

    // Tentative d'ouverture du fichier header, avec l'extension en minuscule ou en majuscule
    header.open ( headerfilename );
    if ( !header ) {
        memcpy(headerfilename+basenamelength, ".HDR", 4);
        header.open ( headerfilename );
        if ( !header ) {
            LOGGER_ERROR ( "No found header, with extension .hdr or .HDR" );
            return NULL;
        }
    }
    
    std::string str;
    char key[20];
    int value = 0;
    while (! header.eof() ) {
        std::getline ( header,str );
        
        if ( std::sscanf ( str.c_str(),"%s %d", key, &value) != 2 ) continue;
        
        if (strncmp(key, "NCOLS", 5) == 0 || strncmp(key, "ncols", 5) == 0) *width = value;
        if (strncmp(key, "NROWS", 5) == 0 || strncmp(key, "nrows", 5) == 0) *height = value;
        if (strncmp(key, "NBITS", 5) == 0 || strncmp(key, "nbits", 5) == 0) *bitspersample = value;
        if (strncmp(key, "NBANDS", 6) == 0 || strncmp(key, "nbands", 5) == 0) *samplesperpixel = value;
    }
}


/* ------------------------------------------------------------------------------------------------ */
/* --------------------------------------- DÉCOMPRESSEUR ZIP -------------------------------------- */

/**
 * \~french
 * \brief Décompresse les données
 * \details La compression des données sources fournies doit forcément être le deflate (zip). Le buffer des données décompressées doit déjà être alloué.
 * \param[in] compresseddata pointeur vers les données à décompresser
 * \param[in] compresseddatasize taille des données à décompresser
 * \param[out] rawdata pointeur vers les données décompressées
 * \param[in] rawdatasize taille théorique des données décompressées
 * \return TRUE en cas de succes, FALSE sinon
 */
bool uncompressedData(uint8_t* compresseddata, int compresseddatasize, uint8_t* rawdata, int rawdatasize) {
    
    // Initialisation du flux
    z_stream zstream;
    zstream.zalloc = Z_NULL;
    zstream.zfree = Z_NULL;
    zstream.opaque = Z_NULL;
    zstream.data_type = Z_BINARY;
    int zinit;
    if ( ( zinit=inflateInit ( &zstream ) ) != Z_OK ) {
        if ( zinit==Z_MEM_ERROR ) LOGGER_ERROR ( "Decompression DEFLATE : pas assez de memoire" );
        else if ( zinit==Z_VERSION_ERROR ) LOGGER_ERROR ( "Decompression DEFLATE : versions de zlib incompatibles" );
        else if ( zinit==Z_STREAM_ERROR ) LOGGER_ERROR ( "Decompression DEFLATE : parametres invalides" );
        else LOGGER_ERROR ( "Decompression DEFLATE : echec" );
        return false;
    }

    zstream.next_in = compresseddata;
    zstream.avail_in = compresseddatasize;
    zstream.next_out = rawdata;
    zstream.avail_out = rawdatasize;
    // Decompression du flux
    while ( zstream.avail_in != 0 ) {
        if ( int err = inflate ( &zstream, Z_SYNC_FLUSH ) ) {
            if ( err == Z_STREAM_END && zstream.avail_in == 0 ) break; // fin du fichier OK.
            if ( zstream.avail_out == 0 ) {
                LOGGER_ERROR ( "Decompression DEFLATE : le buffer de sortie ne devrait pas être trop petit");
                return false;
            }
            LOGGER_ERROR ( "Decompression DEFLATE : probleme deflate decompression " << err );
            return false;
        }
    }

    // Destruction du flux
    if ( inflateEnd ( &zstream ) != Z_OK ) {
        LOGGER_ERROR ( "Decompression DEFLATE : probleme de liberation du flux" );
        return false;
    }

    if (zstream.avail_out != 0) {
        LOGGER_ERROR ( "Decompression DEFLATE : la taille des données décompressée n'est pas celle attendue" );
        LOGGER_ERROR ( "Il manque " << zstream.avail_out << "octets" );
        return false;
    }

    return true;
}


/* ------------------------------------------------------------------------------------------------ */
/* -------------------------------------------- USINES -------------------------------------------- */

/* ----- Pour la lecture ----- */
BilzImage* BilzImageFactory::createBilzImageToRead ( char* filename, BoundingBox< double > bbox, double resx, double resy ) {
        
    
    /************** RECUPERATION DES INFORMATIONS **************/

    int width = 0, height = 0, channels = 0, bitspersample = 0;
    SampleFormat::eSampleFormat sf = SampleFormat::UNKNOWN;
    Photometric::ePhotometric ph = Photometric::UNKNOWN;
    Compression::eCompression comp = Compression::NONE;
    
    if (! readHeaderFile(filename, &width, &height, &channels, &bitspersample)) {
        LOGGER_ERROR ( "Cannot read header associated to (Z)Bil image " << filename );
        return NULL;        
    }
    
    if (width == 0 || height == 0 || channels == 0 || bitspersample == 0) {
        LOGGER_ERROR ( "Missing or invalid information in the header associated to (Z)Bil image " << filename );
        return NULL;         
    }
    
    switch (bitspersample) {
        case 32 :
            sf = SampleFormat::FLOAT;
            break;
        case 8 :
            sf = SampleFormat::UINT;
            break;
    }
    
    switch (channels) {
        case 1 :
        case 2 :
            ph = Photometric::GRAY;
            break;
        case 3 :
        case 4 :
            ph = Photometric::RGB;
            break;
        default :
            LOGGER_ERROR ( "Unhandled number of samples pixel (" << channels << ") for image " << filename );
            return NULL;
    }
    
    /********************** CONTROLES **************************/
    
    if ( ! BilzImage::canRead ( bitspersample, sf ) ) {
        LOGGER_ERROR ( "Not supported sample type : " << SampleFormat::toString ( sf ) << " and " << bitspersample << " bits per sample" );
        LOGGER_ERROR ( "\t for the image to read : " << filename );
        return NULL;
    }

    if ( resx > 0 && resy > 0 ) {
        if (! Image::dimensionsAreConsistent(resx, resy, width, height, bbox)) {
            LOGGER_ERROR ( "Resolutions, bounding box and real dimensions for image '" << filename << "' are not consistent" );
            return NULL;
        }
    } else {
        bbox = BoundingBox<double> ( 0, 0, ( double ) width, ( double ) height );
        resx = 1.;
        resy = 1.;
    }

    /************** LECTURE DE L'IMAGE EN ENTIER ***************/
    
    FILE *file = fopen ( filename, "rb" );
    if ( !file ) {
        LOGGER_ERROR ( "Unable to open the file (to read) " << filename );
        return NULL;
    }
    
    int rawdatasize = width * height * channels * bitspersample / 8;
    uint8_t* bilData = new uint8_t[rawdatasize];
    
    size_t readdatasize = fread ( bilData, 1, rawdatasize, file );

    if (readdatasize < rawdatasize ) {
        LOGGER_DEBUG("Data in bil image are compressed (smaller than expected). We try to uncompressed them (deflate).");
        comp = Compression::DEFLATE;
        
        uint8_t* tmpData = new uint8_t[readdatasize];
        memcpy(tmpData, bilData, readdatasize);
        
        if (! uncompressedData(tmpData, readdatasize, bilData, rawdatasize)) {
            LOGGER_ERROR ( "Cannot uncompressed data for file " << filename );
            LOGGER_ERROR ( "Only deflate compression is supported");
            return NULL;
        }
        
        delete [] tmpData;
    }
    
    /******************** CRÉATION DE L'OBJET ******************/
    
    return new BilzImage (
        width, height, resx, resy, channels, bbox, filename,
        sf, bitspersample, ph, comp,
        bilData
    );
    
}


/* ------------------------------------------------------------------------------------------------ */
/* ----------------------------------------- CONSTRUCTEUR ----------------------------------------- */

BilzImage::BilzImage (
    int width,int height, double resx, double resy, int channels, BoundingBox<double> bbox, char* name,
    SampleFormat::eSampleFormat sampleformat, int bitspersample, Photometric::ePhotometric photometric, Compression::eCompression compression,
    uint8_t* bilData ) :

    FileImage ( width, height, resx, resy, channels, bbox, name, sampleformat, bitspersample, photometric, compression, false ),

    data(bilData) {
        
}

/* ------------------------------------------------------------------------------------------------ */
/* ------------------------------------------- LECTURE -------------------------------------------- */

int BilzImage::getline ( uint8_t* buffer, int line ) {
    // Quel que soit le format des canaux de l'image source, on stocke toujours sur des entiers sur 8 bits
    memcpy(buffer, data + line*width*pixelSize, width*pixelSize);
    return width*pixelSize;
}

int BilzImage::getline ( float* buffer, int line ) {
    
    if ( bitspersample == 8 && sampleformat == SampleFormat::UINT ) {
        // On veut la ligne en flottant pour un réechantillonnage par exemple mais l'image lue est sur des entiers
        uint8_t* buffer_t = new uint8_t[width*channels];
        memcpy(buffer_t, data + line*width*pixelSize, width*pixelSize);

        convert ( buffer,buffer_t,width*channels );
        delete [] buffer_t;
    } else { // float        
        memcpy(buffer, data + line*width*pixelSize, width*pixelSize);
    }
    
    return width*channels;
}

