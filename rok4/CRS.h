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

#ifndef CRS_H
#define CRS_H

#include <string>
#include "BoundingBox.h"
/**
* @class CRS
* @brief Gestion des CRS
*/


class CRS {
private:
    std::string requestCode;    // Code du CRS tel qu'il est ecrit dans la requete WMS
    std::string proj4Code;      // Code du CRS dans la base proj4
    BoundingBox<double> definitionArea;
public:
    CRS();
    CRS ( std::string crs_code );
    CRS ( const CRS& crs );
    CRS& operator= (CRS const& other);
    
    void buildProj4Code();
    void fetchDefinitionArea();
    bool isProj4Compatible();
    bool isLongLat();
    long double getMetersPerUnit();
    void setRequestCode ( std::string crs );
    bool cmpRequestCode ( std::string crs );
    std::string getAuthority(); // Renvoie l'autorite du code passe dans la requete WMS (Ex: EPSG,epsg,IGNF,etc.)
    std::string getIdentifier();// Renvoie l'identifiant du code passe dans la requete WMS (Ex: 4326,LAMB93,etc.)
    bool operator== ( const CRS& crs ) const;
    bool operator!= ( const CRS& crs ) const;
    ~CRS() {};
    std::string inline getRequestCode() {
        return requestCode;
    }
    std::string inline getProj4Code() {
        return proj4Code;
    }
    BoundingBox<double> boundingBoxFromGeographic ( BoundingBox<double> geographicBBox );
    BoundingBox<double> boundingBoxFromGeographic ( double minx, double miny, double maxx, double maxy );
    
    BoundingBox<double> boundingBoxToGeographic ( BoundingBox<double> geographicBBox );
    BoundingBox<double> boundingBoxToGeographic ( double minx, double miny, double maxx, double maxy );

    bool validateBBox ( BoundingBox< double > BBox );
    bool validateBBox ( double minx, double miny, double maxx, double maxy );
    
    bool validateBBoxGeographic ( BoundingBox< double > BBox );
    bool validateBBoxGeographic ( double minx, double miny, double maxx, double maxy );
    
    BoundingBox<double> cropBBox ( BoundingBox< double > BBox );
    BoundingBox<double> cropBBox ( double minx, double miny, double maxx, double maxy );
    
    
};

#endif
