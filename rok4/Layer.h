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

#ifndef LAYER_H_
#define LAYER_H_

#include <vector>
#include <string>
#include "Pyramid.h"
#include "CRS.h"
#include "Style.h"
#include "MetadataURL.h"
#include "ServicesConf.h"
#include "Interpolation.h"
#include "Keyword.h"


/**
 * \~french \brief Structure de stockage d'une emprise géographique
 * \~english \brief Storage structure for a geographic bounding box
 */
struct GeographicBoundingBoxWMS {
public:
    /**
     * \~french \brief minx abscisse du coin inférieur gauche de l'emprise
     * \~english \brief minx x-coordinate of the bottom left corner of the boundingBox
     */
    double minx;
    /**
     * \~french \brief miny ordonnée du coin inférieur gauche de l'emprise
     * \~english \brief miny y-coordinate of the bottom left corner of the boundingBox
     */
    double miny;
    /**
     * \~french \brief maxx abscisse du coin supérieur droit de l'emprise
     * \~english \brief maxx x-coordinate of the top right corner of the boundingBox
     */
    double maxx;
    /**
     * \~french \brief maxy ordonnée du coin supérieur droit de l'emprise
     * \~english \brief maxy y-coordinate of the top right corner of the boundingBox
     */
    double maxy;
    GeographicBoundingBoxWMS() {}
};

/**
 * \~french \brief Structure de stockage d'une emprise
 * \~english \brief Storage structure for a bounding box
 */
struct BoundingBoxWMS {
public:
    /**
     * \~french \brief Code du CRS tel qu'il est ecrit dans la requete WMS
     * \~english \brief CRS identifier from the WMS request
     */
    std::string srs;
    /**
     * \~french \brief minx abscisse du coin inférieur gauche de l'emprise
     * \~english \brief minx x-coordinate of the bottom left corner of the boundingBox
     */
    double minx;
    /**
     * \~french \brief miny ordonnée du coin inférieur gauche de l'emprise
     * \~english \brief miny y-coordinate of the bottom left corner of the boundingBox
     */
    double miny;
    /**
     * \~french \brief maxx abscisse du coin supérieur droit de l'emprise
     * \~english \brief maxx x-coordinate of the top right corner of the boundingBox
     */
    double maxx;
    /**
     * \~french \brief maxy ordonnée du coin supérieur droit de l'emprise
     * \~english \brief maxy y-coordinate of the top right corner of the boundingBox
     */
    double maxy;
    BoundingBoxWMS() {}
};

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
 *     <opaque>true</opaque>
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
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keyWords;
    /**
     * \~french \brief Pyramide de tuiles
     * \~english \brief Tile pyramid
     */
    Pyramid* dataPyramid;
    // TODO Rajouter une metadataPyramid
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
     * \deprecated
     * \~french \brief Définit si la couche est opaque
     * \~english \brief Whether the layer is opaque
     */
    bool opaque;
    /**
     * \~french \brief Nom de l'entité propriétaire de la couche
     * \~english \brief Oo
     */
    std::string authority;
    /**
     * \~french \brief Interpolation utilisée pour reprojeter ou recadrer les tuiles
     * \~english \brief Interpolation used for resizing and reprojecting tiles
     */
    Interpolation::KernelType resampling;
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

