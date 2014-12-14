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
 * \file Style.cpp
 * \~french
 * \brief Implémentation de la classe Style modélisant les styles.
 * \~english
 * \brief Implement the Style Class handling style definition
 */

#include "Style.h"
#include "Logger.h"
#include "intl.h"
#include "config.h"

Style::Style ( const std::string& id,const std::vector<std::string>& titles,
               const std::vector<std::string>& abstracts,const std::vector<Keyword>& keywords,
               const std::vector<LegendURL>& legendURLs, Palette& palette ,int angle, float exaggeration, uint8_t center ) : estompage ( false ), angle ( angle ), exaggeration ( exaggeration ), center ( center ) { //: id(id), titles(titles),             abstracts(abstracts), keywords(keywords), legendURLs(legendURLs), palette(palette)
    LOGGER_DEBUG ( _ ( "Nouveau Style : " ) << id );
    this->id = id.c_str();
    this->titles= titles;
    this->abstracts = abstracts;
    this->keywords = keywords;
    this->legendURLs = legendURLs;
    this->palette = palette;
    if ( angle >= 0 && angle < 360 ) {
        estompage = true;
    }
}

Style::~Style() {

}
