/*
 * Copyright © (2011-2013) Institut national de l'information
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


#include "WebService.h"
#include "config.h.in"
#include "Image.h"
#include "Data.h"
#include "RawImage.h"
#include "Format.h"
#include "Decoder.h"
#include "ExtendedCompoundImage.h"
#include "EmptyImage.h"
#include "CompoundImage.h"


WebService::WebService(std::string url,std::string proxy="",int retry=DEFAULT_RETRY,int interval=DEFAULT_INTERVAL,
                       int timeout=DEFAULT_TIMEOUT):url (url),proxy (proxy),retry (retry), interval (interval), timeout (timeout) {}


WebService::~WebService() {

}

RawDataSource * WebService::performRequest(std::string request) {

    //----variables
    CURL *curl;
    CURLcode res, resC, resT;
    long responseCode = 0;
    char* responseType;
    struct MemoryStruct chunk;
    bool errors = false;
    RawDataSource *rawData = NULL;
    int nbPerformed = 0;

    chunk.memory = (uint8_t*)malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */
    //----

    LOGGER_INFO("Perform a request");

    LOGGER_DEBUG("Initiation of Curl Handle");
    //it is one handle - just one per thread
    curl = curl_easy_init();

    if(curl) {

        //----Set options
        LOGGER_DEBUG("Setting options of Curl");
        curl_easy_setopt(curl, CURLOPT_URL, request.c_str());
        /* example.com is redirected, so we tell libcurl to follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        /* Switch on full protocol/debug output while testing, set to 0L to disable */
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        /* disable progress meter, set to 0L to enable and disable debug output */
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_HEADER, 0L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "identity");
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteInMemoryCallback);
        /* we pass our 'chunk' struct to the callback function */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        //----

        //----Perform request
        while (nbPerformed <= retry) {

            nbPerformed++;

            LOGGER_DEBUG("Perform the request - " << nbPerformed << "/" << retry+1 << " time");
            /* Perform the request, res will get the return code */
            res = curl_easy_perform(curl);

            LOGGER_DEBUG("Checking for errors");
            /* Check for errors */
            if(res == CURLE_OK) {

                resC = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
                resT = curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &responseType);
                if ((resC == CURLE_OK) && responseCode) {

                    if (responseCode != 200) {
                        LOGGER_ERROR("The request returned a " << responseCode << " code");
                        errors = true;
                    } else {
                        errors = false;
                        /* always cleanup */
                        curl_easy_cleanup(curl);
                        break;
                    }

                } else {
                    LOGGER_ERROR("curl_easy_getinfo() on response code failed: " << curl_easy_strerror(resC));
                    errors = true;
                }

                if ((resT == CURLE_OK) && responseType) {
                    if (errors) {
                        LOGGER_ERROR("The request returned with a " << responseType << " content type");
                        std::string text = "text/";
                        std::string rType(responseType);
                        if (rType.find(text) != std::string::npos) {
                            LOGGER_ERROR("Content of the answer: " << chunk.memory);
                        } else {
                            LOGGER_ERROR("Impossible to read the answer...");
                        }
                    }
                } else {
                    LOGGER_ERROR("curl_easy_getinfo() on response type failed: " << curl_easy_strerror(resT));
                    errors = true;
                }



            } else {
                LOGGER_ERROR("curl_easy_perform() failed: " << curl_easy_strerror(res));
                errors = true;
            }

            /* always cleanup */
            curl_easy_cleanup(curl);

        }
        //----


    } else {
      LOGGER_ERROR("Impossible d'initialiser Curl");
      errors = true;
    }

    /* Convert chunk into a DataSource readable by rok4 */
    if (!errors) {
        LOGGER_DEBUG("Sauvegarde de la donnee");
        rawData = new RawDataSource(chunk.memory, chunk.size);
    }

    free(chunk.memory);

    return rawData;

}

