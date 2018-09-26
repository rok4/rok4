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

class Pyramid;

#ifndef PYRAMID_H
#define PYRAMID_H

#include <string>
#include <map>
#include "Level.h"
#include "PyramidXML.h"
#include "TileMatrixSet.h"
#include "CRS.h"
#include "Format.h"
#include "Style.h"
#include <Interpolation.h>
#include "WebService.h"
#include "Source.h"

/**
* @class Pyramid
* @brief Implementation des pyramides
* Une pyramide est associee a un layer et comporte plusieurs niveaux
*/

class Pyramid : public Source {

private:

    /**
     * \~french \brief Liste des différents niveaux de la pyramide
     * \~english \brief List of the different level
     */
    std::map<std::string, Level*> levels;

    /**
     * \~french \brief TileMatrixSet des données
     * \~english \brief TileMatrixSet of the data
     */
    TileMatrixSet* tms;

    /**
     * \~french \brief Format des tuiles
     * \~english \brief Format of the tiles
     */
    Rok4Format::eformat_data format;

    /**
     * \~french \brief Référence au niveau le plus haut
     * \~english \brief Reference to the highest level
     */
    Level* highestLevel;

    /**
     * \~french \brief Référence au niveau le plus bas
     * \~english \brief Reference to the lowest level
     */
    Level* lowestLevel;

    /******************* PYRAMIDE RASTER *********************/

    /**
     * \~french \brief Donne la photométrie des images de la pyramide
     * Non Obligatoire sauf pour les pyramides avec stockage
     * \~english \brief Give the photometry of images
     * Not Mandatory, only used for pyramid onFly
     */
    Photometric::ePhotometric photo;

    /**
     * \~french \brief Donne les valeurs des noData pour la pyramide
     * \~english \brief Give the noData value
     */
    int* ndValues;

    /**
     * \~french \brief Nombre de canaux pour les tuiles
     * \~english \brief Number of channels for the tiles
     */
    int channels;

    bool isBasedPyramid;

    /**
     * \~french \brief Indique si la pyramide peut avoir de la transparence
     * utilisé seulement pour une pyramide de base
     * ne posséde pour le moment aucun intérêt pour une pyramide normale
     * ou une pyramide à la demande
     * \~english \brief Indicate if a pyramid can have transparency
     * It is used only for based pyramid
     * There are no use for a normal pyramid or a OnDemand pyramid
     */
    bool transparent;
    /**
     * \~french \brief Indique si la pyramide a un style
     * utilisé seulement pour une pyramide de base
     * ne posséde pour le moment aucun intérêt pour une pyramide normale
     * ou une pyramide à la demande
     * \~english \brief Indicate if a pyramid have a style
     * It is used only for based pyramid
     * There are no use for a normal pyramid or a OnDemand pyramid
     */
    Style *style;

    bool containOdLevels;

public:

    /**
     * \~french \brief Récupère le premier niveau
     * \return level premier niveau
     * \~english \brief Get the first level
     * \return level first level
     */
    Level* getFirstLevel();

    /**
     * \~french \brief Récupère le plus haut niveau
     * \return level plus haut niveau
     * \~english \brief Get the highest level
     * \return level highest level
     */
    Level* getHighestLevel() ;

    /**
     * \~french \brief Récupère le plus bas niveau
     * \return level plus bas niveau
     * \~english \brief Get the lowest level
     * \return level lowest level
     */
    Level* getLowestLevel() ;

    /**
     * \~french \brief Récupère le TMS
     * \return TileMatrixSet
     * \~english \brief Get the TMS
     * \return TileMatrixSet
     */
    TileMatrixSet* getTms();

    /**
     * \~french \brief Récupère les niveaux
     * \return Liste de level
     * \~english \brief Get the levels
     * \return List of level
     */
    std::map<std::string, Level*>& getLevels() ;

    Level* getLevel(std::string id) ;

    void removeLevel(std::string id) ;

    /**
     * \~french \brief Récupère le format
     * \return format
     * \~english \brief Get the format
     * \return format
     */
    Rok4Format::eformat_data getFormat() ;

    /**
     * \~french \brief Constructeur
     * \~english \brief Constructor
     */
    Pyramid (PyramidXML* p);

