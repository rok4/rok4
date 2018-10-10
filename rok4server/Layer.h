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
 * \file Layer.h
 * \~french
 * \brief Définition de la classe Layer modélisant les couches de données.
 * \~english
 * \brief Define the Layer Class handling data layer.
 */

class Layer;

#ifndef LAYER_H_
#define LAYER_H_

#include <vector>
#include <string>
#include "Pyramid.h"
#include "CRS.h"
#include "Style.h"
#include "MetadataURL.h"
#include "Interpolation.h"
#include "Keyword.h"
#include "BoundingBox.h"

#include "LayerXML.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Layer représente une couche du service WMS ou WMTS.
 * Une couche est défini par :
 * \li Une source de données
 * \li Les styles diponibles
 * \li Les systèmes de coordonnées authorisées
 * \li Une emprise
 *
 * Exemple de fichier de layer complet :
 * \brief Gestion des couches
 * \~english
 * A Layer represent a service layer either WMS or WMTS
 * The layer contain reference to :
 * \li a data source
 * \li availlable styles
 * \li availlable coordinates systems
 * \li a bounding box
 *
 * Layer file sample :
 * \brief Layer handler
 * \details \~ \code{.xml}
 * <layer>
 *     <title>SCAN1000_JPG_LAMB93_FXX</title>
 *     <abstract>Couche utilisant le descripteur de pyramide SCAN1000_JPG_LAMB93_FXX.pyr</abstract>
 *     <keywordList>
 *         <keyword>LAMB93_10cm</keyword>
 *         <keyword>255,255,255</keyword>
 *         <keyword>rgb</keyword>
 *         <keyword>bicubic</keyword>
 *         <keyword>TIFF_JPG_INT8</keyword>
 *         <keyword>Samples per pixel: 3</keyword>
 *         <keyword>Tiles per width: 16</keyword>
 *         <keyword>Tiles per height: 16</keyword>
 *         <keyword>Directory depth: 2</keyword>
 *     </keywordList>
 *     <style>normal</style>
 *     <EX_GeographicBoundingBox>
 *         <westBoundLongitude>-4.9942572726974</westBoundLongitude>
 *         <eastBoundLongitude>10.9803184076772</eastBoundLongitude>
 *         <southBoundLatitude>41.0037193075078</southBoundLatitude>
 *         <northBoundLatitude>51.1940240110352</northBoundLatitude>
 *     </EX_GeographicBoundingBox>
 *     <WMSCRSList>
 *         <WMSCRS>IGNF:LAMB93</WMSCRS>
 *         <WMSCRS>CRS:84</WMSCRS>
 *         <WMSCRS>IGNF:WGS84G</WMSCRS>
 *         <WMSCRS>EPSG:3857</WMSCRS>
 *         <WMSCRS>EPSG:4258</WMSCRS>
 *     </WMSCRSList>
 *     <boundingBox CRS="IGNF:LAMB93" minx="26214.4" miny="6023116.8" maxx="1258291.2" maxy="7150336"/>
 *     <minRes>102.4</minRes>
 *     <maxRes>209715.2</maxRes>
 *     <authority>IGNF</authority>
 *     <resampling>lanczos_4</resampling>
 *     <pyramid>../pyramids/SCAN1000_JPG_LAMB93_FXX.pyr</pyramid>
 * </layer>
 * \endcode
 */
class Layer {
private:
    /**
     * \~french \brief Identifiant WMS/WMTS de la couche
     * \~english \brief WMS/WMTS layer identifier
     */
    std::string id;
    /**
     * \~french \brief Titre
     * \~english \brief Title
     */
    std::string title;
    /**
     * \~french \brief Résumé
     * \~english \brief abstract
     */
    std::string abstract;
    /**
     * \~french \brief Autorisé le WMS pour ce layer
     * \~english \brief Authorized WMS for this layer
     */
    bool WMSAuthorized;
    /**
     * \~french \brief Autorisé le WMTS pour ce layer
     * \~english \brief Authorized WMTS for this layer
     */
    bool WMTSAuthorized;
    /**
     * \~french \brief Autorisé le TMS pour ce layer
     * \~english \brief Authorized TMS for this layer
     */
    bool TMSAuthorized;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keyWords;
    /**
     * \~french \brief Pyramide de tuiles
     * \~english \brief Tile pyramid
     */
    Pyramid* dataPyramid;

