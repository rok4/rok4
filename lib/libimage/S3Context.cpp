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
 * \file S3Context.cpp
 ** \~french
 * \brief Implémentation de la classe S3Context
 * \details
 * \li S3Context : connexion à un container S3
 ** \~english
 * \brief Implement classe S3Context
 * \details
 * \li S3Context : S3 container connection
 */

#include "S3Context.h"
#include "LibcurlStruct.h"
#include <curl/curl.h>
#include <sys/stat.h>
#include <openssl/hmac.h>
#include <time.h>
#include "CurlPool.h"

S3Context::S3Context (std::string u, std::string k, std::string sk, std::string b) :
    Context(),
    url(u), key(k), secret_key(sk), bucket_name(b)
{
    // On calcule host = url sans le protocole ni le port

    host = url;
    std::size_t found = host.find("://");
    if (found != std::string::npos) {
        host = host.substr(found + 3);
    }
    found = host.find(":");
    if (found != std::string::npos) {
        host = host.substr(0, found);
    }
}

S3Context::S3Context (std::string b) : Context(), bucket_name(b) {

    char* u = getenv ("ROK4_S3_URL");
    if (u == NULL) {
        url.assign("http://localhost:8080");
    } else {
        url.assign(u);
    }

    // On calcule host = url sans le protocole ni le port

    host = url;
    std::size_t found = host.find("://");
    if (found != std::string::npos) {
        host = host.substr(found + 3);
    }
    found = host.find(":");
    if (found != std::string::npos) {
        host = host.substr(0, found);
    }    

    char* k = getenv ("ROK4_S3_KEY");
    if (k == NULL) {
        key.assign("KEY");
    } else {
        key.assign(k);
    }

    char* sk = getenv ("ROK4_S3_SECRETKEY");
    if (sk == NULL) {
        secret_key.assign("SECRETKEY");
    } else {
        secret_key.assign(sk);
    }
}

bool S3Context::connection() {
    connected = true;
    return true;
}

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

std::string S3Context::getAuthorizationHeader(std::string toSign) {

    // Using sha1 hash engine here.
    unsigned char* bytes_to_encode = HMAC(EVP_sha1(), secret_key.c_str(), secret_key.length(), ( const unsigned char*) toSign.c_str(), toSign.length(), NULL, NULL);

    std::string signature;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    int in_len = 20;
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                signature += base64_chars[char_array_4[i]];
                i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            signature += base64_chars[char_array_4[j]];

        while((i++ < 3))
            signature += '=';
    }

    //delete[] bytes_to_encode;

    return signature;
}

int S3Context::read(uint8_t* data, int offset, int size, std::string name) {

    LOGGER_DEBUG("S3 read : " << size << " bytes (from the " << offset << " one) in the object " << name);

    // On constitue le moyen de récupération des informations (avec les structures de LibcurlStruct)

    CURLcode res;
    struct curl_slist *list = NULL;
    DataStruct chunk;
    chunk.nbPassage = 0;
    chunk.data = (char*) malloc(1);
    chunk.size = 0;

    int lastBytes = offset + size - 1;

    CURL* curl = CurlPool::getCurlEnv();
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);


    std::string fullUrl = url + "/" + bucket_name + "/" + name;

    time_t current;
    char gmt_time[40];

    time(&current);
    strftime( gmt_time, sizeof(gmt_time), "%a, %d %b %Y %T %z", gmtime(&current) );

    std::string content_type = "application/octet-stream";
    std::string resource = "/" + bucket_name + "/" + name;
    std::string stringToSign = "GET\n\n" + content_type + "\n" + std::string(gmt_time) + "\n" + resource;
    std::string signature = getAuthorizationHeader(stringToSign);

    // Constitution du header

    char range[50];
    sprintf(range, "Range: bytes=%d-%d", offset, lastBytes);
    list = curl_slist_append(list, range);

    char hd_host[256];
    sprintf(hd_host, "Host: %s", host.c_str());
    list = curl_slist_append(list, hd_host);

    char d[100];
    sprintf(d, "Date: %s", gmt_time);
    list = curl_slist_append(list, d);

    char ct[50];
    sprintf(ct, "Content-Type: %s", content_type.c_str());
    list = curl_slist_append(list, ct);

    std::string ex = "Expect:";
    list = curl_slist_append(list, ex.c_str());

    char auth[512];
    sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());
    list = curl_slist_append(list, auth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

    res = curl_easy_perform(curl);
    curl_slist_free_all(list);

    double time;
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &time);
    LOGGER_INFO("CURLTIME=" << time << " (" << size << ")");

    if( CURLE_OK != res) {
        LOGGER_ERROR("Cannot read data from S3 : " << size << " bytes (from the " << offset << " one) in the object " << name);
        LOGGER_ERROR(curl_easy_strerror(res));
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code > 299) {
        LOGGER_ERROR("Cannot read data from S3 : " << size << " bytes (from the " << offset << " one) in the object " << name);
        LOGGER_ERROR("Response HTTP code : " << http_code);
        LOGGER_ERROR("Response HTTP : " << chunk.data);
        return -1;
    }

    memcpy(data, chunk.data, chunk.size);

    return chunk.size;
}