Image * WebMapService::createImageFromRequest(int width, int height, BoundingBox<double> askBbox) {

    Image *img = NULL;
    DataSource *decData = NULL;
    int pix = 1;
    std::string request;
    int requestWidth,requestHeight;
    ExtendedCompoundImageFactory facto;
    std::vector<Image*> images;
    Image *finalImage = NULL;

    //----Récupération du NDValues
    int *ndvalue = new int[channels];
    std::vector<int> ndval = getNdValues();
    for ( int c = 0; c < channels; c++ ) {
        ndvalue[c] = ndval[c];
    }
    //----


    LOGGER_INFO("Create an image from a request");

    //----creation de la requete
    //on adapte la bbox de la future requete aux données
    BoundingBox<double> requestBbox = askBbox.adaptTo(bbox);

    //on cree la requete en fonction de la nouvelle bbox
    if (requestBbox.isNull()) {
        LOGGER_DEBUG ("New Bbox is null");
        EmptyImage* fond = new EmptyImage(width, height, channels, ndvalue);
        fond->setBbox(bbox);
        delete[] ndvalue;
        return fond;
    }
    if (askBbox.isEqual(requestBbox)) {
        //les deux bbox sont égales
        requestWidth = width;
        requestHeight = height;
        request = createWMSGetMapRequest(requestBbox,requestWidth,requestHeight);
    } else {
        //requestBbox est plus petite à cause de la fonction adaptTo(bbox)
        //donc il faut recalculer width et height

        double ratio_x = ( requestBbox.xmax - requestBbox.xmin ) / ( askBbox.xmax - askBbox.xmin );
        double ratio_y = ( requestBbox.ymax - requestBbox.ymin ) / ( askBbox.ymax - askBbox.ymin ) ;
        int newWidth = lround(width * ratio_x);
        int newHeight = lround(height * ratio_y);


        //Avec lround, taille en pixel et cropBBox ne sont plus cohérents.
        //On ajoute la différence de l'arrondi dans la cropBBox et on ajoute un pixel en plus tout autour.

        //Calcul de l'erreur d'arrondi converti en coordonnées
        double delta_h = double (newHeight) - double(height) * ratio_y ;
        double delta_w = double (newWidth) - double(width) * ratio_x ;

        double res_y = ( requestBbox.ymax - requestBbox.ymin ) / double(height * ratio_y) ;
        double res_x = ( requestBbox.xmax - requestBbox.xmin ) / double(width * ratio_x) ;

        double delta_y = res_y * delta_h ;
        double delta_x = res_x * delta_w ;

        //Ajout de l'erreur d'arrondi et le pixel en plus
        requestBbox.ymax += delta_y +res_y;
        requestBbox.ymin -= res_y;

        requestBbox.xmax += delta_x +res_x;
        requestBbox.xmin -= res_x;

        newHeight += 2;
        newWidth += 2;

        LOGGER_DEBUG ( "New Width = " << newWidth << " " <<  "New Height = " << newHeight );
        LOGGER_DEBUG (  "ratio_x = "  << ratio_x << " " <<  "ratio_y = " << ratio_y );

        if ( (1/ratio_x > 5 && newWidth < 3) || (newHeight < 3 && 1/ratio_y > 5) || (newWidth <= 0 || newHeight <= 0)){
            //Too small BBox
            LOGGER_DEBUG ("New Bbox's size too small. Can't hope to have an image");
            EmptyImage* fond = new EmptyImage(width, height, channels, ndvalue);
            fond->setBbox(bbox);
            delete[] ndvalue;
            return fond;
        } else {
            requestWidth = newWidth;
            requestHeight = newHeight;
            request = createWMSGetMapRequest(requestBbox,requestWidth,requestHeight);
        }


    }

    LOGGER_DEBUG("Request => " << request);
    //----

    //----on récupère la donnée brute
    RawDataSource *rawData = performRequest(request);
    //----

    //----on la transforme en image
    if (rawData) {

        LOGGER_DEBUG("Decode Data");
        //on la décode
        Rok4Format::eformat_data fmt = Rok4Format::fromMimeType(format);
        if ( fmt==Rok4Format::TIFF_RAW_INT8 || fmt==Rok4Format::TIFF_RAW_FLOAT32 )
            decData = rawData;
        else if ( fmt == Rok4Format::TIFF_JPG_INT8 )
            decData = new DataSourceDecoder<JpegDecoder> ( rawData );
        else if ( fmt == Rok4Format::TIFF_PNG_INT8 )
            decData = new DataSourceDecoder<PngDecoder> ( rawData );
        else if ( fmt == Rok4Format::TIFF_LZW_INT8 || fmt == Rok4Format::TIFF_LZW_FLOAT32 )
            decData = new DataSourceDecoder<LzwDecoder> ( rawData );
        else if ( fmt == Rok4Format::TIFF_ZIP_INT8 || fmt == Rok4Format::TIFF_ZIP_FLOAT32 )
            decData = new DataSourceDecoder<DeflateDecoder> ( rawData );
        else if ( fmt == Rok4Format::TIFF_PKB_INT8 || fmt == Rok4Format::TIFF_PKB_FLOAT32 )
            decData = new DataSourceDecoder<PackBitsDecoder> ( rawData );

        LOGGER_DEBUG("Create Image");
        //on en fait une image
        pix = Rok4Format::getPixelSize(fmt);
        img = new ImageDecoder(decData,requestWidth,requestHeight,channels,requestBbox,0,0,0,0,pix);
        img->setCRS(CRS(crs));


    }
    //----

    //----on complete l'image avec du noData si nécessaire
    if (!(askBbox.isEqual(requestBbox)) && rawData) {

        images.push_back(img);
        finalImage = facto.createExtendedCompoundImage ( width,height,channels,askBbox,images,ndvalue,0 );

    } else {
        finalImage = img;
    }
    //----
    delete[] ndvalue;

    return finalImage;
}