    /**
     * \~french \brief Chemin du descripteur de pyramide
     * \~english \brief Pyramid descriptor path
     */    
    std::string dataPyramidFilePath;
    /**
     * \~french \brief Nom de l'entité propriétaire de la couche
     * \~english \brief Oo
     */
    std::string authority;
    /**
     * \~french \brief Emprise des données en coordonnées géographique (WGS84)
     * \~english \brief Data bounding box in geographic coordinates (WGS84)
     */
    GeographicBoundingBoxWMS geographicBoundingBox;
    /**
     * \~french \brief Emprise des données dans le système de coordonnées natif
     * \~english \brief Data bounding box in native coordinates system
     */
    BoundingBoxWMS boundingBox;
    /**
     * \~french \brief Liste des métadonnées associées
     * \~english \brief Linked metadata list
     */
    std::vector<MetadataURL> metadataURLs;

    /******************* PYRAMIDE VECTEUR ********************/
    std::string metadataJson;

    
    /******************* PYRAMIDE RASTER *********************/

    /**
     * \~french \brief Identifiant du style par défaut
     * \~english \brief default style identifier
     */
    std::string defaultStyle;
    /**
     * \~french \brief Liste des styles associés
     * \~english \brief Linked styles list
     */
    std::vector<Style*> styles;
    /**
     * \~french \brief Résolution minimale d'affichage
     * \~english \brief Minimal display resolution
     */
    double minRes;
    /**
     * \~french \brief Résolution maximale d'affichage
     * \~english \brief Maximal display resolution
     */
    double maxRes;
    /**
     * \~french \brief Liste des systèmes de coordonnées authorisées pour le WMS
     * \~english \brief Authorised coordinates systems list for WMS
     */
    std::vector<CRS> WMSCRSList;
    /**
     * \~french \brief Interpolation utilisée pour reprojeter ou recadrer les tuiles
     * \~english \brief Interpolation used for resizing and reprojecting tiles
     */
    Interpolation::KernelType resampling;
    /**
     * \~french \brief GetFeatureInfo autorisé
     * \~english \brief Authorized GetFeatureInfo
     */
    bool getFeatureInfoAvailability;
    /**
     * \~french \brief Source du GetFeatureInfo
     * \~english \brief Source of GetFeatureInfo
     */
    std::string getFeatureInfoType;
    /**
     * \~french \brief URL du service WMS-V à utiliser pour le GetFeatureInfo
     * \~english \brief WMS-V service URL to use for getFeatureInfo
     */
    std::string getFeatureInfoBaseURL;
    /**
     * \~french \brief Type de service (WMS ou WMTS)
     * \~english \brief Type of service (WMS or WMTS)
     */
    std::string GFIService;
    /**
     * \~french \brief Version du service
     * \~english \brief Version of service
     */
    std::string GFIVersion;
    /**
     * \~french \brief Paramètre query_layers à fournir au service
     * \~english \brief Parameter query_layers for the service
     */
    std::string GFIQueryLayers;
    /**
     * \~french \brief Paramètre layers à fournir au service
     * \~english \brief Parameter layers for the service
     */
    std::string GFILayers;
    /**
     * \~french \brief Modification des EPSG autorisé (pour Geoserver)
     * \~english \brief Modification of EPSG is authorized (for Geoserver)
     */
    bool GFIForceEPSG;

public:
    /**
    * \~french
    * Crée un Layer à partir d'un LayerXML
    * \brief Constructeur
    * \param[in] s LayerXML contenant les informations
    * \~english
    * Create a Layer from a LayerXML
    * \brief Constructor
    * \param[in] s LayerXML to get informations
    */
    Layer ( const LayerXML& l );

    /**
     * \~french
     * \brief Crée un Layer à partir des ses éléments constitutifs
     * \param[in] obj layer
     * \param[in] styleList liste des styles disponibles
     * \param[in] tmsList liste des tms disponibles
     * \~english
     * \brief Create a Layer
     * \param[in] obj layer
     * \param[in] styleList available style list
     * \param[in] tmsList available tms list
     */
    Layer (Layer* obj, ServerXML* sxml);

