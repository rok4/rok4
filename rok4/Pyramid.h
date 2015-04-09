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

#ifndef PYRAMID_H
#define PYRAMID_H
#include <string>
#include <map>
#include "Level.h"
#include "TileMatrixSet.h"
#include "CRS.h"
#include "Format.h"
#include "Style.h"
#include "ServicesConf.h"
#include <Interpolation.h>

//std::string getMimeType(std::string format);


/**
* @class Pyramid
* @brief Implementation des pyramides
* Une pyramide est associee a un layer et comporte plusieurs niveaux
*/

class Pyramid {

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
    const TileMatrixSet tms;

    /**
     * \~french \brief Format des tuiles
     * \~english \brief Format of the tiles
     */
    const Rok4Format::eformat_data format;

    /**
     * \~french \brief Nombre de canaux pour les tuiles
     * \~english \brief Number of channels for the tiles
     */
    const int channels;

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

    /**
     * \~french \brief Indique si la pyramide contient des tuiles déjà pré-calculées (false)
     * ou si elle est à la demande, donc calcule des tuiles sur demande (true)
     * \~english \brief Indicate if the pyramid has tiles (false)
     * or if tiles are generated on demand (true)
     */
    bool onDemand;

    /**
     * \~french \brief Si une pyramide est à la demande pour l'ensemble des niveaux,
     * alors elle doit pouvoir tirer ses informations d'une autre pyramide
     * Cet attribut contient donc la liste des pyramides de base utilisées
     * pour générer la vraie pyramide
     * Sachant qu'une pyramide de base ne peut pas en contenir à son tour
     * \~english \brief If a pyramid is onDemand for all levels
     * it must contain a reference to other pyramids which have data
     * This is the list of those pyramids
     * They can't be onDemand
     */
    std::vector<Pyramid*> basedPyramids;

    /**
     * \~french \brief Si une pyramide est à la demande par niveau,
     * alors elle doit pouvoir tirer ses informations d'une autre pyramide
     * Cet attribut contient donc la liste des pyramides de base utilisées
     * pour générer la vraie pyramide
     * Sachant qu'une pyramide specifique ne peut pas en contenir à son tour
     * et ne contient qu'un niveau en mémoire
     * \~english \brief If a pyramid is onDemand by level
     * it must contain a reference to other pyramids which have data
     * This is the list of those pyramids
     * They can't be onDemand and have only one level in memory
     */
    std::map<std::string,std::vector<Pyramid*> > specificPyramids;

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

    //C'est un tableau à double entrée qui contient une association des levels de
    //  la pyramide vers les levels des autres pyramides de bases
    //
    //  Un exemple: Si la pyramide Pyr contient trois niveaux 1, 2 et 3; et deux pyramides de base bPyr1 et bPyr2
    //
    //      On veut représenter le tableau suivant:
    //  Level Pyr   Level bPyr1     Level bPyr2
    //      1           2               2
    //      2           2               3
    //      3           4               5
    //
    //  Ce tableau nous dit que pour le level 1 de Pyr, le level associé est 2 pour bPyr1 et 2 poyr bPyr2
    //  Pour le représenter, on va faire une liste de (indice_level_Pyr,liste)
    //
    //      Où  indice_level représente un niveau de Pyr
    //          liste est une liste de (indice_bPyr, indice_level_bPyr)
    //
    //          Où  indice_bPyr représente une basedPyramid
    //              indice_level_bPyr représente le level associé
    //
    //  On trouvera une utilisation de cet attribut dans    ConfLoader.cpp -> parsePyramid() = initialisation
    //                                                      Rok4Server.cpp -> GetTileOnDemand() = lecture
    /**
     * \~french \brief Si une pyramide est à la demande
     * Ce tableau a double entrée fait l'association entre les niveaux
     * théoriques de la pyramide et ceux des pyramides de bases associées
     * Cela ne posséde pour le moment aucun intérêt pour une pyramide normale
     * ou une pyramide à la demande
     * \~english \brief If a pyramid is onDemand
     * This table contain the association between the levels of this pyramid
     * and the level of the basedPyramids
     */
    std::map<std::string, std::map<std::string, std::string> > aLevel;


