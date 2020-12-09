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


#define ROK4_SWIFT_AUTHURL "ROK4_SWIFT_AUTHURL"
#define ROK4_SWIFT_USER "ROK4_SWIFT_USER"
#define ROK4_SWIFT_PASSWD "ROK4_SWIFT_PASSWD"
#define ROK4_KEYSTONE_DOMAINID "ROK4_KEYSTONE_DOMAINID"
#define ROK4_KEYSTONE_PROJECTID "ROK4_KEYSTONE_PROJECTID"
#define ROK4_SWIFT_PUBLICURL "ROK4_SWIFT_PUBLICURL"
#define ROK4_SWIFT_ACCOUNT "ROK4_SWIFT_ACCOUNT"

#ifndef ROK4_SSL_NO_VERIFY
#define ROK4_SSL_NO_VERIFY "ROK4_SSL_NO_VERIFY"
#endif
/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte Swift (connexion à un cluster + conteneur particulier), pour pouvoir récupérer des données stockées sous forme d'objets
 */
class SwiftContext : public Context{
    
private:

    /**
     * \~french \brief URL d'authentification à Swift
     * \~english \brief Authentication URL to Swift
     */
    std::string auth_url;
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
     * \~french \brief Est ce que l'authentification se fait via keystone ?
     * \~english \brief Keystone authentication ?
     */
    bool keystone_auth;

    /**
     * \~french \brief ID de domaine, pour une authentification Keystone
     * \details Récupéré via la variable d'environnement ROK4_KEYSTONE_DOMAINID
     * \~english \brief Domain ID, for keystone authentication
     */
    std::string domain_id;
    /**
     * \~french \brief ID de projet, pour une authentification Keystone
     * \details Récupéré via la variable d'environnement ROK4_KEYSTONE_PROJECTID
     * \~english \brief Project ID, for keystone authentication
     */
    std::string project_id;

    /**
     * \~french \brief Accompte Swift, pour une authentification Swift
     * \details Récupéré via la variable d'environnement ROK4_SWIFT_ACCOUNT
     * \~english \brief Swift account, for swift authentication
     */
    std::string user_account;
    
    /**
     * \~french \brief URL de communication avec Swift
     * \details
     * \li Authentification keystone : récupéré via la variable d'environnement ROK4_SWIFT_PUBLICURL
     * \li Authentification Swift : récupéré dans le header de la réponse à la requête de connexion
     * \~english \brief Communication URL with Swift
     */
    std::string public_url;

    /**
     * \~french \brief Nom du conteneur Swift
     * \~english \brief Swift container name
     */
    std::string container_name;

    /**
     * \~french \brief Token à utiliser pour chaque échange avec Swift
     * \~english \brief Token to use to communicate with Swift
     */
    std::string token;

    /**
     * \~french \brief  Ne pas vérifier les certificats SSL avec Curl?
     * \~english \brief Don't verify SSL certificats with Curl ?
     */
    bool ssl_no_verify;


public:

    /**
     * \~french
     * \brief Constructeur pour un contexte Swift, avec les valeur par défaut
     * \details Les valeurs sont récupérées dans les variables d'environnement ou sont celles par défaut
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variable d'environnement</TH><TH>Valeur par défaut</TH>
     * <TR><TD>auth_url</TD><TD>ROK4_SWIFT_AUTHURL</TD><TD>http://localhost:8080/auth/v1.0</TD>
     * <TR><TD>user_name</TD><TD>ROK4_SWIFT_USER</TD><TD>tester</TD>
     * <TR><TD>user_passwd</TD><TD>ROK4_SWIFT_PASSWD</TD><TD>password</TD>
     * <TR><TD>domain_id</TD><TD>ROK4_KEYSTONE_DOMAINID</TD><TD></TD>
     * <TR><TD>ssl_no_verify</TD><TD>ROK4_SSL_NO_VERIFY</TD><TD>false</TD>
     * </TABLE>
     * \~english
     * \brief Constructor for Swift context, with default value
     * \details Values are read in environment variables, or are deulat one
     * <TABLE>
     * <TR><TH>Attribute</TH><TH>Environment variables</TH><TH>Default value</TH>
     * <TR><TD>auth_url</TD><TD>ROK4_SWIFT_AUTHURL</TD><TD>http://localhost:8080/auth/v1.0</TD>
     * <TR><TD>user_name</TD><TD>ROK4_SWIFT_USER</TD><TD>tester</TD>
     * <TR><TD>user_passwd</TD><TD>ROK4_SWIFT_PASSWD</TD><TD>password</TD>
     * <TR><TD>domain_id</TD><TD>ROK4_KEYSTONE_DOMAINID</TD><TD></TD>
     * <TR><TD>ssl_no_verify</TD><TD>ROK4_SSL_NO_VERIFY</TD><TD>false</TD>
     * </TABLE>
     */
    SwiftContext (std::string cont);

    eContextType getType();
    std::string getTypeStr();
    std::string getTray();
          

    /**
     * \~french
     * \brief Lit de la donnée depuis un objet Swift
     * \~english
     * \brief Read data from Swift object
     */

    int read(uint8_t* data, int offset, int size, std::string name);

    /**
     * \~french
     * \brief Écrit de la donnée dans un objet Swift
     * \details Les données sont en réalité écrites dans #writingBuffer et seront envoyées dans Swift lors de l'appel à #closeToWrite
     * \~english
     * \brief Write data to  Swift object
     * \details Datas are written to #writingBuffer and send at #closeToWrite call
     */
    bool write(uint8_t* data, int offset, int size, std::string name);

    /**
     * \~french
     * \brief Écrit un objet Swift
     * \details Les données sont en réalité écrites dans #writingBuffer et seront envoyées dans Swift lors de l'appel à #closeToWrite
     * \~english
     * \brief Write Swift object
     * \details Datas are written to #writingBuffer and send at #closeToWrite call
     */
    bool writeFull(uint8_t* data, int size, std::string name);

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

    virtual bool openToWrite(std::string name);
    virtual bool closeToWrite(std::string name);



    std::string getPath(std::string racine,int x,int y,int pathDepth);

    virtual void print() {
        LOGGER_INFO ( "------ Swift Context -------" );
        LOGGER_INFO ( "\t- container name = " << container_name );
        LOGGER_INFO ( "\t- token = " << token );
    }

    virtual std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ Swift Context -------" << std::endl;
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
     * \~french \brief Récupère le token #token
     * \details 
     * \li Pour une authentification keystone
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variables d'environnement</TH>
     * <TR><TD>domain_id</TD><TD>ROK4_KEYSTONE_DOMAINID</TD>
     * <TR><TD>project_id</TD><TD>ROK4_KEYSTONE_PROJECTID</TD><TD>false</TD>
     * <TR><TD>public_url</TD><TD>ROK4_SWIFT_PUBLICURL</TD>
     * </TABLE>
     * \li Pour une authentification Swift
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variables d'environnement</TH>
     * <TR><TD>user_account</TD><TD>ROK4_SWIFT_ACCOUNT</TD>
     * </TABLE>
     * \~english \brief Get token #token
     */
    bool connection();

    void closeConnection() {
        connected = false;
    }
    
    virtual ~SwiftContext() {

    }
};

#endif
