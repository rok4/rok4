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
 * \file RedisAliasManager.h
 ** \~french
 * \brief Définition de la classe RedisAliasManager
 * \details
 * \li RedisAliasManager : classe d'abstraction du gestionnaire d'alias (redis)
 ** \~english
 * \brief Define classe RedisAliasManager
 * \details
 * \li RedisAliasManager : alias manager abstraction
 */

#ifndef REDISALIASMANAGER_H
#define REDISALIASMANAGER_H

#include <stdint.h>
#include "Logger.h"
#include "AliasManager.h"
#include <string.h>
#include <hiredis.h>
#include <sstream>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un gestionnaire d'alias redis
 */
class RedisAliasManager : public AliasManager {

private:

    redisContext *rContext;
    redisReply *rReply;

    std::string host;
    std::string passwd;
    int port;

public:


    /**
     * \~french \brief Crée un objet RedisAliasManager
     * \~english \brief Create a RedisAliasManager object
     */
    RedisAliasManager (std::string h, int p, std::string pwd) : AliasManager(), host(h), port(p), passwd(pwd) {

        ok = false;
        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        rContext = redisConnectWithTimeout(host.c_str(), port, timeout);
        if (rContext == NULL || rContext->err) {
            if (rContext) {
                std::cerr << "Redis connection error: " << std::string(rContext->errstr) << std::endl;
                redisFree(rContext);
            } else {
                std::cerr << ("Connection error: can't allocate redis context") << std::endl;
            }
            return;
        }

        // Authentification

        std::string auth = "AUTH " + passwd;
        rReply = (redisReply*) redisCommand(rContext, auth.c_str());

        //if (rReply->type != REDIS_REPLY_STATUS || strcasecmp(rReply->str,"OK") != 0 ) {
        if (rReply->type == REDIS_REPLY_ERROR ) {
            std::cerr << "Connection error: can't authticate redis context: " << std::string(rReply->str) << std::endl;
            freeReplyObject(rReply);
            redisFree(rContext);
            return;
        }

        freeReplyObject(rReply);

        ok = true;
    }

    std::string getAliasedName(std::string alias, bool* exists) {

        rReply = (redisReply*) redisCommand(rContext, "GET %s", alias.c_str());

        if (rReply->type == 4) {
            *exists = false;
            freeReplyObject(rReply);
            return "";
        }

        std::string res = rReply->str;
        freeReplyObject(rReply);
        *exists = true;

        return res;
    }

    eAliasManagerType getType() {
        return REDISDATABASE;
    }

    std::string getTypeStr() {
        return "REDISDATABASE";
    }

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~RedisAliasManager() {
        if (ok && rContext) {
            redisFree(rContext);
        }
    }
};


#endif