    /**
     * \~french \brief Indique si la pyramide doit être générée à la volée
     * Dans ce cas, elle doit nécessairement être à la demande
     * \~english \brief Indicate if the pyramid must be generated on the fly
     * In this case, it must be onDemand
     */
    bool onFly;

    /**
     * \~french \brief Teste si deux CRS sont équivalent
     * \param[in] CRS1
     * \param[in] CRS2
     * \param[in] listofequalsCRS liste des CRS équivalents
     * \~english \brief Test if two CRS are equal
     * \param[in] CRS1
     * \param[in] CRS2
     * \param[in] listofequalsCRS
     */
    bool are_the_two_CRS_equal( std::string crs1, std::string crs2, std::vector<std::string> listofequalsCRS );

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
    Level* getHighestLevel() {
        return highestLevel;
    }

    /**
     * \~french \brief Récupère le plus bas niveau
     * \return level plus bas niveau
     * \~english \brief Get the lowest level
     * \return level lowest level
     */
    Level* getLowestLevel() {
        return lowestLevel;
    }

    /**
     * \~french \brief Récupère le TMS
     * \return TileMatrixSet
     * \~english \brief Get the TMS
     * \return TileMatrixSet
     */
    TileMatrixSet getTms();

    /**
     * \~french \brief Récupère les niveaux
     * \return Liste de level
     * \~english \brief Get the levels
     * \return List of level
     */
    std::map<std::string, Level*>& getLevels() {
        return levels;
    }
    /**
     * \~french \brief Attribue les niveaux
     * \~english \brief Set the levels
     */
    void setLevels(std::map<std::string, Level*>& lv) {
        levels = lv;
    }
    /**
     * \~french \brief Récupère le format
     * \return format
     * \~english \brief Get the format
     * \return format
     */
    Rok4Format::eformat_data getFormat() {
        return format;
    }

    /**
     * \~french \brief Récupère le nombre de canaux d'une tuile
     * \return nombre de canaux
     * \~english \brief Get the number of channels of a tile
     * \return number of channels
     */
    int getChannels() {
        return channels;
    }

    /**
     * \~french \brief Indique si la pyramide est à la demande
     * \return onDemand
     * \~english \brief Indicate if the pyramid is onDemand
     * \return onDemand
     */
    bool getOnDemand(){
        return onDemand;
    }

    /**
     * \~french \brief Modifie le paramètre onDemand
     * \param[in] booleen
     * \~english \brief Modify onDemand
     * \param[in] boolean
     */
    void setOnDemand (bool od) {
        onDemand = od;
    }

    /**
     * \~french \brief Récupère la transparence
     * \return booleen
     * \~english \brief Get the transparency
     * \return boolean
     */
    bool getTransparent(){
        return transparent;
    }
    /**
     * \~french \brief Modifie la transparence
     * \param[in] booleen
     * \~english \brief Set the transparency
     * \param[in] boolean
     */
    void setTransparent (bool tr) {
        transparent = tr;
    }

    /**
     * \~french \brief Récupère le style
     * \return style
     * \~english \brief Get the style
     * \return style
     */
    Style *getStyle(){
        return style;
    }

    /**
     * \~french \brief Modifie le style
     * \param[in] style
     * \~english \brief Set the style
     * \param[in] style
     */
    void setStyle (Style * st) {
        style = st;
    }

    /**
     * \~french \brief Récupère la liste des pyramides de base
     * \return Liste de pyramides de base
     * \~english \brief Get the based pyramids list
     * \return List of based pyramids
     */
    std::vector<Pyramid*> getBPyramids(){
        return basedPyramids;
    }

