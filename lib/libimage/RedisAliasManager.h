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
 * \details Classe d'abstraction du gestionnaire d'alias (redis)
 ** \~english
 * \brief Define classe RedisAliasManager
 * \details Alias manager abstraction
 */

#ifndef REDISALIASMANAGER_H
#define REDISALIASMANAGER_H

#include <stdint.h>
#include "Logger.h"
#include "AliasManager.h"
#include <string.h>
#include <hiredis.h>
#include <sstream>

static pthread_mutex_t mutex_hiredis = PTHREAD_MUTEX_INITIALIZER;

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un gestionnaire d'alias redis
 */
class RedisAliasManager : public AliasManager {

private:

    /**
     * \~french \brief Objet "contexte" redis, pour communiquer avec une base Redis
     * \~english \brief "context" redis object, to communicate with a Redis database
     */
    redisContext *rContext;

    /**
     * \~french \brief Hôte du serveur Redis
     * \~english \brief Redis server host
     */
    std::string host;

    /**
     * \~french \brief Mot de passe du serveur redis
     * \~english \brief Redis server password
     */
    std::string passwd;

    /**
     * \~french \brief Port du serveur redis
     * \~english \brief Redis server port
     */
    int port;

public:

    /**
     * \~french \brief Crée un objet RedisAliasManager
     * \param[in] h Hôte du serveur redis
     * \param[in] p Port du serveur redis
     * \param[in] pwd Mot de passe du serveur redis
     * \~english \brief Create a RedisAliasManager object
     * \param[in] h Redis server host
     * \param[in] p Redis server port
     * \param[in] pwd Redis server password
     */
    RedisAliasManager (std::string h, int p, std::string pwd) : AliasManager(), host(h), port(p), passwd(pwd) {
        ok = true;
    }

    /**
     * \~french \brief Crée un objet RedisAliasManager, à partir des variable d'environnement
     * \details Les variables d'environnement sont :
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variable d'environnement</TH>
     * <TR><TD>host</TD><TD>ROK4_REDIS_HOST</TD>
     * <TR><TD>pwd</TD><TD>ROK4_REDIS_PASSWD</TD>
     * <TR><TD>port</TD><TD>ROK4_REDIS_PORT</TD>
     * </TABLE>
     * \~english \brief Create a RedisAliasManager object, from environment variable
     * \details Environment variable are :
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variable d'environnement</TH>
     * <TR><TD>host</TD><TD>ROK4_REDIS_HOST</TD>
     * <TR><TD>pwd</TD><TD>ROK4_REDIS_PASSWD</TD>
     * <TR><TD>port</TD><TD>ROK4_REDIS_PORT</TD>
     * </TABLE>
     */
    RedisAliasManager () : AliasManager() {

        // Tout est récupéré des variables d'environnement

        char* h = getenv ("ROK4_REDIS_HOST");
        if (h == NULL) {
            LOGGER_WARN("L'utilisation d'un RedisAliasManager necessite d'avoir la variable d'environnement ROK4_REDIS_HOST" );
            return;
        }
        host.assign(h);

        char* pwd = getenv ("ROK4_REDIS_PASSWD");
        if (pwd == NULL) {
            LOGGER_WARN("L'utilisation d'un RedisAliasManager necessite d'avoir la variable d'environnement ROK4_REDIS_PASSWD" );
            return;
        }
        passwd.assign(pwd);

        char* po = getenv ("ROK4_REDIS_PORT");
        if (po == NULL) {
            LOGGER_WARN("L'utilisation d'un RedisAliasManager necessite d'avoir la variable d'environnement ROK4_REDIS_PORT" );
            return;
        }
        port = std::atoi(po);

        ok = true;
    }

    /**
     * \~french \brief Connecte le contexte redis #redisContext
     * \~english \brief Connect the redis context #redisContext
     */
    bool connect() {

        if (connected) {
            LOGGER_WARN("Redis alias manager already connected");
            return true;
        }

        // On connecte

        struct timeval timeout = { 1, 500000 }; // 1.5 seconds
        rContext = redisConnectWithTimeout(host.c_str(), port, timeout);
        if (rContext == NULL || rContext->err) {
            if (rContext) {
                LOGGER_ERROR("Redis connection error: " << std::string(rContext->errstr));
                redisFree(rContext);
            } else {
                LOGGER_ERROR("Connection error: can't allocate redis context");
            }
            return false;
        }

        // Authentification

        std::string auth = "AUTH " + passwd;
        redisReply* rReply = (redisReply*) redisCommand(rContext, auth.c_str());

        if (rReply->type == REDIS_REPLY_ERROR ) {
            std::cerr << "Connection error: can't authticate redis context: " << std::string(rReply->str) << std::endl;
            freeReplyObject(rReply);
            redisFree(rContext);
            return false;
        }

        freeReplyObject(rReply);

        connected = true;

        return true;
    }

    /**
     * \~french \brief Interroge le gestionnaire d'alias
     * \param[in] alias Clé dont on veut la valeur associée
     * \param[in] exists Est ce que la clé fournie existe
     * \return La valeur associée si la clé existe, "" sinon
     * \~english \brief Request de alias manager
     * \param[in] alias Key to obtain the associated value
     * \param[out] exists Provided key exists ?
     * \return Associated value if key exists, "" otherwise
     */
    std::string getAliasedName(std::string alias, bool* exists) {

        pthread_mutex_lock ( & mutex_hiredis );

        redisReply* rReply = (redisReply*) redisCommand(rContext, "GET %s", alias.c_str());

        if (rReply->type == REDIS_REPLY_ERROR ) {
            LOGGER_ERROR("Redis error: can't get '" << alias << "' : " << std::string(rReply->str));
            freeReplyObject(rReply);
            *exists = false;
            pthread_mutex_unlock ( & mutex_hiredis );
            return "";
        }

        if (rReply->type == 4) {
            *exists = false;
            freeReplyObject(rReply);
            pthread_mutex_unlock ( & mutex_hiredis );
            return "";
        }

        std::string res = rReply->str;
        freeReplyObject(rReply);
        *exists = true;

        pthread_mutex_unlock ( & mutex_hiredis );
        return res;
    }

    /**
     * \~french \brief Retourne le type du gestionnaire d'alias
     * \~english \brief Return the alias manager's type
     */
    eAliasManagerType getType() {
        return REDISDATABASE;
    }

    /**
     * \~french \brief Retourne le type du gestionnaire d'alias, en texte
     * \~english \brief Return the alias manager's type, as string
     */
    std::string getTypeStr() {
        return "REDISDATABASE";
    }

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~RedisAliasManager() {
        if (ok && connected) {
            redisFree(rContext);
        }
    }
};


#endif
