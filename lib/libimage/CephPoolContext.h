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
 * \file CephPoolContext.h
 ** \~french
 * \brief Définition de la classe CephPoolContext
 * \details
 * \li CephPoolContext : connexion à un pool de données Ceph
 ** \~english
 * \brief Define classe CephPoolContext
 * \details
 * \li CephPoolContext : Ceph data pool connection
 */

#ifndef CEPH_POOL_CONTEXT_H
#define CEPH_POOL_CONTEXT_H

#include <rados/librados.h>
#include "Logger.h"
#include "Context.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte Ceph (connexion à un cluster + pool particulier), pour pouvoir récupérer des données stockées sous forme d'objets
 */
class CephPoolContext : public Context {
    
private:
    
    std::string cluster_name;
    
    std::string user_name;
    
    std::string conf_file;
    
    std::string pool_name;

    rados_t cluster;
    rados_ioctx_t io_ctx;

public:

    /** Constructeurs */
    CephPoolContext (std::string cluster, std::string user, std::string conf, std::string pool);
    CephPoolContext (std::string pool);
    eContextType getType();

    std::string getPoolName () {
        return pool_name;
    }
    
    bool read(uint8_t* data, int offset, int size, std::string name);
    bool write(uint8_t* data, int offset, int size, std::string name);
    bool writeFull(uint8_t* data, int size, std::string name);

    virtual bool openToWrite(std::string name) {return true;}
    virtual bool closeToWrite(std::string name) {return true;}
    
    bool connection();

    /**
     * \~french
     * \brief Sortie des informations sur le contexte Ceph
     * \~english
     * \brief Ceph context description output
     */
    virtual void print() {
        LOGGER_INFO ( "------ Ceph Context -------" );
        LOGGER_INFO ( "\t- cluster name = " << cluster_name );
        LOGGER_INFO ( "\t- user name = " << user_name );
        LOGGER_INFO ( "\t- configuration file = " << conf_file );
        LOGGER_INFO ( "\t- pool name = " << pool_name );
    }
    
    virtual ~CephPoolContext() {
        rados_aio_flush(io_ctx);
        rados_ioctx_destroy(io_ctx);
        rados_shutdown(cluster);
    }
};

#endif
