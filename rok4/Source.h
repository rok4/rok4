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

#ifndef SOURCE_H
#define SOURCE_H

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une source est une classe abstraite qui permet d'indiquer la source d'une donnée
 * Ce peut être une pyramide ou un webservice
 * \brief Origine de la donnée
 * \~english
 * A source is an abstract class used to indicate the origin of a data
 * It could be a pyramid or a webservice
 * \brief Data origin
 */

/**
 * \~french \brief Énumération des types de sources
 * \~english \brief Available data sources enumeration
 */
enum eSourceType {
    PYRAMID = 0,
    WEBSERVICE = 1
};

class Source {

private:

    /**
     * \~french \brief type de la source
     * \~english \brief source type
     */
    eSourceType type;

public:

    /**
     * \~french
     * \brief Construteur
     * \~english
     * \brief Constructor
     */
    Source(eSourceType t);

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    virtual ~Source();

    /**
     * \~french
     * \brief Récupérer le type
     * \~english
     * \brief Get type
     */
    eSourceType getType() {
        return type;
    }
};

#endif // SOURCE_H
