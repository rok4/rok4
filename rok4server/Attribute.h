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

#ifndef ATTRIBUTE_H
#define ATTRIBUTE_H

#include <vector>
#include <string>

class Attribute
{

    public:
        Attribute(std::string n, std::string t, std::string c, std::string v, std::string mn, std::string mx) {
            name = n;
            type = t;
            count = c;
            values = v;
            min = mn;
            max = mx;
            metadataJson = "";
        };
        ~Attribute(){};

        std::string getName() {return name;}
        std::string getType() {return type;}
        std::string getValues() {return values;}
        std::string getCount() {return count;}
        std::string getMin() {return min;}
        std::string getMax() {return max;}

        std::string getMetadataJson() {
            if (metadataJson != "") return metadataJson;
            /*
            {
                "attribute": "gid",
                "count": 1,
                "max": 49,
                "min": 49,
                "type": "number",
                "values": [
                    49
                ]
            }
            */

            std::ostringstream res;
            res << "{\"attribute\":\"" << name << "\"";
            res << ",\"count\":\"" << count << "\"";
            res << ",\"type\":\"" << type << "\"";

            if (min != "") {
                res << ",\"min\":" << min;
            }
            if (max != "") {
                res << ",\"max\":" << max;
            }

            if (values != "") {
                res << ",\"values\":[" << values << "]";
            }

            res << "}";

            metadataJson = res.str();
            return metadataJson;
        }

    private:

        std::string name;
        std::string type;
        std::string values;
        std::string count;
        std::string min;
        std::string max;
        std::string metadataJson;
};

#endif // ATTRIBUTE_H

