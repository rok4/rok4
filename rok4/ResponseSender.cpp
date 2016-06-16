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
 * \file ResponseSender.cpp
 * \~french
 * \brief Implémentation des fonctions d'envoie de réponse sur le flux FCGI
 * \~english
 * \brief Implement response sender function for the FCGI link
 */

#include "ResponseSender.h"
#include "ServiceException.h"
#include "Message.h"
#include <iostream>
#include "Logger.h"
#include <stdio.h>
#include <string.h> // pour strlen
#include <sstream> // pour les stringstream
#include "intl.h"
#include "config.h"
/**
 * \~french
 * \brief Méthode commune pour générer l'en-tête HTTP en fonction du status code HTTP
 * \param[in] statusCode Code de status HTTP
 * \return élément status de l'en-tête HTTP
 * \~english
 * \brief Common function to generate HTTP headers using the HTTP status code
 * \param[in] statusCode HTTP status code
 * \return HTTP header status element
 */
std::string genStatusHeader ( int statusCode ) {
    // Creation de l'en-tete
    std::stringstream out;
    out << statusCode;
    std::string statusHeader= "Status: "+out.str() +" "+ServiceException::getStatusCodeAsReasonPhrase ( statusCode ) +"\r\n" ;
    return statusHeader ;
}

/**
 * \~french
 * \brief Méthode commune pour générer le nom du fichier en fonction du type mime
 * \param[in] mime type mime
 * \return nom du fichier
 * \~english
 * \brief Common function to generate file name using the mime type
 * \param[in] mime mime type
 * \return filename
 */
std::string genFileName ( std::string mime ) {
    if ( mime.compare ( "image/tiff" ) ==0 )
        return "image.tif";
    else if ( mime.compare ( "image/jpeg" ) ==0 )
        return "image.jpg";
    else if ( mime.compare ( "image/png" ) ==0 )
        return "image.png";
    else if ( mime.compare ( "image/x-bil;bits=32" ) ==0 )
        return "image.bil";
    else if ( mime.compare ( "text/plain" ) ==0 )
        return "message.txt";
    else if ( mime.compare ( "text/xml" ) ==0 )
        return "message.xml";
    return "file";
}

/**
 * \~french
 * \brief Méthode commune pour afficher les codes d'erreur FCGI
 * \param[in] error code d'erreur
 * \~english
 * \brief Common function to display FCGI error code
 * \param[in] error error code
 */
void displayFCGIError ( int error ) {
    if ( error>0 )
        LOGGER_ERROR ( _ ( "Code erreur : " ) <<error ); // Erreur errno(2) (Cf. manpage )
    else if ( error==FCGX_UNSUPPORTED_VERSION )
        LOGGER_ERROR ( _ ( "Version FCGI non supportee" ) );
    else if ( error==FCGX_UNSUPPORTED_VERSION )
        LOGGER_ERROR ( _ ( "Erreur de protocole" ) );
    else if ( error==FCGX_CALL_SEQ_ERROR )
        LOGGER_ERROR ( _ ( "Erreur de parametre" ) );
    else if ( error==FCGX_UNSUPPORTED_VERSION )
        LOGGER_ERROR ( _ ( "Preconditions non remplies" ) );
    else
        LOGGER_ERROR ( _ ( "Erreur inconnue" ) );
}

int ResponseSender::sendresponse ( DataSource* source, FCGX_Request* request ) {
    // Creation de l'en-tete
    std::string statusHeader = genStatusHeader ( source->getHttpStatus() );
    std::string filename = genFileName ( source->getType() );
    LOGGER_DEBUG ( filename );
    FCGX_PutStr ( statusHeader.data(),statusHeader.size(),request->out );
    FCGX_PutStr ( "Content-Type: ",14,request->out );
    FCGX_PutStr ( source->getType().c_str(), strlen ( source->getType().c_str() ),request->out );
    if ( !source->getEncoding().empty() ){
        FCGX_PutStr ( "\r\nContent-Encoding: ",20,request->out );
        FCGX_PutStr ( source->getEncoding().c_str(), strlen ( source->getEncoding().c_str() ),request->out );
    }
    FCGX_PutStr ( "\r\nContent-Disposition: filename=\"",33,request->out );
    FCGX_PutStr ( filename.data(),filename.size(), request->out );
    FCGX_PutStr ( "\"",1,request->out );
    FCGX_PutStr ( "\r\n\r\n",4,request->out );

    // Copie dans le flux de sortie
    size_t buffer_size;
    const uint8_t *buffer = source->getData ( buffer_size );
    int wr = 0;
    // Ecriture iterative de la source de donnees dans le flux de sortie
    while ( wr < buffer_size ) {
        // Taille ecrite dans le flux de sortie
        int w = FCGX_PutStr ( ( char* ) ( buffer + wr ), buffer_size,request->out );
        if ( w < 0 ) {
            LOGGER_ERROR ( _ ( "Echec d'ecriture dans le flux de sortie de la requete FCGI " ) << request->requestId );
            displayFCGIError ( FCGX_GetError ( request->out ) );
            delete source;
            //delete[] buffer;
            return -1;
        }
        wr += w;
    }
    delete source;
    LOGGER_DEBUG ( _ ( "End of Response" ) );
    return 0;
}

int ResponseSender::sendresponse ( DataStream* stream, FCGX_Request* request ) {
    // Creation de l'en-tete
    std::string statusHeader= genStatusHeader ( stream->getHttpStatus() );
    std::string filename = genFileName ( stream->getType() );
    LOGGER_DEBUG ( filename );
    FCGX_PutStr ( statusHeader.data(),statusHeader.size(),request->out );
    FCGX_PutStr ( "Content-Type: ",14,request->out );
    FCGX_PutStr ( stream->getType().c_str(), strlen ( stream->getType().c_str() ),request->out );
    FCGX_PutStr ( "\r\nContent-Disposition: filename=\"",33,request->out );
    FCGX_PutStr ( filename.data(),filename.size(), request->out );
    FCGX_PutStr ( "\"",1,request->out );
    FCGX_PutStr ( "\r\n\r\n",4,request->out );
    // Copie dans le flux de sortie
    uint8_t *buffer = new uint8_t[2 << 20];
    size_t size_to_read = 2 << 20;
    int pos = 0;

    // Ecriture progressive du flux d'entree dans le flux de sortie
    while ( true ) {
        // Recuperation d'une portion du flux d'entree

        size_t read_size = stream->read ( buffer, size_to_read );
        if ( read_size==0 )
            break;
        int wr = 0;
        // Ecriture iterative de la portion du flux d'entree dans le flux de sortie
        while ( wr < read_size ) {
            // Taille ecrite dans le flux de sortie
            int w = FCGX_PutStr ( ( char* ) ( buffer + wr ), read_size,request->out );
            if ( w < 0 ) {
                LOGGER_ERROR ( _ ( "Echec d'ecriture dans le flux de sortie de la requete FCGI " ) << request->requestId );
                displayFCGIError ( FCGX_GetError ( request->out ) );
                delete stream;
                delete[] buffer;
                return -1;
            }
            wr += w;
        }
        if ( wr != read_size ) {
            LOGGER_DEBUG ( _ ( "Nombre incorrect d'octets ecrits dans le flux de sortie" ) );
            delete stream;
            stream = 0;
            delete[] buffer;
            buffer = 0;
            break;
        }
        pos += read_size;
    }
    if ( stream ) {
        delete stream;
    }
    if ( buffer ) {
        delete[] buffer;
    }
    LOGGER_DEBUG ( _ ( "End of Response" ) );
    return 0;
}
