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
 * \file ResponseSender.h
 * \~french
 * \brief Définition des fonctions d'envoie de réponse sur le flux FCGI
 * \~english
 * \brief Define response sender function for the FCGI link
 */

#ifndef _SENDER_
#define _SENDER_

#include "Data.h"
#include "fcgiapp.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Gestions de l'envoie des réponse dans le flux FCGI
 * \~english
 * \brief FCGI response handler
 */
class ResponseSender {
public:
    /**
     * \~french
     * \brief Copie d'une source de données dans le flux de sortie de l'objet request de type FCGX_Request
     * \return -1 en cas de problème, 0 sinon
     * \~english
     * \brief Copy a data source in the FCGX_Request output stream
     * \return -1 if error, else 0
     */
    int sendresponse ( DataSource* response, FCGX_Request* request );
    /**
     * \~french
     * \brief Copie d'un flux d'entree dans le flux de sortie de l'objet request de type FCGX_Request
     * \return -1 en cas de problème, 0 sinon
     * \~english
     * \brief Copy a data stream in the FCGX_Request output stream
     * \return -1 if error, else 0
     */
    int sendresponse ( DataStream* response, FCGX_Request* request );
};


#endif

