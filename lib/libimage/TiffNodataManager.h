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
 * \file TiffNodataManager.h
 ** \~french
 * \brief Définition de la classe TiffNodataManager, permettant de modifier la couleur de nodata des images à canal entier
 ** \~english
 * \brief Define classe TiffNodataManager, allowing to modify nodata color for byte sample image
 */

using namespace std;

#ifndef _TIFFNODATAMANAGER_
#define _TIFFNODATAMANAGER_


#include "Logger.h"
#include <stdint.h>
#include <tiff.h>
#include "tiffio.h"
#include <cstdlib>
#include <iostream>
#include <string.h>
#include <queue>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Manipulation de la couleur de nodata
 * \details On veut pouvoir modifier la couleur de nodata d'images à canal entier, et ce pour des images qui ne possèdent pas de masque de données. De plus, on veut avoir la possibilité de supprimer une couleur d'une image pour la réserver au nodata. Typiquement, il est des cas où le blanc pur doit être exclusivement utilisé pour caractériser des pixels de nodata. On va alors remplacer le blanc de donnée (neige, écume, hot spot...) par du "gris clair" (254,254,254).
 *
 * On va donc définir 3 couleurs :
 * \li la couleur cible #targetValue : les pixels de cette couleur sont ceux potentiellement modifiés
 * \li la nouvelle couleur de nodata #nodataValue (peut être la même que celle cible, pour spécifier qu'on ne veut pas la changer)
 * \li la nouvelle couleur de donnée #dataValue (peut être la même que celle cible, pour spécifier qu'on ne veut pas la changer)
 *
 * Et la classe va permettre les actions suivantes :
 * \li de modifier la couleur des pixels de nodata
 * \li de modifier la couleur des pixels de données
 * \li écrire le masque correspondant à l'image en entrée et sortie. Si l'image ne contient pas de nodata (on dit alors qu'elle est pleine), le masque n'est pas écrit.
 *
 * Pour identifier les pixels de nodata, on peut utiliser l'option "touche les bords" (#touchEdges) ou non en plus de la valeur cible.
 *
 * On dit qu'un pixel "touche le bord" dès lors que l'on peut relier le pixel au bord en ne passant que par des pixels dont la couleur est celle cible. Techniquement, on commence par les bord puis on se propage vers l'intérieur de l'image.
 *
 * \~ \image html manageNodata.png \~french
 *
 * Les fonctions utilisent les loggers et doivent donc être initialisés par un des appelant.
 */

template<typename T>
class TiffNodataManager {
private:

    /**
     * \~french \brief Largeur de l'image en cours de traitement
     * \~english \brief Width of the treated image
     */
    uint32_t width;

    /**
     * \~french \brief Hauteur de l'image en cours de traitement
     * \~english \brief Height of the treated image
     */
    uint32_t height;

    /**
     * \~french \brief Nombre de canaux de l'image en cours de traitement
     * \~english \brief Number of samples of the treated image
     */
    uint16_t samplesperpixel;

    
    /**
     * \~french \brief Nombre de canaux des couleurs du manager
     * \~english \brief Number of samples in manager colors
     */
    uint16_t channels;

    /**
     * \~french \brief Couleur concerné par les modifications éventuelles
     * \~english \brief Color impacted by modifications
     */
    T *targetValue;

    /**
     * \~french \brief Tolérance de la comparaison avec la valeur cible
     * \~english \brief Target value comparison tolerance
     */
    int tolerance;

    /**
     * \~french \brief Méthode de détection des pixels de nodata
     * \details Part-on des bords pour identifier les pixels de nodata ?
     * \~english \brief Nodata pixels detection method
     * \details Do we spread from image's edges to identify nodata pixels ?
     */
    bool touchEdges;
    
    /**
     * \~french \brief Nouvelle couleur pour les pixels de donnée
     * \details Elle peut être la même que la couleur cible #targetValue. Dans ce cas, on ne touche pas aux pixels de donnée.
     * \~english \brief New color for data
     * \details Could be the same as #targetValue.
     */
    T *dataValue;
    /**
     * \~french \brief Nouvelle couleur pour les pixels de non-donnée
     * \details Elle peut être la même que la couleur cible #targetValue. Dans ce cas, on ne touche pas aux pixels de non-donnée.
     * \~english \brief New color for nodata
     * \details Could be the same as #targetValue.
     */
    T *nodataValue;

    /**
     * \~french \brief Doit-on modifier les pixels de données contenant la valeur cible #targetValue
     * \details Cela peut être utile pour réserver une couleur aux pixels de nodata.
     * \~english \brief Have we to modify data pixel wich contain the target value #targetValue
     */
    bool removeTargetValue;
    /**
     * \~french \brief Doit-on modifier la valeur des pixels de nodata ?
     * \details On mettra la couleur #nodataValue.
     * \~english \brief Have we to switch the nodata pixels' color ?
     * \details #nodataValue will be put.
     */
    bool newNodataValue;

    /**
     * \~french \brief Identifie les pixels de nodata
     * \details Les pixels de nodata potentiels sont ceux qui ont la valeur #targetValue. Deux méthodes sont disponibles :
     * \li Tous les pixels qui ont cette valeur sont du nodata
     * \li Seuls les pixels qui ont cette valeur et qui "touchent le bord" sont du nodata
     *
     * Pour cette deuxième méthode, plus lourde, nous allons procéder comme suit. On identifie les pixels du bord dont la couleur est #targetValue, et on les ajoute dans une pile (on stocke la position en 1 dimension dans l'image). On remplit en parallèle le masque de donnée, pour mémoriser les pixels identifié comme nodata.
     *
     * Itérativement, tant que la pile n'est pas vide, on en prend la tête et on considère les 4 pixels autour. Si ils sont de la couleur #dataValue et qu'ils n'ont pas déjà été traités (on le sait grâce au masque), on les ajoute à la pile. Ainsi de suite jusqu'à vider la pile.
     *
     * Dans les deux acs, on remplit un buffer qui servira de masque afin de, au choix :
     * \li changer la valeur de nodata
     * \li changer la valeur des mixels de données qui ont la valeur #targetValue (réserver une couleur aux pixels de nodata)
     * \li écrire le masque dans un fichier
     *
     * \param[in] IM image à analyser
     * \param[out] MSK masque à remplir
     *
     * \return VRAI si l'image contient au moins 1 pixel de nodata, FAUX si elle n'en contient pas
     *
     * \~english \brief Identify nodata pixels
     * \param[in] IM image to analyze
     * \param[out] MSK mask to fill
     * \return TRUE if the image sontains 1 nodata pixel or more, FALSE otherwise
     */
    bool identifyNodataPixels ( T* IM, uint8_t* MSK );

    /**
     * \~french \brief Détermine si un pixel contient la valeur cible
     * \details Un pixel est considéré comme de la couleur cible s'il appartient à l'intervalle définit par #targetValue et #tolerance
     * \param[in] pix pixel à tester
     * \return 
     *
     * \~english \brief Determine target value pixel
     * \param[in] pix pixel to test
     */
    inline bool isTargetValue(T* pix);

    /**
     * \~french \brief Change la couleur des pixels de nodata
     * \details Les pixels de nodata ont déjà été identifiés. Reste à en changer la valeur par #nodataValue.
     *
     * \param[in,out] IM image à modifier
     * \param[in] MSK masque à utiliser
     * 
     * \~english \brief Switch color of nodata pixels
     * \param[in,out] IM image to modify
     * \param[in] MSK mask to use
     */
    void changeNodataValue ( T* IM, uint8_t* MSK );

    /**
     * \~french \brief Change la couleur des pixels de donnée
     * \details Les pixels de nodata ont déjà été identifiés. Il se peut que des pixels de données soit de la couleur #targetValue et qu'on veuille la changer (réserver une couleur aux pixels de nodata). On parcourt l'image et on change la valeur de ces derniers par #dataValue.
     *
     * \param[in,out] IM image à modifier
     * \param[in] MSK masque à utiliser
     *
     * \~english \brief Switch color of data pixels
     * \param[in,out] IM image to modify
     * \param[in] MSK mask to use
     */
    void changeDataValue ( T* IM, uint8_t* MSK );

public:

    /** \~french
     * \brief Crée un objet TiffNodataManager à partir des différentes couleurs
     * \details Les booléens #removeTargetValue et #newNodataValue sont déduits des couleurs, si elles sont identiques ou non.
     * \param[in] channels nombre de canaux dans les couleurs
     * \param[in] targetValue couleur cible
     * \param[in] touchEdges méthode de détection des pixels de nodata
     * \param[in] dataValue nouvelle couleur pour les données
     * \param[in] nodataValue nouvelle couleur pour les non-données
     ** \~english
     * \brief Create a TiffNodataManager object from three characteristic colors
     * \details Booleans #removeTargetValue and #newNodataValue are deduced from colors.
     * \param[in] channels colors' number of samples
     * \param[in] targetValue color to treat
     * \param[in] touchEdges Nodata pixels detection method
     * \param[in] dataValue new color for data
     * \param[in] nodataValue new color for nodata
     */
    TiffNodataManager ( uint16 channels, int* targetValue, bool touchEdges,
                        int* dataValue, int* nodataValue, int tolerance = 0 );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~TiffNodataManager() {
        delete[] targetValue;
        delete[] nodataValue;
        delete[] dataValue;
    }

    /** \~french
     * \brief Fonction de traitement du manager, effectuant les modification de l'image
     * \details Elle utilise les booléens #removeTargetValue et #newNodataValue pour déterminer le travail à faire. Si le travail consisite simplement à identifier le nodata et écrire un maque (pas de modification à apporter à l'image), l'image ne sera pas réecrite, même si un chemin différent pour la sortie est fourni.
     * \param[in] input chemin de l'image à modifier
     * \param[in] output chemin de l'image de sortie
     * \param[in] outputMask chemin du masque de sortie
     * \return Vrai en cas de réussite, faux sinon
     ** \~english
     * \brief Manager treatment function, doing image's modifications
     * \details Use booleans #removeTargetValue and #newNodataValue to define what to do.
     * \param[in] input Image's path to modify
     * \param[in] output Output image path
     * \param[in] output Output mask path
     * \return True if success, false otherwise
     */
    bool treatNodata ( char* inputImage, char* outputImage, char* outputMask = 0 );

};




/*****************************************************************************************************/
/******************************************** DEFINITIONS ********************************************/
/*****************************************************************************************************/




template<typename T>
TiffNodataManager<T>::TiffNodataManager ( uint16 channels, int* tv, bool touchEdges, int* dv, int* nv, int t ) :
    channels ( channels ), touchEdges(touchEdges), tolerance(t) {

    targetValue = new T[channels];
    dataValue = new T[channels];
    nodataValue = new T[channels];

    for (int i = 0; i < channels; i++) {
        targetValue[i] = (T) tv[i];
        dataValue[i] = (T) dv[i];
        nodataValue[i] = (T) nv[i];
    }

    if ( memcmp ( tv,nv,channels*sizeof(int) ) ) {
        newNodataValue = true;
    } else {
        // La nouvelle valeur de non-donnée est la même que la couleur cible : on ne change pas la couleur de non-donnée
        newNodataValue = false;
    }

    if ( memcmp ( tv,dv,channels*sizeof(int) ) && touchEdges ) {
        // Pour changer la couleur des données contenant la couleur cible, il faut avoir l'option "touche les bords".
        // Sinon, par définition, aucun pixel de la couleur cible est à considérer comme de la donnée.
        newNodataValue = true;
    } else {
        // La nouvelle valeur de donnée est la même que la couleur cible : on ne supprime donc pas la couleur cible des données
        removeTargetValue = false;
    }
}

template<typename T>
bool TiffNodataManager<T>::treatNodata ( char* inputImage, char* outputImage, char* outputMask ) {
    if ( ! newNodataValue && ! removeTargetValue && ! outputMask) {
        LOGGER_INFO ( "Have nothing to do !" );
        return true;
    }

    uint32 rowsperstrip;
    uint16 bitspersample, photometric, compression , planarconfig, nb_extrasamples;

    TIFF *TIFF_FILE = 0;

    TIFF_FILE = TIFFOpen ( inputImage, "r" );
    if ( !TIFF_FILE ) {
        LOGGER_ERROR ( "Unable to open file for reading: " << inputImage );
        return false;
    }
    if ( ! TIFFGetField ( TIFF_FILE, TIFFTAG_IMAGEWIDTH, &width )                            ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_IMAGELENGTH, &height )                       ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_BITSPERSAMPLE, &bitspersample )              ||
            ! TIFFGetFieldDefaulted ( TIFF_FILE, TIFFTAG_PLANARCONFIG, &planarconfig )       ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_PHOTOMETRIC, &photometric )                  ||
            ! TIFFGetFieldDefaulted ( TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel ) ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_COMPRESSION, &compression )                 ||
            ! TIFFGetField ( TIFF_FILE, TIFFTAG_ROWSPERSTRIP, &rowsperstrip ) ) {
        LOGGER_ERROR ( "Error reading file: " << inputImage );
        return false;
    }

    if ( planarconfig != 1 )  {
        LOGGER_ERROR ( "Sorry : only single image plane as planar configuration is handled" );
        return false;
    }

    if ( samplesperpixel > channels )  {
        LOGGER_ERROR ( "The nodata manager is not adapted (samplesperpixel have to be " << channels <<
                       " or less) for the image " << inputImage << " (" << samplesperpixel << ")" );
        return false;
    }

    /*************** Chargement de l'image ***************/

    T *IM  = new T[width * height * samplesperpixel];

    for ( int h = 0; h < height; h++ ) {
        if ( TIFFReadScanline ( TIFF_FILE, IM + width*samplesperpixel*h, h ) == -1 ) {
            LOGGER_ERROR ( "Unable to read line to " + string ( inputImage ) );
            return false;
        }
    }

    TIFFClose ( TIFF_FILE );

    /************* Calcul du masque de données ***********/

    uint8_t *MSK = new uint8_t[width * height];

    bool containNodata = identifyNodataPixels(IM, MSK);

    /*************** Modification des pixels *************/

    // 'targetValue' data pixels are replaced by 'dataValue' pixels
    if ( removeTargetValue ) {
        changeDataValue ( IM, MSK );
    }

    // nodata pixels which touch edges are replaced by 'nodataValue' pixels
    if ( newNodataValue ) {
        changeNodataValue ( IM, MSK );
    }

    /**************** Ecriture de l'images ****************/

    /* Seulement si on a modifié l'image. Si seule l'écriture du masque nous intéressait, on ne réecrit pas l'image,
     * même si un chemin d'image différent est fourni pour la sortie */
    if (removeTargetValue || newNodataValue) {

        TIFF_FILE = TIFFOpen ( outputImage, "w" );
        if ( !TIFF_FILE ) {
            LOGGER_ERROR ( "Unable to open file for writting: " + string ( outputImage ) );
            return false;
        }
        if ( ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGEWIDTH, width )                ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGELENGTH, height )              ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_BITSPERSAMPLE, bitspersample )     ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, samplesperpixel ) ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_PHOTOMETRIC, photometric )         ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip )       ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig )       ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_COMPRESSION, compression ) ) {
            LOGGER_ERROR ( "Error writting file: " +  string ( outputImage ) );
            return false;
        }

        T *LINEI = new T[width * samplesperpixel];

        // output image is written
        for ( int h = 0; h < height; h++ ) {
            memcpy ( LINEI, IM+h*width*samplesperpixel, width * samplesperpixel );
            if ( TIFFWriteScanline ( TIFF_FILE, LINEI, h ) == -1 ) {
                LOGGER_ERROR ( "Unable to write line to " + string ( outputImage ) );
                return false;
            }
        }

        TIFFClose ( TIFF_FILE );
        delete[] LINEI;
    } else {
        if (memcmp(inputImage, outputImage, sizeof(outputImage)))
            LOGGER_WARN("The image have not be modified, the file '" << outputImage <<"' is not written");
    }

    /**************** Ecriture du masque ? ****************/
    if (outputMask && containNodata) {
        TIFF_FILE = TIFFOpen ( outputMask, "w" );
        if ( !TIFF_FILE ) {
            LOGGER_ERROR ( "Unable to open file for writting: " + string ( outputMask ) );
            return false;
        }
        if ( ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGEWIDTH, width )                ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_IMAGELENGTH, height )              ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_BITSPERSAMPLE, 8 )     ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_SAMPLESPERPIXEL, 1 ) ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT ) ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK )         ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_ROWSPERSTRIP, rowsperstrip )       ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_PLANARCONFIG, planarconfig )       ||
             ! TIFFSetField ( TIFF_FILE, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE ) ) {
            LOGGER_ERROR ( "Error writting file: " +  string ( outputMask ) );
            return false;
        }

        uint8_t *LINEM = new uint8_t[width];

        // output image is written
        for ( int h = 0; h < height; h++ ) {
            memcpy ( LINEM, MSK+h*width, width);
            if ( TIFFWriteScanline ( TIFF_FILE, LINEM, h ) == -1 ) {
                LOGGER_ERROR ( "Unable to write line to " + string ( outputMask ) );
                return false;
            }
        }
        delete[] LINEM;

        TIFFClose ( TIFF_FILE );
    }


    delete[] IM;
    delete[] MSK;

    return true;
}