bool S3Context::writeFromFile(std::string fileName, std::string objectName) {
    LOGGER_DEBUG("Write file '" << fileName << "' as a S3 object '" << objectName << "'");

    struct stat file_info;
    FILE *fileToUpload;
    fileToUpload = fopen(fileName.c_str(), "rb");

    if(! fileToUpload) {
        LOGGER_ERROR("Cannot open the file to upload " << fileName);
        return false;
    }

    if(fstat(fileno(fileToUpload), &file_info) != 0) {
        LOGGER_ERROR("Cannot obtain the file's size' to upload " << fileName);
        return false;
    }
    LOGGER_DEBUG("Size to upload " << file_info.st_size);


    CURLcode res;
    struct curl_slist *list = NULL;
    CURL* curl = CurlPool::getCurlEnv();
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    std::string fullUrl = url + "/" + bucket_name + "/" + objectName;

    time_t current;
    char gmt_time[40];

    time(&current);
    strftime( gmt_time, sizeof(gmt_time), "%a, %d %b %Y %T %z", gmtime(&current) );

    std::string content_type = "application/octet-stream";
    std::string resource = "/" + bucket_name + "/" + objectName;
    std::string stringToSign = "PUT\n\n" + content_type + "\n" + std::string(gmt_time) + "\n" + resource;
    std::string signature = getAuthorizationHeader(stringToSign);

    // Constitution du header

    char host[256];
    sprintf(host, "Host: %s", url.c_str());
    list = curl_slist_append(list, host);

    char d[100];
    sprintf(d, "Date: %s", gmt_time);
    list = curl_slist_append(list, d);

    char ct[50];
    sprintf(ct, "Content-Type: %s", content_type.c_str());
    list = curl_slist_append(list, ct);

    char cl[50];
    sprintf(cl, "Content-Length: %d", file_info.st_size);
    list = curl_slist_append(list, cl);

    std::string ex = "Expect:";
    list = curl_slist_append(list, ex.c_str());

    char auth[512];
    sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());
    list = curl_slist_append(list, auth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, fileToUpload);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

    res = curl_easy_perform(curl);
    curl_slist_free_all(list);


    if( CURLE_OK != res) {
        LOGGER_ERROR("Cannot upload the file " << fileName);
        LOGGER_ERROR(curl_easy_strerror(res));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code > 299) {
        LOGGER_ERROR("Cannot upload the file " << fileName);
        LOGGER_ERROR("Response HTTP code : " << http_code);
        return false;
    }


    fclose(fileToUpload);

    return true;
}

eContextType S3Context::getType() {
    return S3CONTEXT;
}

std::string S3Context::getTypeStr() {
    return "S3Context";
}

std::string S3Context::getTray() {
    return bucket_name;
}
