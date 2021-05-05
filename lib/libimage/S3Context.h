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
 * \file S3Context.h
 ** \~french
 * \brief Définition de la classe S3Context
 * \details
 * \li S3Context : connexion à un container S3
 ** \~english
 * \brief Define classe S3Context
 * \details
 * \li S3Context : S3 container connection
 */

#ifndef S3_CONTEXT_H
#define S3_CONTEXT_H

#include <curl/curl.h>
#include <boost/log/trivial.hpp>
#include "Context.h"
#include "LibcurlStruct.h"

#define ROK4_S3_URL "ROK4_S3_URL"
#define ROK4_S3_KEY "ROK4_S3_KEY"
#define ROK4_S3_SECRETKEY "ROK4_S3_SECRETKEY"

#ifndef ROK4_SSL_NO_VERIFY
#define ROK4_SSL_NO_VERIFY "ROK4_SSL_NO_VERIFY"
#endif

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte S3 (connexion à un cluster + bucket particulier), pour pouvoir récupérer des données stockées sous forme d'objets
 */
class S3Context : public Context {
    
private:
    
    /**
     * \~french \brief URL de l'API S3, avec protocole et port
     * \~english \brief S3 API URL, with protocol and port
     */
    std::string url;

    /**
     * \~french \brief Hôte de l'API S3 (URL sans protocole ni port)
     * \~english \brief S3 API Host (URL without protocol or port)
     */
    std::string host;
    /**
     * \~french \brief Clé S3
     * \~english \brief S3 key
     */
    std::string key;
    /**
     * \~french \brief Clé de hachage S3
     * \~english \brief S3 hash key
     */
    std::string secret_key;

    /**
     * \~french \brief Nom du conteneur S3
     * \~english \brief S3 container name
     */
    std::string bucket_name;
    
    /**
     * \~french \brief  Ne pas vérifier les certificats SSL avec Curl?
     * \~english \brief Don't verify SSL certificats with Curl ?
     */
    bool ssl_no_verify;

    /**
     * \~french \brief Calcule la signature à partir du header
     * \~english \brief Calculate header's signature
     */
    std::string getAuthorizationHeader(std::string toSign);

public:

    /**
     * \~french
     * \brief Constructeur pour un contexte S3, avec les valeur par défaut
     * \details Les valeurs sont récupérées dans les variables d'environnement ou sont celles par défaut
     * <TABLE>
     * <TR><TH>Attribut</TH><TH>Variable d'environnement</TH><TH>Valeur par défaut</TH>
     * <TR><TD>url</TD><TD>ROK4_S3_URL</TD><TD>localhost:8080</TD>
     * <TR><TD>key</TD><TD>ROK4_S3_KEY</TD><TD>KEY</TD>
     * <TR><TD>secret_key</TD><TD>ROK4_S3_SECRETKEY</TD><TD>SECRETKEY</TD>
     * <TR><TD>ssl_no_verify</TD><TD>ROK4_SSL_NO_VERIFY</TD><TD>false</TD>
     * </TABLE>
     * \param[in] b Bucket avec lequel on veut communiquer
     * \~english
     * \brief Constructor for S3 context, with default value
     * \details Values are read in environment variables, or are deulat one
     * <TABLE>
     * <TR><TH>Attribute</TH><TH>Environment variables</TH><TH>Default value</TH>
     * <TR><TD>url</TD><TD>ROK4_S3_URL</TD><TD>localhost:8080</TD>
     * <TR><TD>key</TD><TD>ROK4_S3_KEY</TD><TD>KEY</TD>
     * <TR><TD>secret_key</TD><TD>ROK4_S3_SECRETKEY</TD><TD>SECRETKEY</TD>
     * <TR><TD>ssl_no_verify</TD><TD>ROK4_SSL_NO_VERIFY</TD><TD>false</TD>
     * </TABLE>
     * \param[in] b Bucket to use
     */
    S3Context (std::string b);


    ContextType::eContextType getType();
    std::string getTypeStr();
    std::string getTray();
        
    /**
     * \~french \brief Retourne le nom du bucket
     * \~english \brief Return the name of bucket
     */
    std::string getBucketName () {
        return bucket_name;
    }

    /**
     * \~french
     * \brief Lit de la donnée depuis un objet S3
     * \~english 
     * \brief read data from S3 object
     */
    int read(uint8_t* data, int offset, int size, std::string name);

    /**
     * \~french
     * \brief Écrit de la donnée dans un objet S3
     * \details Les données sont en réalité écrites dans #writingBuffer et seront envoyées dans S3 lors de l'appel à #closeToWrite 
     * \~english
     * \brief Write data to  S3 object
     * \details Datas are written to #writingBuffer and send at #closeToWrite call
     */
    bool write(uint8_t* data, int offset, int size, std::string name);

    /**
     * \~french
     * \brief Écrit un objet S3
     * \details Les données sont en réalité écrites dans #writingBuffer et seront envoyées dans S3 lors de l'appel à #closeToWrite
     * \~english
     * \brief Write S3 object
     * \details Datas are written to #writingBuffer and send at #closeToWrite call
     */
    bool writeFull(uint8_t* data, int size, std::string name);

    virtual bool openToWrite(std::string name);
    virtual bool closeToWrite(std::string name);

    std::string getPath(std::string racine,int x,int y,int pathDepth);

    virtual void print() {
        BOOST_LOG_TRIVIAL(info) <<  "------ S3 Context -------" ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- URL = " << url ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Key = " << key ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Secrete Key = " << secret_key ;
        BOOST_LOG_TRIVIAL(info) <<  "\t- Bucket name = " << bucket_name ;
    }

    virtual std::string toString() {
        std::ostringstream oss;
        oss.setf ( std::ios::fixed,std::ios::floatfield );
        oss << "------ S3 Context -------" << std::endl;
        oss << "\t- URL = " << url << std::endl;
        oss << "\t- Key = " << key << std::endl;
        oss << "\t- Secrete Key = " << secret_key << std::endl;
        oss << "\t- Bucket name = " << bucket_name << std::endl;
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
    
    virtual ~S3Context() {
        closeConnection();
    }
};

#endif