    /**
     * \~french
     * \brief Retourne l'indentifiant de la couche
     * \return identifiant
     * \~english
     * \brief Return the layer's identifier
     * \return identifier
     */
    std::string getId();
    /**
     * \~french
     * L'image résultante est découpé sur l'emprise de définition du système de coordonnées demandé.
     * Code d'erreur possible :
     *  - \b 0 pas d'erreur
     *  - \b 1 erreur de reprojection de l'emprise demandé dans le système de coordonnées de la pyramide
     *  - \b 2 l'emprise demandée nécessite plus de tuiles que le nombre authorisé.
     * \brief Retourne une l'image correspondant à l'emprise demandée
     * \param [in] servicesConf paramètre de configuration du service WMS
     * \param [in] bbox rectangle englobant demandé
     * \param [in] width largeur de l'image demandé
     * \param [in] height hauteur de l'image demandé
     * \param [in] dst_crs système de coordonnées du rectangle englobant
     * \param [in,out] error code de retour d'erreur
     * \return une image ou un poiteur nul
     * \~english
     * The resulting image is cropped on the coordinates system definition area.
     * \brief
     * \param [in] servicesConf WMS service configuration
     * \param [in] bbox requested bounding box
     * \param [in] width requested image widht
     * \param [in] height requested image height
     * \param [in] dst_crs bounding box coordinate system
     * \param [in,out] error error code
     * \return an image or a null pointer
     */
    Image* getbbox (ServicesXML* servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, int dpi, int& error );
    /**
    * \~french
    * \brief Retourne le résumé
    * \return résumé
    * \~english
    * \brief Return the abstract
    * \return abstract
    */
    std::string getAbstract() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service WMS
     * \return WMSAuthorized
     * \~english
     * \brief Return the right to use WMS
     * \return WMSAuthorized
     */
    bool getWMSAuthorized() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service TMS
     * \return TMSAuthorized
     * \~english
     * \brief Return the right to use TMS
     * \return TMSAuthorized
     */
    bool getTMSAuthorized() ;
    /**
     * \~french
     * \brief Retourne le droit d'utiliser un service WMTS
     * \return WMTSAuthorized
     * \~english
     * \brief Return the right to use WMTS
     * \return WMTSAuthorized
     */
    bool getWMTSAuthorized() ;
    /**
     * \~french
     * \brief Retourne le nom de l'entité propriétaire de la couche
     * \return nom
     * \~english
     * \brief Return the layer owner's entitity name
     * \return name
     */
    std::string getAuthority() ;
    /**
     * \~french
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    std::vector<Keyword>* getKeyWords() ;
    /**
     * \~french
     * \brief Retourne l'échelle maximum
     * \return échelle maximum
     * \~english
     * \brief Return the maximum scale
     * \return maximum scale
     */
    double getMaxRes() ;
    /**
     * \~french
     * \brief Retourne l'échelle minimum
     * \return échelle minimum
     * \~english
     * \brief Return the minimum scale
     * \return minimum scale
     */
    double getMinRes() ;
    /**
     * \~french
     * \brief Retourne la pyramide de données associée
     * \return pyramide
     * \~english
     * \brief Return the associated data pyramid
     * \return pyramid
     */
    Pyramid* getDataPyramid() ;
    /**
     * \~french
     * \brief Retourne le chemin du descripteur de pyramide
     * \return pyramide
     * \~english
     * \brief Return the pyramid descriptor path
     * \return pyramid
     */
    std::string getDataPyramidFilePath() ;
    /**
     * \~french
     * \brief Retourne l'interpolation utilisée
     * \return interpolation
     * \~english
     * \brief Return the used interpolation
     * \return interpolation
     */
    Interpolation::KernelType getResampling() ;
    /**
     * \~french
     * \brief Retourne le style par défaut associé à la couche
     * \return identifiant de style
     * \~english
     * \brief Return the layer's default style
     * \return style identifier
     */
    std::string getDefaultStyle() ;
    /**
     * \~french
     * \brief Retourne la liste des styles associés à la couche
     * \return liste de styles
     * \~english
     * \brief Return the associated styles list
     * \return styles list
     */
    std::vector<Style*> getStyles() ;
    /**
     * \~french
     * \brief Retourne le style associé à la couche
     * \return le style si associé, NULL sinon
     * \~english
     * \brief Return the associated style
     * \return the style if present, NULL otherwise
     */
    Style* getStyle(std::string id) ;