    /**
     * \~french \brief Modifie la liste des pyramides de base
     * \param[in] Liste de pyramides de base
     * \~english \brief Set the list of based pyramids
     * \param[in] List of based pyramids
     */
    void setBPyramids (std::vector<Pyramid*> bp) {
        basedPyramids = bp;
    }

    /**
     * \~french \brief Récupère les niveaux associés
     * \return Liste des niveaux associés
     * \~english \brief Get the associated levels
     * \return List of associated levels
     */
    std::map<std::string, std::map<std::string, std::string> > getALevel(){
        return aLevel;
    }

    /**
     * \~french \brief Modifie la liste des niveaux associés
     * \param[in] Liste des niveaux associés
     * \~english \brief Set the associated levels
     * \param[in] List of associated levels
     */
    void setALevel (std::map<std::string, std::map<std::string, std::string> > aL) {
        aLevel = aL;
    }

    /**
     * \~french \brief Récupère les niveaux associés
     * \return Liste des niveaux associés
     * \~english \brief Get the associated levels
     * \return List of associated levels
     */
    std::map<std::string,std::vector<Pyramid*> >  getSPyramids(){
        return specificPyramids;
    }

    /**
     * \~french \brief Modifie la liste des niveaux associés
     * \param[in] Liste des niveaux associés
     * \~english \brief Set the associated levels
     * \param[in] List of associated levels
     */
    void setSPyramids (std::map<std::string,std::vector<Pyramid*> >  sP) {
        specificPyramids = sP;
    }

    /**
     * \~french \brief Indique si la pyramide est à la volée
     * \return onFly
     * \~english \brief Indicate if the pyramid is onFly
     * \return onFly
     */
    bool getOnFly(){
        return onFly;
    }

    /**
     * \~french \brief Modifie le paramètre onFly
     * \param[in] booleen
     * \~english \brief Modify onFly
     * \param[in] boolean
     */
    void setOnFly (bool of) {
        if (of) {
            if (onDemand) {
                onFly = of;
            }
        } else {
            if (!onDemand) {
                onFly = of;
            }
        }

    }

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
     * \~french \brief Récupère une tuile déjà calculée
     * \param[in] x
     * \param[in] y
     * \param[in] ID du TileMatrix
     * \~english \brief Get a tile
     * \param[in] x
     * \param[in] y
     * \param[in] ID of the TileMatrix
     */
    DataSource* getTile ( int x, int y, std::string tmId, DataSource* errorDataSource = NULL );

    /**
     * \~french \brief Récupère une image
     * \~english \brief Get an image
     */
    Image* getbbox (ServicesConf& servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, Interpolation::KernelType interpolation, int& error );

    /**
     * \~french \brief Créé une image reprojetée
     * \~english \brief Create a reprojected image
     */
    Image *createReprojectedImage(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesConf& servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

    /**
     * \~french \brief Renvoit une image de noData
     * \param[in] id du level de base concerné
     * \param[in] bbox de la requête
     * \~english \brief Get a noData image
     * \param[in] id of the based level
     * \param[in] bbox of the request
     */
    Image *NoDataOnDemand(std::string bLevel, BoundingBox<double> bbox);

    /**
     * \~french \brief Constructeur
     * \param[in] levels de la pyramide
     * \param[in] tms
     * \param[in] format des tuiles
     * \param[in] nombre de canaux des tuiles
     * \param[in] onDemand
     * \param[in] onFly
     * \~english \brief Constructor
     * \param[in] levels of the pyramid
     * \param[in] tms
     * \param[in] format of the tiles
     * \param[in] number of channels
     * \param[in] onDemand
     * \param[in] onFly
     */
    Pyramid (std::map<std::string, Level*> &levels, TileMatrixSet tms, Rok4Format::eformat_data format, int channels, bool onDemand, bool onFly);

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~Pyramid();

};



#endif
