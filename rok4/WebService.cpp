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
#include "config.h"
#include "Image.h"
#include "Data.h"
#include "RawImage.h"


WebService::WebService(std::string url,std::string proxy="",int retry=DEFAULT_RETRY,int interval=DEFAULT_INTERVAL,
                       int timeout=DEFAULT_TIMEOUT):url (url),proxy (proxy),retry (retry), interval (interval), timeout (timeout) {}


WebService::~WebService() {

}

RawDataSource * WebService::performRequest(std::string request) {

    CURL *curl;
    CURLcode res, resC, resT;
    long responseCode = 0;
    char* responseType;
    struct MemoryStruct chunk;
    bool errors = false;
    RawDataSource *rawData = NULL;

    chunk.memory = (uint8_t*)malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    LOGGER_INFO("Create an image from a request");

    LOGGER_DEBUG("Initiation of Curl Handle");
    //it is one handle - just one per thread
    curl = curl_easy_init();
    if(curl) {
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


        LOGGER_DEBUG("Perform the request");
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
    } else {
      LOGGER_ERROR("Impossible d'initialiser Curl");
      errors = true;
    }

    /* Convert chunk into an image readable by rok4 */
    if (!errors) {
        rawData = new RawDataSource(chunk.memory, chunk.size);
    }

    free(chunk.memory);

    return rawData;

}

Image * WebService::createImageFromRequest(std::string request, int width, int height, int channels) {

    Image *img = NULL;

    RawDataSource *rawData = performRequest(request);
    if (rawData) {
        img = new RawImage(width,height,channels,rawData);
    }

    return img;
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

