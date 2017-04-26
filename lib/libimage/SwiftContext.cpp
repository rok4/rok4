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
 * \file SwiftContext.cpp
 ** \~french
 * \brief Implémentation de la classe SwiftContext
 * \details
 * \li SwiftContext : connexion à un container Swift
 ** \~english
 * \brief Implement classe SwiftContext
 * \details
 * \li SwiftContext : Swift container connection
 */

#include "SwiftContext.h"
#include "LibcurlStruct.h"
#include <curl/curl.h>
#include <sys/stat.h>

SwiftContext::SwiftContext (std::string auth, std::string user, std::string passwd, std::string container, bool ks) :
    Context(),
    auth_url(auth),user_name(user), user_passwd(passwd), container_name(container), keystone_connection (ks)
{
}

SwiftContext::SwiftContext (std::string container, bool ks) : Context(), container_name(container), keystone_connection (ks) {

    char* auth = getenv ("ROK4_SWIFT_AUTHURL");
    if (auth == NULL) {
        auth_url.assign("http://localhost:8080/auth/v1.0");
    } else {
        auth_url.assign(auth);
    }

    char* user = getenv ("ROK4_SWIFT_USER");
    if (user == NULL) {
        user_name.assign("tester");
    } else {
        user_name.assign(user);
    }

    char* passwd = getenv ("ROK4_SWIFT_PASSWD");
    if (passwd == NULL) {
        user_passwd.assign("password");
    } else {
        user_passwd.assign(passwd);
    }
}

bool SwiftContext::connection() {

    if (! connected) {

        if (keystone_connection) {
            LOGGER_DEBUG("Keystone authentication");

            char* domain = getenv ("ROK4_KEYSTONE_DOMAINID");
            if (domain == NULL) {
                LOGGER_ERROR("We need a domain id (ROK4_KEYSTONE_DOMAINID) for a keystone authentication");
                return false;
            } else {
                domain_id.assign(domain);
            }

            char* project = getenv ("ROK4_KEYSTONE_PROJECTID");
            if (project == NULL) {
                LOGGER_ERROR("We need a project id (ROK4_KEYSTONE_PROJECTID) for a keystone authentication");
                return false;
            } else {
                project_id.assign(project);
            }

            char* publicu = getenv ("ROK4_SWIFT_PUBLICURL");
            if (publicu == NULL) {
                LOGGER_ERROR("We need a public url (ROK4_SWIFT_PUBLICURL) for a keystone authentication");
                return false;
            } else {
                public_url.assign(publicu);
            }

            CURLcode res;
            struct curl_slist *list = NULL;

            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

            // On constitue le header

            char* ct = "Content-Type: application/json";
            list = curl_slist_append(list, ct);

            // On constitue le body

            std::string body = "{ \"auth\": {\"scope\": { \"project\": {\"id\": \"'"+project_id+"'\"}}, ";
            body += " \"identity\": { \"methods\": [\"password\"], \"password\": { \"user\": { \"domain\": { \"id\": \"'"+domain_id+"'\"},";
            body += "\"name\": \"'"+user_name+"'\", \"password\": \"'"+user_passwd+"'\" } } } } }";

            HeaderStruct authHdr;

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &authHdr);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

            res = curl_easy_perform(curl);
            if( CURLE_OK != res) {
                LOGGER_ERROR("Cannot authenticate to Keystone");
                LOGGER_ERROR(curl_easy_strerror(res));
                curl_slist_free_all(list);
                curl_easy_cleanup(curl);
                return false;
            }

            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code < 200 || http_code > 299) {
                LOGGER_ERROR("Cannot authenticate to Keystone");
                LOGGER_ERROR("Response HTTP code : " << http_code);
                curl_slist_free_all(list);
                curl_easy_cleanup(curl);
                return false;
            }

            // On récupère le token dans le header de la réponse
            token = std::string(authHdr.token);

            curl_slist_free_all(list);
            curl_easy_cleanup(curl);

        } else {

            LOGGER_DEBUG("Swift authentication");

            char* account = getenv ("ROK4_SWIFT_ACCOUNT");
            if (account == NULL) {
                LOGGER_ERROR("We need an account (ROK4_SWIFT_ACCOUNT) for a keystone authentication");
                return false;
            } else {
                user_account.assign(account);
            }

            CURLcode res;
            struct curl_slist *list = NULL;

            curl = curl_easy_init();
            curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
            //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

            // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

            char xUser[256];
            strcpy(xUser, "X-Storage-User: ");
            strcat(xUser, user_account.c_str());
            strcat(xUser, ":");
            strcat(xUser, user_name.c_str());

            char xPass[256];
            strcpy(xPass, "X-Storage-Pass: ");
            strcat(xPass, user_passwd.c_str());

            char xAuthUser[256];
            strcpy(xAuthUser, "X-Auth-User: ");
            strcat(xAuthUser, user_account.c_str());
            strcat(xAuthUser, ":");
            strcat(xAuthUser, user_name.c_str());

            char xAuthKey[256];
            strcpy(xAuthKey, "X-Auth-Key: ");
            strcat(xAuthKey, user_passwd.c_str());


            list = curl_slist_append(list, xUser);
            list = curl_slist_append(list, xPass);        
            list = curl_slist_append(list, xAuthUser);
            list = curl_slist_append(list, xAuthKey);

            HeaderStruct authHdr;

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*) &authHdr);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);

            res = curl_easy_perform(curl);
            if( CURLE_OK != res) {
                LOGGER_ERROR("Cannot authenticate to Swift");
                LOGGER_ERROR(curl_easy_strerror(res));
                curl_slist_free_all(list);
                curl_easy_cleanup(curl);
                return false;
            }

            long http_code = 0;
            curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code < 200 || http_code > 299) {
                LOGGER_ERROR("Cannot authenticate to Swift");
                LOGGER_ERROR("Response HTTP code : " << http_code);
                curl_slist_free_all(list);
                curl_easy_cleanup(curl);
                return false;
            }

            // On récupère l'URL publique et le token dans le header de la réponse
            public_url = std::string(authHdr.url);
            token = std::string(authHdr.token);

            curl_slist_free_all(list);
            curl_easy_cleanup(curl);
        }

        connected = true;

    }

    return true;
}

