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
public:
    CRS();
    CRS(std::string crs_code);
    CRS(const CRS& crs);
    void buildProj4Code();
    bool isProj4Compatible();
    bool isLongLat();
    long double getMetersPerUnit();
    void setRequestCode(std::string crs);
    bool cmpRequestCode(std::string crs);
    std::string getAuthority(); // Renvoie l'autorite du code passe dans la requete WMS (Ex: EPSG,epsg,IGNF,etc.)
    std::string getIdentifier();// Renvoie l'identifiant du code passe dans la requete WMS (Ex: 4326,LAMB93,etc.)
    bool operator==(const CRS& crs) const;
    bool operator!=(const CRS& crs) const;
    ~CRS() {};
    std::string inline getRequestCode() {
        return requestCode;
    }
    std::string inline getProj4Code() {
        return proj4Code;
    }
    BoundingBox<double> boundingBoxFromGeographic(BoundingBox<double> geographicBBox);
    BoundingBox<double> boundingBoxFromGeographic(double minx, double miny, double maxx, double maxy);

};

#endif
