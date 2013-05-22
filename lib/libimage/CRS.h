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
 * \file CRS.h
 * \~french
 * \brief Définition de la gestion des systèmes de référence
 * \~english
 * \brief Define the reference systems handler
 */

#ifndef CRS_H
#define CRS_H

#include <string>
#include "BoundingBox.h"

/**
 * \~french \brief Code utilisé en cas de non correspondance avec les référentiel de Proj
 * \~english \brief Used code when no corresponding Proj code is found
 */
#define NO_PROJ4_CODE "noProj4Code"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Un CRS permet de faire le lien entre un identifiant de CRS issue d'une requête WMS et l'identifiant utilisé dans la bibliothèque Proj.
 * Des fonctions de gestion des emprises sont disponibles (reprojection, recadrage sur l'emprise de définition)
 * \brief Gestion des systèmes de référence
 * \~english
 * A CRS allow to link the WMS and the Proj library identifiers.
 * Functions are availlable to work with bounding boxes (reprojection, cropping on the definition area)
 * \brief Reference systems handler
 */

class CRS {
private:
    /**
     * \~french \brief Code du CRS tel qu'il est ecrit dans la requete WMS
     * \~english \brief CRS identifier from the WMS request
     */
    std::string requestCode;
    /**
     * \~french \brief Code du CRS dans la base proj4
     * \~english \brief CRS identifier from Proj registry
     */
    std::string proj4Code;
    /**
     * \~french \brief Emprise de définition du CRS
     * \~english \brief CRS's definition area
     */
    BoundingBox<double> definitionArea;
public:
    /**
     * \~french
     * \brief Crée un CRS sans correspondance avec une entrée du registre PROJ
     * \~english
     * \brief Create a CRS without Proj correspondance
     */
    CRS();
    /**
     * \~french
     * \brief Crée un CRS à partir de son identifiant
     * \details La chaîne est comparée, sans prendre en compte la casse, avec les registres de Proj. Puis la zone de validité est récupérée dans le registre.
     * \param[in] crs_code identifiant du CRS
     * \~english
     * \brief Create a CRS from its identifier
     * \details The string is compared, case insensitively, to Proj registry. Then the corresponding definition area is fetched from Proj.
     * \param[in] crs_code CRS identifier
     */
    CRS ( std::string crs_code );
    /**
     * \~french
     * Crée un CRS à partir d'un autre
     * \brief Constructeur de copie
     * \param[in] crs CRS à copier
     * \~english
     * Create a CRS from another
     * \brief Copy Constructor
     * \param[in] crs CRS to copy
     */
    CRS ( const CRS& crs );
    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    CRS& operator= ( CRS const& other );
    /**
     * \~french
     * \brief Test d'egalite de 2 CRS
     * \return true s'ils ont le meme code Proj, false sinon
     * \~english
     * \brief Test whether 2 CRS are equals
     * \return true if they share the same Proj identifier
     */
    bool operator== ( const CRS& crs ) const;
    /**
     * \~french
     * \brief Test d'inégalite de 2 CRS
     * \return true s'ils ont un code Proj différent, false sinon
     * \~english
     * \brief Test whether 2 CRS are different
     * \return true if the the Proj identifier is different
     */
    bool operator!= ( const CRS& crs ) const;
    /**
     * \~french
     * \brief Détermine a partir du code du CRS passe dans la requete le code Proj correspondant
     * \~english
     * \brief Determine the Proj code from the requested CRS
     */
    void buildProj4Code();
    /**
     * \~french
     * \brief Récupère l'emprise de définition du CRS dans les registres Proj
     * \~english
     * \brief Fetch the CRS definition area from Proj registries
     */
    void fetchDefinitionArea();
    /**
     * \~french
     * \brief Test si le CRS possède un équivalent dans Proj
     * \return vrai si disponible dans Proj
     * \~english
     * \brief Test whether the CRS has a Proj equivalent
     * \return true if available in Proj
     */
    bool isProj4Compatible();
    /**
     * \~french
     * \brief Test si le CRS est géographique
     * \return vrai si géographique
     * \~english
     * \brief Test whether the CRS is geographic
     * \return true if geographic
     */
    bool isLongLat();
    /**
     * \~french
     * \brief Le nombre de mètre par unité du CRS
     * \return rapport entre le mètre et l'unité du CRS
     * \todo supporter les CRS autres qu'en degré et en mètre
     * \~english
     * \brief Amount of meter in one CRS's unit
     * \return quotient between meter and CRS's unit
     * \todo support all CRS types not only projected in meter and geographic in degree
     */
    long double getMetersPerUnit();
    /**
     * \~french
     * \brief Définit le nouveau code que le CRS représentera
     * \details La chaîne est comparée, sans prendre en compte la casse, avec les registres de Proj. Puis la zone de validité est récupérée dans le registre.
     * \param[in] crs_code identifiant du CRS
     * \~english
     * \brief Assign a new code to the CRS
     * \details The string is compared, case insensitively, to Proj registry. Then the corresponding definition area is fetched from Proj.
     * \param[in] crs_code CRS identifier
     */
    void setRequestCode ( std::string crs );
    /**
     * \~french
     * \brief Compare le code fournit lors de la création du CRS avec la chaîne
     * \param[in] crs chaîne à comparer
     * \return vrai si identique (insenble à la casse)
     * \~english
     * \brief Compare the CRS original code with the supplied string
     * \param[in] crs string for comparison
     * \return true if identic (case insensitive)
     */
    bool cmpRequestCode ( std::string crs );
    /**
     * \~french
     * \brief Retourne l'authorité du CRS
     * \return l'identifiant de l'authorité
     * \~english
     * \brief Return the CRS authority
     * \return the authority identifier
     */
    std::string getAuthority(); // Renvoie l'autorite du code passe dans la requete WMS (Ex: EPSG,epsg,IGNF,etc.)
    /**
     * \~french
     * \brief Retourne l'identifiant du CRS sans l'authorité
     * \return l'identifiant du système
     * \~english
     * \brief Return the CRS identifier without the authority
     * \return the system identifier
     */
    std::string getIdentifier();// Renvoie l'identifiant du code passe dans la requete WMS (Ex: 4326,LAMB93,etc.)

    /**
     * \~french
     * \brief Retourne le code du CRS dans la base proj4
     * \return code du CRS
     * \~english
     * \brief Return the CRS identifier from Proj registry
     * \return CRS identifier
     */
    bool inline isDefine() {
        return ( proj4Code.compare(NO_PROJ4_CODE) == 0);
    }

    /**
     * \~french
     * \brief Retourne le code du CRS tel qu'il est ecrit dans la requete WMS
     * \return code du CRS
     * \~english
     * \brief Return the CRS identifier from the WMS request
     * \return CRS identifier
     */
    std::string inline getRequestCode() {
        return requestCode;
    }

    /**
     * \~french
     * \brief Retourne le code du CRS dans la base proj4
     * \return code du CRS
     * \~english
     * \brief Return the CRS identifier from Proj registry
     * \return CRS identifier
     */
    std::string inline getProj4Code() {
        return proj4Code;
    }
    
    /**
     * \~french
     * \brief Calcule la BoundingBox dans le CRS courant à partir de la BoundingBox Géographique
     * \param[in] geographicBBox une emprise définit en WGS84
     * \return l'emprise dans le CRS courant
     * \~english
     * \brief Compute a BoundingBox in the current CRS from a geographic BoundingBox
     * \param[in] geographicBBox a BoundingBox in geographic coordinate WGS84
     * \return the same BoundingBox in the current CRS
     */
    BoundingBox<double> boundingBoxFromGeographic ( BoundingBox<double> geographicBBox );
    /**
     * \~french
     * \brief Calcule la BoundingBox dans le CRS courant à partir de la BoundingBox Géographique
     * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit en WGS84
     * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit en WGS84
     * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit en WGS84
     * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit en WGS84
     * \return l'emprise dans le CRS courant
     * \~english
     * \brief Compute a BoundingBox in the current CRS from a geographic BoundingBox
     * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in WGS84
     * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in WGS84
     * \param[in] maxx x-coordinate of the top right corner of the boundingBox in WGS84
     * \param[in] maxy y-coordinate of the top right corner of the boundingBox in WGS84
     * \return the same BoundingBox in the current CRS
     */
    BoundingBox<double> boundingBoxFromGeographic ( double minx, double miny, double maxx, double maxy );
    /**
     * \~french
     * \brief Calcule la BoundingBox Géographique à partir de la BoundingBox dans le CRS courant
     * \param[in] geographicBBox une emprise définit dans le CRS courant
     * \return l'emprise en WGS84
     * \~english
     * \brief Compute a geographic BoundingBox from a BoundingBox in the current CRS
     * \param[in] geographicBBox a BoundingBox in the current CRS
     * \return the same BoundingBox in WGS84
     */
    BoundingBox<double> boundingBoxToGeographic ( BoundingBox<double> projectedBBox );
    /**
     * \~french
     * \brief Calcule la BoundingBox Géographique à partir de la BoundingBox dans le CRS courant
     * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit dans le CRS courant
     * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit dans le CRS courant
     * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit dans le CRS courant
     * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit dans le CRS courant
     * \return l'emprise en WGS84
     * \~english
     * \brief Compute a geographic BoundingBox from a BoundingBox in the current CRS
     * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in the current CRS
     * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in the current CRS
     * \param[in] maxx x-coordinate of the top right corner of the boundingBox in the current CRS
     * \param[in] maxy y-coordinate of the top right corner of the boundingBox in the current CRS
     * \return the same BoundingBox in WGS84
     */
    BoundingBox<double> boundingBoxToGeographic ( double minx, double miny, double maxx, double maxy );

    /**
     * \~french
     * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
     * \param[in] geographicBBox une emprise définit dans le CRS courant
     * \return true si incluse
     * \~english
     * \brief Verify if the supplied BoundingBox is in the CRS definition area
     * \param[in] geographicBBox a BoundingBox in the current CRS
     * \return true if inside
     */
    bool validateBBox ( BoundingBox< double > BBox );
    /**
     * \~french
     * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
     * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit dans le CRS courant
     * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit dans le CRS courant
     * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit dans le CRS courant
     * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit dans le CRS courant
     * \return true si incluse
     * \~english
     * \brief Verify if the supplied BoundingBox is in the CRS definition area
     * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in the current CRS
     * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in the current CRS
     * \param[in] maxx x-coordinate of the top right corner of the boundingBox in the current CRS
     * \param[in] maxy y-coordinate of the top right corner of the boundingBox in the current CRS
     * \return true if inside
     */
    bool validateBBox ( double minx, double miny, double maxx, double maxy );
    /**
     * \~french
     * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
     * \param[in] geographicBBox une emprise en WGS84
     * \return true si incluse
     * \~english
     * \brief Verify if the supplied BoundingBox is in the CRS definition area
     * \param[in] geographicBBox a BoundingBox in WGS84
     * \return true if inside
     */
    bool validateBBoxGeographic ( BoundingBox< double > BBox );
    /**
     * \~french
     * \brief Vérifie que la BoundingBox est dans le domaine de définition de la projection
     * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit en WGS84
     * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit en WGS84
     * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit en WGS84
     * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit en WGS84
     * \return true si incluse
     * \~english
     * \brief Verify if the supplied BoundingBox is in the CRS definition area
     * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in WGS84
     * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in WGS84
     * \param[in] maxx x-coordinate of the top right corner of the boundingBox in WGS84
     * \param[in] maxy y-coordinate of the top right corner of the boundingBox in WGS84
     * \return true if inside
     */
    bool validateBBoxGeographic ( double minx, double miny, double maxx, double maxy );
    /**
     * \~french
     * \brief Calcule la BoundingBox incluse dans le domaine de définition du CRS courant
     * \param[in] BBox une emprise définit dans le CRS courant
     * \return l'emprise recadrée
     * \~english
     * \brief Compute a BoundingBox included in the current CRS definition area
     * \param[in] BBox a BoundingBox in the current CRS
     * \return the cropped BoundingBox
     */
    BoundingBox<double> cropBBox ( BoundingBox< double > BBox );
    /**
     * \~french
     * \brief Calcule la BoundingBox incluse dans le domaine de définition du CRS courant
     * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit dans le CRS courant
     * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit dans le CRS courant
     * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit dans le CRS courant
     * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit dans le CRS courant
     * \return l'emprise recadrée
     * \~english
     * \brief Compute a BoundingBox included in the current CRS definition area
     * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in the current CRS
     * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in the current CRS
     * \param[in] maxx x-coordinate of the top right corner of the boundingBox in the current CRS
     * \param[in] maxy y-coordinate of the top right corner of the boundingBox in the current CRS
     * \return the cropped BoundingBox
     */
    BoundingBox<double> cropBBox ( double minx, double miny, double maxx, double maxy );
    
    /**
     * \~french
     * \brief Calcule la BoundingBox incluse dans le domaine de définition du CRS courant
     * \param[in] geographicBBox une emprise définit en WGS84
     * \return l'emprise recadrée
     * \~english
     * \brief Compute a BoundingBox included in the current CRS definition area
     * \param[in] geographicBBox a BoundingBox in WGS84
     * \return the cropped BoundingBox
     */
    BoundingBox<double> cropBBoxGeographic ( BoundingBox< double > BBox );
    /**
     * \~french
     * \brief Calcule la BoundingBox Géographique incluse dans le domaine de définition du CRS courant
     * \param[in] minx abscisse du coin inférieur gauche de l'emprise définit en WGS84
     * \param[in] miny ordonnée du coin inférieur gauche de l'emprise définit en WGS84
     * \param[in] maxx abscisse du coin supérieur droit de l'emprise définit en WGS84
     * \param[in] maxy ordonnée du coin supérieur droit de l'emprise définit en WGS84
     * \return l'emprise recadrée
     * \~english
     * \brief Compute a BoundingBox included in the current CRS definition area
     * \param[in] minx x-coordinate of the bottom left corner of the boundingBox in WGS84
     * \param[in] miny y-coordinate of the bottom left corner of the boundingBox in WGS84
     * \param[in] maxx x-coordinate of the top right corner of the boundingBox in WGS84
     * \param[in] maxy y-coordinate of the top right corner of the boundingBox in WGS84
     * \return the cropped BoundingBox
     */
    BoundingBox<double> cropBBoxGeographic ( double minx, double miny, double maxx, double maxy );
    
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~CRS();

};

#endif

