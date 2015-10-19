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
 * \file CephContext.h
 ** \~french
 * \brief Définition de la classe CephContext
 * \details
 * \li CephContext : connexion à un pool de données Ceph
 ** \~english
 * \brief Define classe CephContext
 * \details
 * \li CephContext : Ceph data pool connection
 */

#ifndef CEPH_CONTEXT_H
#define CEPH_CONTEXT_H

#include <rados/librados.hpp>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * \brief Création d'un contexte Ceph (connexion à un cluster + pool particulier), pour pouvoir récupérer des données stockées sous forme d'objets
 */
class CephContext {
    
private:
    
    char* cluster_name;
    
    char* user_name;
    
    char* conf_file;
    
    char* pool_name;
    
    librados::Rados cluster;
    librados::IoCtx io_ctx;

public:

    /** Constructeurs */
    CephContext (char* pool);
    
    librados::IoCtx getContext () {
        return io_ctx;
    }
    
    char* getPoolName () {
        return pool_name;
    }
    
    bool readFromCephObject(uint8_t* data, int offset, int size, std::string name);
    bool writeToCephObject(uint8_t* data, int offset, int size, std::string name);
    
    bool connection();
    
    virtual ~CephContext() {
        io_ctx.close();
        cluster.shutdown();
        
        delete[] cluster_name;
        delete[] user_name;
        delete[] conf_file;
        delete[] pool_name;
    };
};

#endif