template<typename T>
inline bool TiffNodataManager<T>::isTargetValue(T* pix) {
    int pixint;
    int tvint;
    for (int i = 0; i < samplesperpixel; i++) {
        pixint = (int) pix[i];
        tvint = (int) targetValue[i];
        if (pixint < tvint - tolerance || pixint > tvint + tolerance) {
            return false;
        }
    }
    return true;
}

template<typename T>
bool TiffNodataManager<T>::identifyNodataPixels( T* IM, uint8_t* MSK ) {

    LOGGER_DEBUG("Identify nodata pixels...");
    memset ( MSK, 255, width * height );

    bool containNodata = false;

    if (touchEdges) {
        LOGGER_DEBUG("\t...which touch edges");
        // On utilise la couleur targetValue et on part des bords
        queue<int> Q;

        // Initialisation : we identify front pixels which are lightGray
        for ( int pos = 0; pos < width; pos++ ) { // top
            if ( isTargetValue( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( int pos = width* ( height-1 ); pos < width*height; pos++ ) { // bottom
            if ( isTargetValue( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( int pos = 0; pos < width*height; pos += width ) { // left
            if ( isTargetValue( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }
        for ( int pos = width -1; pos < width*height; pos+= width ) { // right
            if ( isTargetValue( IM + samplesperpixel * pos ) ) {
                Q.push ( pos );
                MSK[pos] = 0;
            }
        }

        if ( Q.empty() ) {
            // No nodata pixel identified, nothing to do
            return false;
        }

        containNodata = true;

        // while there are 'targetValue' pixels which can propagate, we do it
        while ( !Q.empty() ) {
            int pos = Q.front();
            Q.pop();
            int newpos;
            if ( pos % width > 0 ) {
                newpos = pos - 1;
                if ( MSK[newpos] && isTargetValue( IM + newpos*samplesperpixel ) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos % width < width - 1 ) {
                newpos = pos + 1;
                if ( MSK[newpos] && isTargetValue( IM + newpos*samplesperpixel) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos / width > 0 ) {
                newpos = pos - width;
                if ( MSK[newpos] && isTargetValue(IM + newpos*samplesperpixel) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
            if ( pos / width < height - 1 ) {
                newpos = pos + width;
                if ( MSK[newpos] && isTargetValue(IM + newpos*samplesperpixel) ) {
                    MSK[newpos] = 0;
                    Q.push ( newpos );
                }
            }
        }

    } else {
        LOGGER_DEBUG("\t..., all pixels in 'target color'");
        // Tous les pixels de la couleur targetValue sont à considérer comme du nodata

        for ( int i = 0; i < width * height; i++ ) {
            if ( isTargetValue( IM+i*samplesperpixel ) ) {
                containNodata = true;
                MSK[i] = 0;
            }
        }

    }

    return containNodata;
}

template<typename T>
void TiffNodataManager<T>::changeNodataValue ( T* IM, uint8_t* MSK ) {

    for ( int i = 0; i < width * height; i ++ ) {
        if ( ! MSK[i] ) {
            memcpy ( IM+i*samplesperpixel,nodataValue,samplesperpixel*sizeof(T) );
        }
    }
}

template<typename T>
void TiffNodataManager<T>::changeDataValue ( T* IM, uint8_t* MSK ) {

    for ( int i = 0; i < width * height; i ++ ) {
        if ( MSK[i] && isTargetValue(IM+i*samplesperpixel) ) {
            memcpy ( IM+i*samplesperpixel,dataValue,samplesperpixel*sizeof(T) );
        }
    }
}

#endif // _TIFFNODATAMANAGER_