Image * WebMapService::createSlabFromRequest(int width, int height, BoundingBox<double> askBbox) {

    DataSource *decData = NULL;
    int pix = 1;
    std::string request;
    int dataWidth,dataHeight;
    ExtendedCompoundImageFactory facto;
    std::vector<Image*> images;
    Image *finalImage = NULL;
    int nbRequestH = 1;
    int newHeight,newWidth;
    double newWidthLand,newHeightLand;
    int nbRequestW = 1;
    std::vector<std::vector<Image*> > composeImg;

    //----Récupération du NDValues
    int *ndvalue = new int[channels];
    std::vector<int> ndval = getNdValues();
    for ( int c = 0; c < channels; c++ ) {
        ndvalue[c] = ndval[c];
    }
    //----

    LOGGER_INFO("Create an image from a request");

    //----creation de la requete
    //on adapte la bbox de la future requete aux données
    BoundingBox<double> dataBbox = askBbox.adaptTo(bbox);

    //on cree la requete en fonction de la nouvelle bbox
    if (dataBbox.isNull()) {
        LOGGER_DEBUG ("New Bbox is null");
        EmptyImage* fond = new EmptyImage(width, height, channels, ndvalue);
        fond->setBbox(bbox);
        delete[] ndvalue;
        return fond;
    }
    if (askBbox.isEqual(dataBbox)) {
        //les deux bbox sont égales
        dataWidth = width;
        dataHeight = height;

    } else {
        //requestBbox est plus petite à cause de la fonction adaptTo(bbox)
        //donc il faut recalculer width et height

        double ratio_x = ( dataBbox.xmax - dataBbox.xmin ) / ( askBbox.xmax - askBbox.xmin );
        double ratio_y = ( dataBbox.ymax - dataBbox.ymin ) / ( askBbox.ymax - askBbox.ymin ) ;
        int newWidth = lround(width * ratio_x);
        int newHeight = lround(height * ratio_y);


        //Avec lround, taille en pixel et cropBBox ne sont plus cohérents.
        //On ajoute la différence de l'arrondi dans la cropBBox et on ajoute un pixel en plus tout autour.

        //Calcul de l'erreur d'arrondi converti en coordonnées
        double delta_h = double (newHeight) - double(height) * ratio_y ;
        double delta_w = double (newWidth) - double(width) * ratio_x ;

        double res_y = ( dataBbox.ymax - dataBbox.ymin ) / double(height * ratio_y) ;
        double res_x = ( dataBbox.xmax - dataBbox.xmin ) / double(width * ratio_x) ;

        double delta_y = res_y * delta_h ;
        double delta_x = res_x * delta_w ;

        //Ajout de l'erreur d'arrondi et le pixel en plus
        dataBbox.ymax += delta_y +res_y;
        dataBbox.ymin -= res_y;

        dataBbox.xmax += delta_x +res_x;
        dataBbox.xmin -= res_x;

        newHeight += 2;
        newWidth += 2;

        LOGGER_DEBUG ( "New Width = " << newWidth << " " <<  "New Height = " << newHeight );
        LOGGER_DEBUG (  "ratio_x = "  << ratio_x << " " <<  "ratio_y = " << ratio_y );

        if ( (1/ratio_x > 5 && newWidth < 3) || (newHeight < 3 && 1/ratio_y > 5) || (newWidth <= 0 || newHeight <= 0)){
            //Too small BBox
            LOGGER_DEBUG ("New Bbox's size too small. Can't hope to have an image");
            EmptyImage* fond = new EmptyImage(width, height, channels, ndvalue);
            fond->setBbox(bbox);
            delete[] ndvalue;
            return fond;
        } else {
            dataWidth = newWidth;
            dataHeight = newHeight;
        }


    }

    //----Check image width and height to make multiple requests if necessary

    if (dataWidth >= DEFAULT_MAX_SIZE_BEFORE_CUT) {
        for (int k = 2; k <= DEFAULT_MAX_NB_CUT; k++) {
            if ((dataWidth % k) == 0) {
                nbRequestW = k;
                newWidth = dataWidth / k;
                newWidthLand = (dataBbox.xmax - dataBbox.xmin) / k;
            }
        }
    } else {
        newWidth = dataWidth;
        newWidthLand = (dataBbox.xmax - dataBbox.xmin);
    }

    if (dataHeight >= DEFAULT_MAX_SIZE_BEFORE_CUT) {
        for (int k = 2; k <= DEFAULT_MAX_NB_CUT; k++) {
            if ((dataHeight % k) == 0) {
                nbRequestH = k;
                newHeight = dataHeight / k;
                newHeightLand = (dataBbox.ymax - dataBbox.ymin) / k;
            }
        }
    } else {
        newHeight = dataHeight;
        newHeightLand = (dataBbox.ymax - dataBbox.ymin);
    }

    if (nbRequestH > 1 || nbRequestW > 1) {
        LOGGER_DEBUG("Multiple request will be performed to compute the image");
    }


    //----

    //----Dimensionnement de composeImg
    composeImg.resize(nbRequestH);
    for (int row = 0; row < nbRequestH; row++) {
        composeImg.at(row).resize(nbRequestW);
    }
    //----

    for (int i = 0; i < nbRequestH; i++) {
        for (int j = 0; j < nbRequestW; j++) {

            BoundingBox<double> requestBbox = BoundingBox<double>(dataBbox.xmin + j*newWidthLand,
                                                              dataBbox.ymin + i*newHeightLand,
                                                              dataBbox.xmin + (j+1)*newWidthLand,
                                                              dataBbox.ymin + (i+1)*newHeightLand);

            request = createWMSGetMapRequest(requestBbox,newWidth,newHeight);

            LOGGER_DEBUG("Request => " << request);
            //----

            //----on récupère la donnée brute
            RawDataSource *rawData = performRequest(request);
            //----

            //----on la transforme en image
            if (rawData) {

                LOGGER_DEBUG("Decode Data");
                //on la décode
                Rok4Format::eformat_data fmt = Rok4Format::fromMimeType(format);
                if ( fmt==Rok4Format::TIFF_RAW_INT8 || fmt==Rok4Format::TIFF_RAW_FLOAT32 )
                    decData = rawData;
                else if ( fmt == Rok4Format::TIFF_JPG_INT8 )
                    decData = new DataSourceDecoder<JpegDecoder> ( rawData );
                else if ( fmt == Rok4Format::TIFF_PNG_INT8 )
                    decData = new DataSourceDecoder<PngDecoder> ( rawData );
                else if ( fmt == Rok4Format::TIFF_LZW_INT8 || fmt == Rok4Format::TIFF_LZW_FLOAT32 )
                    decData = new DataSourceDecoder<LzwDecoder> ( rawData );
                else if ( fmt == Rok4Format::TIFF_ZIP_INT8 || fmt == Rok4Format::TIFF_ZIP_FLOAT32 )
                    decData = new DataSourceDecoder<DeflateDecoder> ( rawData );
                else if ( fmt == Rok4Format::TIFF_PKB_INT8 || fmt == Rok4Format::TIFF_PKB_FLOAT32 )
                    decData = new DataSourceDecoder<PackBitsDecoder> ( rawData );

                LOGGER_DEBUG("Create Image");
                //on en fait une image
                pix = Rok4Format::getPixelSize(fmt);
                Image *img = new ImageDecoder(decData,newWidth,newHeight,channels,requestBbox,0,0,0,0,pix);
                img->setCRS(CRS(crs));
                composeImg[nbRequestH - (i+1)][j] = img;


            } else {
                LOGGER_ERROR("No Raw Data...");
                delete[] ndvalue;
                return finalImage;
            }
            //----

        }
    }

    CompoundImage *cmImg = new CompoundImage(composeImg);


    //----on complete l'image avec du noData si nécessaire
    if (!(askBbox.isEqual(dataBbox))) {

        images.push_back(cmImg);
        finalImage = facto.createExtendedCompoundImage ( width,height,channels,askBbox,images,ndvalue,0 );

    } else {
        finalImage = cmImg;
    }
    //----
    delete[] ndvalue;

    return finalImage;
}

bool WebMapService::hasOption ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = options.find ( paramName );
    if ( it == options.end() ) {
        return false;
    }
    return true;
}

std::string WebMapService::getOption ( std::string paramName ) {
    std::map<std::string, std::string>::iterator it = options.find ( paramName );
    if ( it == options.end() ) {
        return "";
    }
    return it->second;
}

std::string WebMapService::createWMSGetMapRequest ( BoundingBox<double> bbox, int width, int height ) {

    std::string request = "";
    std::ostringstream str_w,str_h;
    str_w << width;
    str_h << height;

    if (version == "1.3.0") {
        request = url+"VERSION="+version
                +"&REQUEST=GetMap"
                +"&SERVICE=WMS"
                +"&LAYERS="+layers
                +"&STYLES="+styles
                +"&FORMAT="+format
                +"&CRS="+crs
                +"&BBOX="+bbox.toString()
                +"&WIDTH="+str_w.str()
                +"&HEIGHT="+str_h.str();

        if (options.size() != 0) {
            for ( std::map<std::string,std::string>::iterator lv = options.begin(); lv != options.end(); lv++) {
                request += "&"+lv->first+"="+lv->second;
            }
        }

    }

    return request;


}

WebMapService::~WebMapService() {}