int SwiftContext::read(uint8_t* data, int offset, int size, std::string name) {

    LOGGER_DEBUG("Swift read : " << size << " bytes (from the " << offset << " one) in the object " << name);

    CURLcode res;
    struct curl_slist *list = NULL;
    DataStruct chunk;
    chunk.nbPassage = 0;
    chunk.data = (char*) malloc(1);
    chunk.size = 0;

    int lastBytes = offset + size - 1;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

    std::string fullUrl;
    fullUrl = public_url + "/" + container_name + "/" + name;

    char range[50];
    sprintf(range, "Range: bytes=%d-%d", offset, lastBytes);

    list = curl_slist_append(list, token.c_str());
    list = curl_slist_append(list, range);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

    res = curl_easy_perform(curl);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    if( CURLE_OK != res) {
        LOGGER_ERROR("Cannot read data from Swift : " << size << " bytes (from the " << offset << " one) in the object " << name);
        LOGGER_ERROR(curl_easy_strerror(res));
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code > 299) {
        LOGGER_ERROR("Cannot read data from Swift : " << size << " bytes (from the " << offset << " one) in the object " << name);
        LOGGER_ERROR("Response HTTP code : " << http_code);
        return -1;
    }

    memcpy(data, chunk.data, chunk.size);

    return chunk.size;
}

bool SwiftContext::writeFromFile(std::string fileName, std::string objectName) {
    LOGGER_DEBUG("Write file '" << fileName << "' as a Swift object '" << objectName << "'");

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
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // On constitue le header

    std::string fullUrl;
    fullUrl = public_url + "/" + container_name + "/" + objectName;

    list = curl_slist_append(list, token.c_str());

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_READDATA, fileToUpload);
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

    res = curl_easy_perform(curl);


    if( CURLE_OK != res) {
        LOGGER_ERROR("Cannot upload the file " << fileName);
        LOGGER_ERROR(curl_easy_strerror(res));
        curl_slist_free_all(list);
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code > 299) {
        LOGGER_ERROR("Cannot upload the file " << fileName);
        LOGGER_ERROR("Response HTTP code : " << http_code);
        curl_slist_free_all(list);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);

    fclose(fileToUpload);

    return true;
}

eContextType SwiftContext::getType() {
    return SWIFTCONTEXT;
}

std::string SwiftContext::getTypeStr() {
    return "SWIFTCONTEXT";
}

std::string SwiftContext::getBucket() {
    return container_name;
}