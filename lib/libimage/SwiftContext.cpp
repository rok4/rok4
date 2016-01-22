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

SwiftContext::SwiftContext (std::string auth, std::string account, std::string user, std::string passwd, std::string container) :
    Context(),
    auth_url(auth),user_name(user), user_account(account), user_passwd(passwd), container_name(container)
{
}

bool SwiftContext::connection() {
    LOGGER_DEBUG("Swift authentication");

    CURLcode res;
    struct curl_slist *list = NULL;

    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, auth_url.c_str());
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_NOPROXY, "*");

    // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

    char xUser[256];
    strcpy(xUser, "X-Storage-User: ");
    strcat(xUser, user_account.c_str());
    strcat(xUser, ":");
    strcat(xUser, user_name.c_str());

    char xPass[256];
    strcpy(xPass, "X-Storage-Pass: ");
    strcat(xPass, user_passwd.c_str());

    list = curl_slist_append(list, xUser);
    list = curl_slist_append(list, xPass);

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

    curl_slist_free_all(list);
    curl_easy_cleanup(curl);
    connected = true;
    return true;
}

bool SwiftContext::read(uint8_t* data, int offset, int size, std::string name) {
    //LOGGER_DEBUG("Swift read : " << size << " bytes (from the " << offset << " one) in the object " << name);

    CURLcode res;
    struct curl_slist *list = NULL;
    DataStruct chunk;
    chunk.nbPassage = 0;
    chunk.data = (char*) malloc(1);
    chunk.size = 0;

    int lastBytes = offset + size - 1;

    curl = curl_easy_init();
    //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    // On constitue le header et le moyen de récupération des informations (avec les structures de LibcurlStruct)

    char fullUrl[256];
    strcpy(fullUrl, authHdr.url);
    strcat(fullUrl, "/");
    strcat(fullUrl, container_name.c_str());
    strcat(fullUrl, "/");
    strcat(fullUrl, name.c_str());

    char range[50];
    sprintf(range, "Range: bytes=%d-%d", offset, lastBytes);

    list = curl_slist_append(list, authHdr.token);
    list = curl_slist_append(list, range);

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
    curl_easy_setopt(curl, CURLOPT_URL, fullUrl);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, data_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &chunk);

    res = curl_easy_perform(curl);
    curl_slist_free_all(list);
    curl_easy_cleanup(curl);


    if( CURLE_OK != res) {
        LOGGER_ERROR("Cannot read data from Swift : " << size << " bytes (from the " << offset << " one) in the object " << name);
        LOGGER_ERROR(curl_easy_strerror(res));
        return false;
    }

    memcpy(data, chunk.data, chunk.size);

    return true;
}


bool SwiftContext::write(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Swift write : " << size << " bytes (from the " << offset << " one) in the object " << name);
    return true;
}

bool SwiftContext::writeFull(uint8_t* data, int size, std::string name) {
    LOGGER_DEBUG("Swift write : " << size << " bytes (one shot) in the object " << name);
    return true;
}