    /**
     * \~french
     * \brief Retourne le titre
     * \return titre
     * \~english
     * \brief Return the title
     * \return title
     */
    std::string getTitle() ;

    /**
     * \~french
     * \brief Retourne les métadonnées en JSON (vecteur)
     * \return metadonnée
     * \~english
     * \brief Return the JSON metadata (vector)
     * \return metadata
     */
    std::string getMetadataJSON() ;
    /**
     * \~french
     * \brief Mémorise les métadonnées en JSON (vecteur)
     * \~english
     * \brief Memorize the JSON metadata (vector)
     */
    void setMetadataJSON(std::string mjson) ;
    /**
     * \~french
     * \brief Retourne la liste des systèmes de coordonnées authorisés
     * \return liste des CRS
     * \~english
     * \brief Return the authorised coordinates systems list
     * \return CRS list
     */
    std::vector<CRS> getWMSCRSList() ;

    /**
     * \~french
     * \brief Teste la présence du CRS dans la liste
     * \return Présent ou non
     * \~english
     * \brief Test if CRS is in the CRS list
     * \return Present or not
     */
    bool isInWMSCRSList(CRS* c) ;
    /**
     * \~french
     * \brief Retourne l'emprise des données en coordonnées géographique (WGS84)
     * \return emprise
     * \~english
     * \brief Return the data bounding box in geographic coordinates (WGS84)
     * \return bounding box
     */
    GeographicBoundingBoxWMS getGeographicBoundingBox() ;
    /**
     * \~french
     * \brief Retourne l'emprise des données dans le système de coordonnées natif
     * \return emprise
     * \~english
     * \brief Return the data bounding box in the native coordinates system
     * \return bounding box
     */
    BoundingBoxWMS getBoundingBox() ;
    /**
     * \~french
     * \brief Retourne la liste des métadonnées associées
     * \return liste de métadonnées
     * \~english
     * \brief Return the associated metadata list
     * \return metadata list
     */
    std::vector<MetadataURL> getMetadataURLs() ;
    /**
     * \~french
     * \brief GFI est-il autorisé
     * \return true si oui
     * \~english
     * \brief Is GFI authorized
     * \return true if it is
     */
    bool isGetFeatureInfoAvailable() ;
    /**
     * \~french
     * \brief Retourne la source du GFI
     * \return source du GFI
     * \~english
     * \brief Return the source used by GFI
     * \return source used by GFI
     */
    std::string getGFIType() ;
    /**
     * \~french
     * \brief Retourne l'URL du service de GFI
     * \return URL de service
     * \~english
     * \brief Return the URL of the service used for GFI
     * \return URL of the service
     */
    std::string getGFIBaseUrl() ;
    /**
     * \~french
     * \brief Retourne le paramètre layers de la requête de GFI
     * \return paramètre layers
     * \~english
     * \brief Return the parameter layers of GFI request
     * \return parameter layers
     */
    std::string getGFILayers() ;
    /**
     * \~french
     * \brief Retourne le paramètre query_layers de la requête de GFI
     * \return paramètre query_layers
     * \~english
     * \brief Return the parameter query_layers of GFI request
     * \return parameter query_layers
     */
    std::string getGFIQueryLayers() ;
    /**
     * \~french
     * \brief Retourne le type du service de GFI
     * \return type du service de GFI
     * \~english
     * \brief Return type of service used for GFI
     * \return type of service used for GFI
     */
    std::string getGFIService() ;
    /**
     * \~french
     * \brief Retourne la version du service de GFI
     * \return version du service de GFI
     * \~english
     * \brief Return version of service used for GFI
     * \return version of service used for GFI
     */
    std::string getGFIVersion() ;
    /**
     * \~french
     * \brief
     * \return
     * \~english
     * \brief
     * \return
     */
    bool getGFIForceEPSG() ;
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~Layer();
};

#endif /* LAYER_H_ */