public:
    /**
     * \~french
     * \brief Crée un Layer à partir des ses éléments constitutifs
     * \param[in] id identifiant
     * \param[in] title titre
     * \param[in] abstract résumé
     * \param[in] keyWords liste des mots-clés
     * \param[in] dataPyramid pyramide de tuiles
     * \param[in] styles liste des styles, le premier élément est le style par défaut
     * \param[in] minRes résolution minimale de la couche
     * \param[in] maxRes résolution maximale de la couche
     * \param[in] WMSCRSList liste des systèmes de coordonnées authorisés
     * \param[in] opaque définit si la couche est opaque
     * \param[in] authority nom de l'entité propriétaire de la couche
     * \param[in] resampling interpolation utilisée pour reprojeter ou recadrer les tuile
     * \param[in] geographicBoundingBox emprise des données en coordonnées géographique (WGS84)
     * \param[in] boundingBox emprise des données dans le système de coordonnées natif
     * \param[in] metadataURLs liste des métadonnées associées
     * \~english
     * \brief Create a Layer
     * \param[in] id identifier
     * \param[in] title title
     * \param[in] abstract abstract
     * \param[in] keyWords list of keywords
     * \param[in] dataPyramid Tile pyramids
     * \param[in] styles linked styles list, first element is the default style
     * \param[in] minRes minimal display resolution
     * \param[in] maxRes maximal display resolution
     * \param[in] WMSCRSList authorised coordinates systems list
     * \param[in] opaque whether the layer is opaque
     * \param[in] authority owner's entitity name
     * \param[in] resampling interpolation used for resizing and reprojecting tiles
     * \param[in] geographicBoundingBox data bounding box in geographic coordinates (WGS84)
     * \param[in] boundingBox data bounding box in native coordinates system
     * \param[in] metadataURLs linked metadata list
     */
    Layer ( std::string id, std::string title, std::string abstract,
            std::vector<Keyword> & keyWords, Pyramid*& dataPyramid,
            std::vector<Style*> & styles, double minRes, double maxRes,
            std::vector<CRS> & WMSCRSList, bool opaque, std::string authority,
            Interpolation::KernelType resampling, GeographicBoundingBoxWMS geographicBoundingBox,
            BoundingBoxWMS boundingBox, std::vector<MetadataURL>& metadataURLs )
        :id ( id ), title ( title ), abstract ( abstract ), keyWords ( keyWords ),
         dataPyramid ( dataPyramid ), styles ( styles ), minRes ( minRes ),
         maxRes ( maxRes ), WMSCRSList ( WMSCRSList ), opaque ( opaque ),
         authority ( authority ),resampling ( resampling ),
         geographicBoundingBox ( geographicBoundingBox ),
         boundingBox ( boundingBox ), metadataURLs ( metadataURLs ), defaultStyle ( styles.at ( 0 )->getId() ) {
    }

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
     * Deux possibilités :
     * - la tuile existe dans la pyramide : la tuile est retournée
     * - la tuile n'existe pas dans la pyramide : retourne une tuile de NoData si errorDataSource est nulle, errorDataSource sinon
     * \brief Retourne une tuile
     * \param [in] x Indice de la colone de la tuile
     * \param [in] y Indice de la ligne de la tuile
     * \param [in] tmId Identifiant du TileMatrix contenant la tuile
     * \param [in] errorDataSource Réponse alternative à renvoyer si la tuile demandée n'existe pas dans la pyramide
     * \return une tuile ou un message d'erreur
     * \~english
     * Two possibilities :
     * - the tile is present in the pyramid : the tile is returned
     * - the tile is not present in the pyramid : a NoData tile is returned if errorDataSource is null, else errorDataSource is returned
     * \brief Return a tile
     * \param [in] x Column index of the tile
     * \param [in] y Line index of the tile
     * \param [in] tmId TileMatrix identifier
     * \param [in] errorDataSource Alternative response in case of missing data
     * \return a tile or an error message
     */
    DataSource* gettile ( int x, int y, std::string tmId, DataSource* errorDataSource = NULL );
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
    Image* getbbox (ServicesConf& servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, int& error );
    /**
    * \~french
    * \brief Retourne le résumé
    * \return résumé
    * \~english
    * \brief Return the abstract
    * \return abstract
    */
    std::string              getAbstract()   const {
        return abstract;
    }
    /**
     * \~french
     * \brief Retourne le nom de l'entité propriétaire de la couche
     * \return nom
     * \~english
     * \brief Return the layer owner's entitity name
     * \return name
     */
    std::string              getAuthority()  const {
        return authority;
    }
    /**
     * \~french
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    std::vector<Keyword>* getKeyWords() {
        return &keyWords;
    }
    /**
     * \~french
     * \brief Retourne l'échelle maximum
     * \return échelle maximum
     * \~english
     * \brief Return the maximum scale
     * \return maximum scale
     */
    double                   getMaxRes()     const {
        return maxRes;
    }
    /**
     * \~french
     * \brief Retourne l'échelle minimum
     * \return échelle minimum
     * \~english
     * \brief Return the minimum scale
     * \return minimum scale
     */
    double                   getMinRes()     const {
        return minRes;
    }
    /**
     * \deprecated
     * \~french
     * \brief La couche est elle opaque
     * \return true si oui
     * \~english
     * \brief The layer is opaque
     * \return true if it is
     */
    bool                     getOpaque()     const {
        return opaque;
    }
    /**
     * \~french
     * \brief Retourne la pyramide de données associée
     * \return pyramide
     * \~english
     * \brief Return the associated data pyramid
     * \return pyramid
     */
    Pyramid*&            getDataPyramid() {
        return dataPyramid;
    }
    /**
     * \~french
     * \brief Retourne l'interpolation utilisée
     * \return interpolation
     * \~english
     * \brief Return the used interpolation
     * \return interpolation
     */
    Interpolation::KernelType getResampling() const {
        return resampling;
    }
    /**
     * \~french
     * \brief Retourne le style par défaut associé à la couche
     * \return identifiant de style
     * \~english
     * \brief Return the layer's default style
     * \return style identifier
     */
    std::string getDefaultStyle() const {
        return defaultStyle;
    }
    /**
     * \~french
     * \brief Retourne la liste des styles associés à la couche
     * \return liste de styles
     * \~english
     * \brief Return the associated styles list
     * \return styles list
     */
    std::vector<Style*>      getStyles()     const {
        return styles;
    }
    /**
     * \~french
     * \brief Retourne le titre
     * \return titre
     * \~english
     * \brief Return the title
     * \return title
     */
    std::string              getTitle()      const {
        return title;
    }
    /**
     * \~french
     * \brief Retourne la liste des systèmes de coordonnées authorisés
     * \return liste des CRS
     * \~english
     * \brief Return the authorised coordinates systems list
     * \return CRS list
     */
    std::vector<CRS> getWMSCRSList() const {
        return WMSCRSList;
    }
    /**
     * \~french
     * \brief Retourne l'emprise des données en coordonnées géographique (WGS84)
     * \return emprise
     * \~english
     * \brief Return the data bounding box in geographic coordinates (WGS84)
     * \return bounding box
     */
    GeographicBoundingBoxWMS getGeographicBoundingBox() const {
        return geographicBoundingBox;
    }
    /**
     * \~french
     * \brief Retourne l'emprise des données dans le système de coordonnées natif
     * \return emprise
     * \~english
     * \brief Return the data bounding box in the native coordinates system
     * \return bounding box
     */
    BoundingBoxWMS           getBoundingBox() const {
        return boundingBox;
    }
    /**
     * \~french
     * \brief Retourne la liste des métadonnées associées
     * \return liste de métadonnées
     * \~english
     * \brief Return the associated metadata list
     * \return metadata list
     */
    std::vector<MetadataURL> getMetadataURLs() const {
        return metadataURLs;
    }
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~Layer();
};

#endif /* LAYER_H_ */
