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
 * \file SwiftContext.h
 ** \~french
 * \brief Définition de la classe SwiftContext
 * \details
 * \li SwiftContext : connexion à un container Swift
 ** \~english
 * \brief Define classe SwiftContext
 * \details
 * \li SwiftContext : Swift container connection
 */

#ifndef SWIFT_CONTEXT_H
#define SWIFT_CONTEXT_H

#include <curl/curl.h>
#include "Logger.h"
#include "Context.h"
#include "LibcurlStruct.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte Swift (connexion à un cluster + pool particulier), pour pouvoir récupérer des données stockées sous forme d'objets
 */
class SwiftContext : public Context{
    
private:
    
    /**
     * \~french \brief URL d'authentification à Swift
     * \~english \brief Authentication URL to Swift
     */
    std::string auth_url;
    /**
     * \~french \brief URL de communication avec Swift
     * \~english \brief Communication URL with Swift
     */
    std::string public_url;
    /**
     * \~french \brief Accompte Swift
     * \~english \brief Swift account
     */
    std::string user_account;
    /**
     * \~french \brief Utilisateur Swift
     * \~english \brief Swift user
     */
    std::string user_name;
    /**
     * \~french \brief Mot de passe Swift
     * \~english \brief Swift password
     */
    std::string user_passwd;
    /**
     * \~french \brief Nom du conteneur Swift
     * \~english \brief Swift container name
     */
    std::string container_name;
    
    /**
     * \~french \brief Objet Curl pour communiquer avec l'API rest de Swift
     * \~english \brief Curl object to communicate with Swift Rest API
     */
    CURL *curl;

    /**
     * \~french \brief En-tête HTTP à utiliser pour chaque échange avec Swift (contient le token)
     * \~english \brief HTTP header to use to communicate with Swift (contain the token)
     */
    HeaderStruct authHdr;

public:

    /**
     * \~french
     * \brief Constructeur pour un contexte Swift
     * \param[in] name Nom du cluster Swift
     * \param[in] user Nom de l'utilisateur Swift
     * \param[in] conf Configuration du cluster Swift
     * \param[in] container Conteneur avec lequel on veut communiquer
     * \~english
     * \brief Constructor for Swift context
     * \param[in] name Name of Swift cluster
     * \param[in] user Name of Swift user
     * \param[in] conf Swift configuration file
     * \param[in] container Container to use
     */
    SwiftContext (std::string auth, std::string account, std::string user, std::string passwd, std::string container);

    /**
     * \~french
     * \brief Constructeur pour un contexte Swift, avec les valeur par défaut
     * \details Les valeurs sont récupérées dans les variables d'environnement ou sont celles par défaut
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variable d'environnement</TH><TH>Valeur par défaut</TH>
     * <TR><TD>auth_url</TD><TD>ROK4_SWIFT_AUTHURL</TD><TD>http://localhost:8080/auth/v1.0</TD>
     * <TR><TD>user_account</TD><TD>ROK4_SWIFT_ACCOUNT</TD><TD>test</TD>
     * <TR><TD>user_name</TD><TD>ROK4_SWIFT_USER</TD><TD>tester</TD>
     * <TR><TD>user_passwd</TD><TD>ROK4_SWIFT_PASSWD</TD><TD>password</TD>
     * </TABLE>
     * \param[in] container Conteneur avec lequel on veut communiquer
     * \~english
     * \brief Constructor for Swift context, with default value
     * \details Values are read in environment variables, or are deulat one
     * <TABLE>
     * <TR><TH>Attribute</TH><TH>Environment variables</TH><TH>Default value</TH>
     * <TR><TD>auth_url</TD><TD>ROK4_SWIFT_AUTHURL</TD><TD>http://localhost:8080/auth/v1.0</TD>
     * <TR><TD>user_account</TD><TD>ROK4_SWIFT_ACCOUNT</TD><TD>test</TD>
     * <TR><TD>user_name</TD><TD>ROK4_SWIFT_USER</TD><TD>tester</TD>
     * <TR><TD>user_passwd</TD><TD>ROK4_SWIFT_PASSWD</TD><TD>password</TD>
     * </TABLE>
     * \param[in] container Container to use
     */
    SwiftContext (std::string container);


    eContextType getType();
    std::string getTypeStr();
    std::string getBucket();
        
    /**
     * \~french \brief Retourne le nom du conteneur
     * \~english \brief Return the name of container
     */
    std::string getContainerName () {
        return container_name;
    }

    /**
     * \~french \brief Retourne l'URL d'authentification
     * \~english \brief Return the authentication URL
     */
    std::string getAuthUrl () {
        return auth_url;
    }
    /**
     * \~french \brief Retourne l'accompte
     * \~english \brief Return the account
     */
    std::string getAccount () {
        return user_account;
    }
    /**
     * \~french \brief Retourne le nom de l'utilisateur
     * \~english \brief Return the name of user
     */
    std::string getUserName () {
        return user_name;
    }

    /**
     * \~french \brief Retourne le mot de passe
     * \~english \brief Return the password
     */
    std::string getUserPwd () {
        return user_passwd;
    }
    
    bool exists(std::string name) {
        // TODO : Implémenter l'existance d'un objet dans swift
        return true;
    }
    int read(uint8_t* data, int offset, int size, std::string name);

    /**
     * \~french
     * \brief Écrit de la donnée dans un objet Swift
     * \warning Pas d'implémentation de l'écriture depuis un buffer, retourne systématiquement une erreur
     */
    bool write(uint8_t* data, int offset, int size, std::string name) {
        LOGGER_ERROR("Can't write a Swift object from buffer");
        return false;
    }
    /**
     * \~french
     * \brief Écrit un objet Swift
     * \warning Pas d'implémentation de l'écriture depuis un buffer, retourne systématiquement une erreur
     */
    bool writeFull(uint8_t* data, int size, std::string name) {
        LOGGER_ERROR("Can't write a Swift object from buffer");
        return false;
    }

    /**
     * \~french
     * \brief Écrit un objet Swift depuis un fichier
     * \param[in] fileName Nom du fichier à téléverser dans Swift
     * \param[in] objectName Nom de l'objet Swift
     * \~english
     * \brief Écrit un objet Swift depuis un fichier
     * \param[in] fileName File path to upload into Swift
     * \param[in] objectName Swift object's name
     */
    bool writeFromFile(std::string fileName, std::string objectName);

    virtual bool openToWrite(std::string name) {return true;}
    virtual bool closeToWrite(std::string name) {return true;}


    virtual void print() {
        LOGGER_INFO ( "------ Swift Context -------" );
        LOGGER_INFO ( "\t- user account = " << user_account );
        LOGGER_INFO ( "\t- user name = " << user_name );
        LOGGER_INFO ( "\t- container name = " << container_name );
    }

    virtual std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Swift Context -------" << std::endl;
        oss << "\t- user account = " << user_account << std::endl;
        oss << "\t- user name = " << user_name << std::endl;
        oss << "\t- container name = " << container_name << std::endl;
        if (connected) {
            oss << "\t- CONNECTED !" << std::endl;
        } else {
            oss << "\t- NOT CONNECTED !" << std::endl;
        }
        return oss.str() ;
    }
    
    /**
     * \~french \brief Récupère l'URL publique #public_url et constitue l'en-tête HTTP #authHdr
     * \~english \brief Get public URL #public_url and constitute the HTTP header #authHdr
     */
    bool connection();

    void closeConnection() {
        connected = false;
    }
    
    virtual ~SwiftContext() {

    }
};

#endif
