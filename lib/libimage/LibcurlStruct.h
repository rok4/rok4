/*
 * Copyright © (2011) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file LibcurlStruct.h
 ** \~french
 * \brief Définition de structures et fonctions utilisées pour récupérer de la donnée via la lib cURL
 ** \~english
 * \brief Define strcutures and functions used to get data via cURL library
 */

#ifndef LIBCURL_STRUCT_H
#define LIBCURL_STRUCT_H

#include <stdlib.h>

struct HeaderStruct {
    char* url;
    char* token;

    HeaderStruct()
    {
        url = 0;
        token = 0;
    }

    ~HeaderStruct()
    {
        if (url) free(url);
        if (token) free(token);
    }
};

struct DataStruct {
    int nbPassage;
    size_t size;
    char* data;

    ~DataStruct()
    {
        if (data) free(data);
    }
};


static size_t header_callback(char *buffer, size_t nitems, size_t size, void *userp) {

    size_t realsize = size * nitems;
    struct HeaderStruct* hdr = (HeaderStruct*) userp;

    if (! strncmp ( buffer,"X-Storage-Url: ", 15)) {
        hdr->url = (char*) malloc(realsize - 15);
        strncpy(hdr->url, buffer + 15, realsize - 15 - 2);
        hdr->url[realsize - 15 - 2] = '\0';
    }

    else if (! strncmp ( buffer,"X-Auth-Token: ", 14)) {
        hdr->token = (char*) malloc(realsize);
        strncpy(hdr->token, buffer, realsize - 2);
        hdr->token[realsize - 2] = '\0';
    }

    else if (! strncmp ( buffer,"X-Subject-Token: ", 17)) {
        hdr->token = (char*) malloc(realsize - 17 + 14);
        strncpy(hdr->token, "X-Auth-Token: ", 14);
        strncpy(hdr->token + 14, buffer + 17, realsize - 17 - 2);
        hdr->token[realsize - 17 + 14 - 2] = '\0';
    }

    return realsize;
}

static size_t data_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;

    struct DataStruct *mem = (struct DataStruct *)userp;
    mem->nbPassage = mem->nbPassage + 1;

    mem->data = (char*)realloc(mem->data, mem->size + realsize + 1);
    if(mem->data == NULL) {
        /* out of memory! */
        BOOST_LOG_TRIVIAL(error) << "not enough memory (realloc returned NULL)";
        return 0;
    }

    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

#endif
