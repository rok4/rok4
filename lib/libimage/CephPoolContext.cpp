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
}

bool CephPoolContext::connection() {


    uint64_t flags;
    int ret = 0;

    ret = rados_create2(&cluster, cluster_name.c_str(), user_name.c_str(), flags);
    if (ret < 0) {
        LOGGER_ERROR("Couldn't initialize the cluster handle! error " << ret);
        return false;
    }

    ret = rados_conf_read_file(cluster, conf_file.c_str());
    if (ret < 0) {
        LOGGER_ERROR( "Couldn't read the Ceph configuration file! error " << ret );
        LOGGER_ERROR (strerror(-ret));
        LOGGER_ERROR( "Configuration file : " << conf_file );
        return false;
    }

    // On met les timeout à 10 minutes
    rados_conf_set(cluster, "client_mount_timeout", "60");
    rados_conf_set(cluster, "rados_mon_op_timeout", "60");
    rados_conf_set(cluster, "rados_osd_op_timeout", "60");

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

    connected = true;

    return true;
}

int CephPoolContext::read(uint8_t* data, int offset, int size, std::string name) {
   
    LOGGER_DEBUG("Ceph read : " << size << " bytes (from the " << offset << " one) in the object " << name);

    if (! connected) {
        LOGGER_ERROR("Try to read using the unconnected ceph pool context " << pool_name);
        return -1;
    }

    int readSize;
    int attempt = 1;
    bool error = false;
    while(attempt <= attempts) {
        readSize = rados_read(io_ctx, name.c_str(), (char*) data, size, offset);

        if (readSize < 0) {
            error = true;
            // Seul le timeout donne lieu à une nouvelle tentative
            if (readSize == -ETIMEDOUT) {
                LOGGER_WARN ( "Try " << attempt << " timed out" );
            } else {
                LOGGER_ERROR ( "Try " << attempt << " failed" );
                LOGGER_ERROR ("Error code: " << readSize );
                LOGGER_ERROR (strerror(-readSize));
                break;
            }
        } else {
            error = false;
            break;
        }

        attempt++;
    }

    if (error) {
        LOGGER_ERROR ( "Unable to read " << size << " bytes (from the " << offset << " one) in the object " << name  << " after " << attempt << " tries" );
    }

    return readSize;
}


bool CephPoolContext::write(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Ceph write : " << size << " bytes (from the " << offset << " one) in the writing buffer " << name);

    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 == writingBuffers.end() ) {
        // pas de buffer pour ce nom d'objet
        LOGGER_ERROR("No writing buffer for the name " << name);
        return false;
    }
   
    // Calcul de la taille finale et redimensionnement éventuel du vector
    if (it1->second->size() < size + offset) {
        it1->second->resize(size + offset);
    }

    memcpy(&((*(it1->second))[0]) + offset, data, size);

    return true;
}

bool CephPoolContext::writeFull(uint8_t* data, int size, std::string name) {
    LOGGER_DEBUG("Ceph write : " << size << " bytes (one shot) in the writing buffer " << name);

    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 == writingBuffers.end() ) {
        // pas de buffer pour ce nom d'objet
        LOGGER_ERROR("No Ceph writing buffer for the name " << name);
        return false;
    }

    it1->second->clear();

    it1->second->resize(size);
    memcpy(&((*(it1->second))[0]), data, size);

    return true;
}

eContextType CephPoolContext::getType() {
    return CEPHCONTEXT;
}

std::string CephPoolContext::getTypeStr() {
    return "CEPHCONTEXT";
}

std::string CephPoolContext::getTray() {
    return pool_name;
}

bool CephPoolContext::openToWrite(std::string name) {

    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 != writingBuffers.end() ) {
        LOGGER_ERROR("A Ceph writing buffer already exists for the name " << name);
        return false;

    } else {
        writingBuffers.insert ( std::pair<std::string,std::vector<char>*>(name, new std::vector<char>()) );
    }

    return true;
}


bool CephPoolContext::closeToWrite(std::string name) {


    std::map<std::string, std::vector<char>*>::iterator it1 = writingBuffers.find ( name );
    if ( it1 == writingBuffers.end() ) {
        LOGGER_ERROR("The Ceph writing buffer with name " << name << "does not exist, cannot flush it");
        return false;
    }


    LOGGER_DEBUG("Write buffered " << it1->second->size() << " bytes in the ceph object " << name);

    int tentative = 1;
    while(tentative <= 10) {
        int err = rados_write_full(io_ctx,name.c_str(), &((*(it1->second))[0]), it1->second->size());
        if (err < 0) {
            LOGGER_WARN ( "Try " << tentative );
            LOGGER_WARN ( "Unable to flush " << it1->second->size() << " bytes in the object " << name );
            LOGGER_WARN (strerror(-err));
        } else {
            break;
        }

        tentative++;
        sleep(60);
    }

    if (tentative == 11) {
        LOGGER_ERROR ( "Unable to write after 10 tries" );
        return false;
    } else {
        LOGGER_DEBUG("Erase the flushed buffer");
        delete it1->second;
        writingBuffers.erase(it1);

        return true;
    }
}
