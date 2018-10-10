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

class Attribute;

#ifndef TABLE_H
#define TABLE_H

#include <vector>
#include <string>
#include "Attribute.h"

class Table
{

    public:
        Table(std::string n, std::string g, std::vector<Attribute> atts) {
            name = n;
            geometry = g;
            attributes = atts;
            metadataJsonLong = "";
            metadataJsonShort = "";
        }
        ~Table(){};

        std::string getName() {return name;}
        std::string getGeometry() {return geometry;}
        std::vector<Attribute> getAttributes() {return attributes;}
        std::string getMetadataJsonLong() {
            if (metadataJsonLong != "") return metadataJsonLong;
            /*
            {
                "attributeCount": 8,
                "attributes": [...],
                "geometry": "Polygon",
                "layer": "filter"
            }
            */

            std::ostringstream res;
            res << "{\"layer\":\"" << name << "\"";
            res << ",\"geometry\":\"" << geometry << "\"";
            res << ",\"attributeCount\":\"" << attributes.size() << "\"";
            res << ",\"attributes\":[";
            for (int i = 0; i < attributes.size(); i++) {
                if (i != 0) {
                    res << ",";
                }
                res << attributes.at(i).getMetadataJson();
            }
            res << "]}";

            metadataJsonLong = res.str();
            return metadataJsonLong;
        }
        std::string getMetadataJsonShort(std::string max, std::string min) {
            if (metadataJsonShort != "") return metadataJsonShort;
            /*
            {
                "fields": {
                    "genre": "String"
                },
                "id": "limite_administrative_simp",
                "maxzoom": 18,
                "minzoom": 13
            }
            */

            std::ostringstream res;
            res << "{\"id\":\"" << name << "\"";
            res << ",\"maxzoom\":\"" << max << "\"";
            res << ",\"minzoom\":\"" << min << "\"";
            res << ",\"fields\":{";
            for (int i = 0; i < attributes.size(); i++) {
                if (i != 0) {
                    res << ",";
                }
                res << "\"" << attributes.at(i).getName() << "\": \"" << attributes.at(i).getType() << "\"";
            }
            res << "}}";

            metadataJsonShort = res.str();
            return metadataJsonShort;
        }

    private:

        std::string name;
        std::string geometry;
        std::vector<Attribute> attributes;
        std::string metadataJsonLong;
        std::string metadataJsonShort;
};

#endif // ATTRIBUTE_H

