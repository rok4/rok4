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
 * \file SwiftContext.cpp
 ** \~french
 * \brief Implémentation de la classe SwiftContext
 * \details
 * \li SwiftContext : connexion à un container Swift
 ** \~english
 * \brief Implement classe SwiftContext
 * \details
 * \li SwiftContext : Swift container connection
 */

#include "SwiftContext.h"

SwiftContext::SwiftContext (std::string auth, std::string account, std::string user, std::string passwd, std::string container) :
    Context(),
    auth_url(auth),user_name(user), user_account(account), user_passwd(passwd), container_name(container)
{
}

bool SwiftContext::connection() {
    connected = true;
    return true;
}

bool SwiftContext::read(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Swift read : " << size << " bytes (from the " << offset << " one) in the object " << name);
    return true;
}


bool SwiftContext::write(uint8_t* data, int offset, int size, std::string name) {
    LOGGER_DEBUG("Swift write : " << size << " bytes (from the " << offset << " one) in the object " << name);
    return true;
}

bool SwiftContext::writeFull(uint8_t* data, int size, std::string name) {
    LOGGER_DEBUG("Swift write : " << size << " bytes (one shot) in the object " << name);
    return true;
}
