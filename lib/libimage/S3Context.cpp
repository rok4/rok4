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

S3Context::S3Context (std::string b) : Context(), ssl_no_verify(false), bucket_name(b) {

    char* u = getenv (ROK4_S3_URL);
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

    char* k = getenv (ROK4_S3_KEY);
    if (k == NULL) {
        key.assign("KEY");
    } else {
        key.assign(k);
    }

    char* sk = getenv (ROK4_S3_SECRETKEY);
    if (sk == NULL) {
        secret_key.assign("SECRETKEY");
    } else {
        secret_key.assign(sk);
    }

    if(getenv( ROK4_SSL_NO_VERIFY ) != NULL){
        ssl_no_verify=true;
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

/**
 * \~french \brief Noms court des jours en anglais
 * \~english \brief Short english day names
 */
static const char wday_name[][4] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

/**
 * \~french \brief Noms court des mois en anglais
 * \~english \brief Short english month names
 */
static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

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

    std::string fullUrl = url + "/" + bucket_name + "/" + name;

    time_t current;
    
    time(&current);
    struct tm * ptm = gmtime ( &current );

    static char gmt_time[40];
    sprintf(
        gmt_time, "%s, %d %s %d %.2d:%.2d:%.2d GMT",
        wday_name[ptm->tm_wday], ptm->tm_mday, mon_name[ptm->tm_mon], 1900 + ptm->tm_year,
        ptm->tm_hour, ptm->tm_min, ptm->tm_sec
    );

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
    if(ssl_no_verify){
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);


    LOGGER_DEBUG("S3 READ START (" << size << ") " << pthread_self());
    res = curl_easy_perform(curl);
    LOGGER_DEBUG("S3 READ END (" << size << ") " << pthread_self());
    
    curl_slist_free_all(list);
    //delete[] gmt_time;

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

bool S3Context::write(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("S3 write : " << size << " bytes (from the " << offset << " one) in the writing buffer " << name);

    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 == writingBuffers.end() ) {
        // pas de buffer pour ce nom d'objet
        LOGGER_ERROR("No writing buffer for the name " << name);
        return false;
    }
    LOGGER_DEBUG("old length: " << it1->second->size());
   
    // Calcul de la taille finale et redimensionnement éventuel du vector
    if (it1->second->size() < size + offset) {
        it1->second->resize(size + offset);
    }

    memcpy(&((*(it1->second))[0]) + offset, data, size);
    LOGGER_DEBUG("new length: " << it1->second->size());

    return true;
}

bool S3Context::writeFull(uint8_t* data, int size, std::string name) {
    LOGGER_DEBUG("S3 write : " << size << " bytes (one shot) in the writing buffer " << name);

    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 == writingBuffers.end() ) {
        // pas de buffer pour ce nom d'objet
        LOGGER_ERROR("No S3 writing buffer for the name " << name);
        return false;
    }

    it1->second->clear();

    it1->second->resize(size);
    memcpy(&((*(it1->second))[0]), data, size);

    return true;
}

ContextType::eContextType S3Context::getType() {
    return ContextType::S3CONTEXT;
}

std::string S3Context::getTypeStr() {
    return "S3Context";
}

std::string S3Context::getTray() {
    return bucket_name;
}

std::string S3Context::getPath(std::string racine,int x,int y,int pathDepth){
    //std::ostringstream convert;
    //convert << "_" << x << "_" << y;
    //convert.str();
    return racine + "_" + std::to_string(x) + "_" + std::to_string(y);
}


bool S3Context::openToWrite(std::string name) {

    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 != writingBuffers.end() ) {
        LOGGER_ERROR("A S3 writing buffer already exists for the name " << name);
        return false;

    } else {
        writingBuffers.insert ( std::pair<std::string,std::vector<char>*>(name, new std::vector<char>()) );
    }

    return true;
}


bool S3Context::closeToWrite(std::string name) {


    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 == writingBuffers.end() ) {
        LOGGER_ERROR("The S3 writing buffer with name " << name << "does not exist, cannot flush it");
        return false;
    }

    LOGGER_DEBUG("Write buffered " << it1->second->size() << " bytes in the S3 object " << name);

    CURLcode res;
    struct curl_slist *list = NULL;
    CURL* curl = CurlPool::getCurlEnv();

    std::string fullUrl = url + "/" + bucket_name + "/" + name;

    time_t current;
    char gmt_time[40];

    //On passe en locale="C"  Pour l'autorisation des requètes S3
    char loc[32] ;
    strcpy( loc, setlocale(LC_ALL,NULL));
    setlocale(LC_ALL, "C");
    time(&current);
    strftime( gmt_time, sizeof(gmt_time), "%a, %d %b %Y %T %z", gmtime(&current) );
    setlocale(LC_ALL, loc); 

    std::string content_type = "application/octet-stream";
    std::string resource = "/" + bucket_name + "/" + name;
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
    sprintf(cl, "Content-Length: %d",(int)it1->second->size());
    list = curl_slist_append(list, cl);

    std::string ex = "Expect:";
    list = curl_slist_append(list, ex.c_str());

    char auth[512];
    sprintf(auth, "Authorization: AWS %s:%s", key.c_str(), signature.c_str());
    list = curl_slist_append(list, auth);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    if(ssl_no_verify){
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &((*(it1->second))[0]));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, it1->second->size());

    res = curl_easy_perform(curl);
    curl_slist_free_all(list);


    if( CURLE_OK != res) {
        LOGGER_ERROR ( "Unable to flush " << it1->second->size() << " bytes in the object " << name );
        LOGGER_ERROR(curl_easy_strerror(res));
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code > 299) {
        LOGGER_ERROR ( "Unable to flush " << it1->second->size() << " bytes in the object " << name );
        LOGGER_ERROR("Response HTTP code : " << http_code);
        return false;
    }

    LOGGER_DEBUG("Erase the flushed buffer");
    delete it1->second;
    writingBuffers.erase(it1);

    return true;
}