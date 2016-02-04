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
 * \file CephPoolContext.cpp
 ** \~french
 * \brief Implémentation de la classe CephPoolContext
 * \details
 * \li CephPoolContext : connexion à un pool de données Ceph
 ** \~english
 * \brief Implement classe CephPoolContext
 * \details
 * \li CephPoolContext : Ceph data pool connection
 */

#include "CephPoolContext.h"
#include <stdlib.h>

CephPoolContext::CephPoolContext (std::string cluster, std::string user, std::string conf, std::string pool) : Context(), cluster_name(cluster), user_name(user), conf_file(conf), pool_name(pool) {

    writting_in_progress = false;
}

CephPoolContext::CephPoolContext (std::string pool) : Context(), pool_name(pool) {

    char* cluster = getenv ("ROK4_CEPH_CLUSTERNAME");
    if (cluster == NULL) {
        cluster_name.assign("ceph");
    } else {
        cluster_name.assign(cluster);
    }

    char* user = getenv ("ROK4_CEPH_USERNAME");
    if (user == NULL) {
        user_name.assign("client.admin");
    } else {
        user_name.assign(user);
    }

    char* conf = getenv ("ROK4_CEPH_CONFFILE");
    if (conf == NULL) {
        conf_file.assign("/etc/ceph/ceph.conf");
    } else {
        conf_file.assign(conf);
    }

    writting_in_progress = false;
}

bool CephPoolContext::connection() {
    uint64_t flags;
    int ret = 0;

    rados_t cluster;
    rados_ioctx_t io_ctx;

    ret = rados_create2(&cluster, cluster_name.c_str(), user_name.c_str(), flags);
    if (ret < 0) {
        LOGGER_ERROR("Couldn't initialize the cluster handle! error " << ret);
        return false;
    }

    ret = rados_conf_read_file(cluster, conf_file.c_str());
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't read the Ceph configuration file! error " << ret );
        LOGGER_ERROR( "Configuration file : " << conf_file );
        return false;
    }

    ret = rados_connect(cluster);
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't connect to cluster! error " << ret );
        return false;
    }

    ret = rados_ioctx_create(cluster, pool_name.c_str(), &io_ctx);
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't set up ioctx! error " << ret );
        LOGGER_ERROR( "Pool : " << pool_name );
        return false;
    }
/*
    ret = rados_aio_create_completion(NULL, NULL, NULL, &completion);
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't create completion object. Error: " << ret );
        return false;
    }*/

    connected = true;


    rados_ioctx_destroy(io_ctx);
    rados_shutdown(cluster);

    return true;
}

bool CephPoolContext::read(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Ceph read : " << size << " bytes (from the " << offset << " one) in the object " << name);

    int err = rados_read(io_ctx, name.c_str(), (char*) data, size, offset);

    if (err < 0) {
        LOGGER_ERROR ( "Unable to read " << size << " bytes (from the " << offset << " one) in the object " << name );
        return false;
    }

    return true;
}


bool CephPoolContext::write(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Ceph write : " << size << " bytes (from the " << offset << " one) in the object " << name);
/*
    if (writting_in_progress) {
        rados_aio_wait_for_complete(completion);
    }

    int err = rados_aio_write(io_ctx, name.c_str(), completion, (char*) data, size, offset);
    writting_in_progress = true;
    if (err < 0) {
        LOGGER_ERROR ( "Unable to start to write " << size << " bytes (from the " << offset << " one) in the object " << name );
        return false;
    }*/

    return true;
}

bool CephPoolContext::writeFull(uint8_t* data, int size, std::string name) {
    LOGGER_DEBUG("Ceph write : " << size << " bytes (one shot) in the object " << name);

    int err = rados_write_full(io_ctx,name.c_str(), (char*) data, size);
    if (err < 0) {
        LOGGER_ERROR ( "Unable to write " << size << " bytes (one shot) in the object " << name );
        return false;
    }

    return true;
}
