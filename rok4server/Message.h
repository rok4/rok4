/*
 * Copyright © (2011-2013) Institut national de l'information
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
 * \file Message.h
 * \~french
 * \brief Définition des classes gérant les messages utilisateurs
 * \~english
 * \brief Define classes handling user messages
 */

#ifndef _MESSAGE_
#define _MESSAGE_

#include "Data.h"
#include <string.h> // Pour memcpy
#include "ServiceException.h"
#include <vector>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance de MessageDataSource définit un message renvoyé à l'utilisateur.
 * Le message est renvoyé comme un fichier.
 * \brief Gestion des messages sous forme de fichier
 * \~english
 * A MessageDataSource defines a message sent to the user.
 * The message is sent as a file.
 * \brief File messages handler
 * \~ \see MessageDataStream
 */
class MessageDataSource : public DataSource {
private:
    /**
     * \~french Type MIME du message
     * \~english MIME type
     */
    std::string type;
protected:
    /**
     * \~french Message à envoyer
     * \~english Sent message
     */
    std::string message;
public:
    /**
     * \~french
     * \brief Crée un MessageDataSource à partir de ses éléments constitutifs
     * \param[in] message contenu du message
     * \param[in] type type MIME
     * \~english
     * \param[in] message message content
     * \param[in] type MIME type
     */
    MessageDataSource ( std::string message, std::string type ) : message ( message ), type ( type ) {}
    const uint8_t* getData ( size_t& size ) {
        size=message.length();
        return ( const uint8_t* ) message.data();
    }
    /**
     * \~french
     * \brief Retourne le code de retour HTTP associé au message
     * \return code
     * \~english
     * \brief Return the associated HTTP return code
     * \return code
     */
    int getHttpStatus() {
        return 200;
    }
    /**
     * \~french
     * \brief Retourne le type MIME du message
     * \return type
     * \~english
     * \brief Return the message's MIME type
     * \return type
     */
    std::string getType() {
        return type.c_str();
    }
    /**
     * \~french
     * \brief Retourne l'encodage message
     * \return type
     * \~english
     * \brief Return the message's encoding
     * \return type
     */
    std::string getEncoding() {
        return "";
    }
    /**
     * \~french
     * \brief Libère les données mémoire allouées.
     * \return true en cas de succès
     * \~english
     * \brief Free the allocated memories
     * \return true if successful
     * \~ \see libimage : DataSource
     */
    bool releaseData() {}
    
    unsigned int getLength(){
            return message.length();
    }
};

/**
 * @class SERDataSource
 * genere un MessageDataSource a partir de ServiceExceptions
 */
class SERDataSource : public MessageDataSource {
private:
    int httpStatus ;
public:
    /**
        * Constructeur à partir d'un ServiceException
        * @param sex le ServiceException
        */
    SERDataSource ( ServiceException *sex ) ;
    /**
        * Constructeur à partir d'un tableau de ServiceException
        * @param sexcp le tableau des ServiceExceptions
        */
    SERDataSource ( std::vector<ServiceException*> *sexcp ) ;
    /**
     * getter pour la propriete Message
     */
    std::string getMessage() {
        return this->message;
    } ;
    /**
     * getter pour la propriete httpStatus
     */
    int getHttpStatus() {
        return this->httpStatus;
    }
};

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance de MessageDataStream définit un message renvoyé à l'utilisateur.
 * Le message est renvoyé sous forme de flux
 * \brief Gestion des messages sous forme de flux
 * \~english
 * A MessageDataStream defines a message sent to the user.
 * \brief Streamed messages handler
 * \~ \see MessageDataSource
 */
class MessageDataStream : public DataStream {
private:
    /**
     * \~french Type MIME du message
     * \~english MIME type
     */
    std::string type;
    
    /**
     * \~french Position courante dans le flux
     * \~english Current stream position
     */
    uint32_t pos;
protected:
    /**
     * \~french Message à envoyer
     * \~english Sended message
     */
    std::string message;

public:
    MessageDataStream ( std::string message, std::string type ) : message ( message ), type ( type ), pos ( 0 ) {}

    size_t read ( uint8_t *buffer, size_t size ) {
        if ( size > message.length() - pos ) size = message.length() - pos;
        memcpy ( buffer, ( uint8_t* ) ( message.data() +pos ),size );
        pos+=size;
        return size;
    }
    bool eof() {
        return ( pos==message.length() );
    }
    std::string getType() {
        return type.c_str();
    }
    std::string getEncoding() {
        return "";
    }
    int getHttpStatus() {
        return 200;
    }
    unsigned int getLength(){
        return message.length();
    }
};

/**
 * @class SERDataStream
 * genere un MessageDataStream a partir de ServiceExceptions
 */
class SERDataStream : public MessageDataStream {
private:
    int httpStatus ;
public:
    /**
        * Constructeur à partir d'un ServiceException
        * @param sex le ServiceException
        */
    SERDataStream ( ServiceException *sex ) ;
    /**
        * Constructeur à partir d'un tableau de ServiceException
        * @param sexcp le tableau des ServiceExceptions
        */
    SERDataStream ( std::vector<ServiceException*> *sexcp ) ;
    /**
     * getter pour la propriete Message
     */
    std::string getMessage() {
        return this->message;
    } ;
    /**
     * getter pour la propriete httpStatus
     */
    int getHttpStatus() {
        return this->httpStatus;
    }
};

#endif
