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
 * \file CephContext.cpp
 ** \~french
 * \brief Implémentation de la classe CephContext
 * \details
 * \li CephContext : connexion à un pool de données Ceph
 ** \~english
 * \brief Implement classe CephContext
 * \details
 * \li CephContext : Ceph data pool connection
 */

#include "CephContext.h"
#include "Ceph_library_config.h"
#include "Logger.h"



CephContext::CephContext (char* pool) {
    cluster_name = new char[255];
    user_name = new char[255];
    conf_file = new char[255];
    pool_name = new char[255];
    
    strcpy ( cluster_name,CEPH_CLUSTER_NAME );
    strcpy ( user_name,CEPH_USER_NAME );
    strcpy ( conf_file,CEPH_CONF_FILE );
    strcpy ( pool_name,pool );
}

bool CephContext::connection() {
    uint64_t flags;
    int ret = 0;
    ret = cluster.init2(user_name, cluster_name, flags);
    if (ret < 0) {
        LOGGER_ERROR("Couldn't initialize the cluster handle! error " << ret);
        LOGGER_ERROR( "User name : " << user_name );
        LOGGER_ERROR( "Cluster name : " << cluster_name );
        return false;
    }
    
    ret = cluster.conf_read_file(conf_file);
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't read the Ceph configuration file! error " << ret );
        LOGGER_ERROR( "Configuration file : " << conf_file );
        return false;
    }
    
    ret = cluster.connect();
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't connect to cluster! error " << ret );
        return false;
    }
    
    ret = cluster.ioctx_create(pool_name, io_ctx);
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't set up ioctx! error " << ret );
        LOGGER_ERROR( "Pool : " << pool_name );
        return false;
    }
    
    return true;
}

bool CephContext::readFromCephObject(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Ceph read : " << size << " bytes (from the " << offset << " one) in the object " << name);
    librados::bufferlist bl;
    int ret = io_ctx.read(name, bl, size, offset);
    if (ret < 0) {
        LOGGER_ERROR ( "Unable to read " << size << " bytes (from the " << offset << " one) in the object " << name );
        return false;
    }
    memcpy(data, bl.get_contiguous(0, bl.length()), bl.length());
    
    return true;
}


bool CephContext::writeToCephObject(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Ceph write : " << size << " bytes (from the " << offset << " one) in the object " << name);
    librados::bufferlist bl;
    bl.append((char*) data, size);
    
    int ret = io_ctx.write(name, bl, size, offset);
    if (ret < 0) {
        LOGGER_ERROR ( "Unable to write " << size << " bytes (from the " << offset << " one) in the object " << name );
        return false;
    }
    
    return true;
}