    /**
     * \~french
     * \brief Crée une pyramide à partir d'une autre
     * \param[in] obj pyramide
     * \param[in] tmsList liste des tms disponibles
     * \~english
     * \brief Create a pyramid from another
     * \param[in] obj pyramid
     * \param[in] tmsList available tms list
     */
    Pyramid (Pyramid *obj, ServerXML* sxml);

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~Pyramid();

    /******************* PYRAMIDE RASTER *********************/

    Level* getUniqueLevel() ;
    void setUniqueLevel(std::string id);

    /**
     * \~french \brief Indique la photometrie de la pyramide
     * \return photo
     * \~english \brief Indicate the photometry of the pyramid
     * \return photo
     */
    Photometric::ePhotometric getPhotometric();

    /**
     * \~french \brief Récupère le sample
     * \return format
     * \~english \brief Get the sample
     * \return format
     */
    SampleFormat::eSampleFormat getSampleFormat();

    /**
     * \~french \brief Récupère la compression
     * \return format
     * \~english \brief Get the compression
     * \return format
     */
    Compression::eCompression getSampleCompression();

    /**
     * \~french \brief Indique les valeurs de noData
     * \return ndValues
     * \~english \brief Indicate the noData values
     * \return ndValues
     */
    int* getNdValues() ;

    /**
     * \~french \brief Retourne la premiere valeur de noData
     * \~english \brief Return the first noData value
     */
    int getFirstNdValues ();

    /**
     * \~french \brief Récupère le nombre de bits par sample
     * \return format
     * \~english \brief Get the number of bits per sample
     * \return format
     */
    int getBitsPerSample();

    /**
     * \~french \brief Récupère le nombre de canaux d'une tuile
     * \return nombre de canaux
     * \~english \brief Get the number of channels of a tile
     * \return number of channels
     */
    int getChannels() ;

    /**
     * \~french \brief Indique si la pyramide contient des niveaux à la demande
     * \return containOdLevels
     * \~english \brief Indicate if the pyramid contains onDemand levels
     * \return containOdLevels
     */
    bool getContainOdLevels();

    /**
     * \~french \brief Récupère la transparence
     * \return booleen
     * \~english \brief Get the transparency
     * \return boolean
     */
    bool getTransparent();
    /**
     * \~french \brief Modifie la transparence
     * \param[in] booleen
     * \~english \brief Set the transparency
     * \param[in] boolean
     */
    void setTransparent (bool tr) ;

    /**
     * \~french \brief Récupère le style
     * \return style
     * \~english \brief Get the style
     * \return style
     */
    Style* getStyle();
    /**
     * \~french \brief Modifie le style
     * \param[in] style
     * \~english \brief Set the style
     * \param[in] style
     */
    void setStyle (Style * st) ;


    /**
     * \~french \brief Récupère le meilleur niveau pour une résolution donnée
     * \param[in] résolution en x
     * \param[in] résolution en y
     * \param[in] onDemand
     * \~english \brief Get the best level for the given resolution
     * \param[in] resolution in x
     * \param[in] resolution in y
     * \param[in] onDemand
     */
    std::string best_level ( double resolution_x, double resolution_y, bool onDemand );


    /**
     * \~french \brief Récupère une image
     * \~english \brief Get an image
     */
    Image* getbbox (ServicesXML* servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, Interpolation::KernelType interpolation, int dpi, int& error );

    /**
     * \~french \brief Créé une image reprojetée
     * \~english \brief Create a reprojected image
     */
    Image *createReprojectedImage(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesXML* servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

    /**
     * \~french \brief Créé une image reprojetée mais complétée par du nodata
     * Les données sont prises dans cropBbox mais complété par du nodata sur bbox
     * \~english \brief Create a reprojected image but completed by nodata
     * Used data on cropBBox but completed by nodata on bbox
     */
    Image *createExtendedCompoundImage(std::string l, BoundingBox<double> bbox, BoundingBox<double> cropBBox,CRS dst_crs, ServicesXML* servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

    /**
     * \~french \brief Créé une dalle
     * \~english \brief Create a slab
     */
    Image *createBasedSlab(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesXML* servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

    /**
     * \~french \brief Renvoit une image de noData
     * \param[in] id du level de base concerné
     * \param[in] bbox de la requête
     * \~english \brief Get a noData image
     * \param[in] id of the based level
     * \param[in] bbox of the request
     */
    Image *NoDataOnDemand(std::string bLevel, BoundingBox<double> bbox);

};


#endif
