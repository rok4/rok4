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

    //Liste des différents niveaux de la pyramide
    std::map<std::string, Level*> levels;

    //TileMatrixSet des données
    const TileMatrixSet tms;

    //Format des tuiles
    const Rok4Format::eformat_data format;

    //Nombre de canaux pour les tuiles
    const int channels;

    //Référence au niveau le plus haut
    Level* highestLevel;

    //Référence au niveau le plus bas
    Level* lowestLevel;

    //Indique si la pyramide contient des tuiles déjà pré-calculées (false)
    //  ou si elle est à la demande, donc calcule des tuiles sur demande (true)
    bool onDemand;

    //Si une pyramide est à la demande,
    //  alors elle doit pouvoir tirer ses informations d'une autre pyramide
    //  Cet attribut contient donc la liste des pyramides de base utilisées
    //  pour générer la vraie pyramide
    //  Sachant qu'une pyramide de base ne peut pas en contenir à son tour
    std::vector<Pyramid*> basedPyramids;

    //Indique si la pyramide peut avoir de la transparence
    //  utilisé seulement pour une pyramide de base
    //  ne posséde pour le moment aucun intérêt pour une pyramide normale
    //  ou une pyramide à la demande
    bool transparent;

    //Indique si la pyramide a un style
    //  utilisé seulement pour une pyramide de base
    //  ne posséde pour le moment aucun intérêt pour une pyramide normale
    //  ou une pyramide à la demande
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
    std::map<std::string, std::map<std::string, std::string> > aLevel;

    //    DataStream* nodatastream;

    //    std::map<std::string, DataSource*> noDataSources;

    bool are_the_two_CRS_equal( std::string crs1, std::string crs2, std::vector<std::string> listofequalsCRS );

public:

    Level* getFirstLevel();

    Level* getHighestLevel() {
        return highestLevel;
    }

    Level* getLowestLevel() {
        return lowestLevel;
    }

    TileMatrixSet getTms();

    std::map<std::string, Level*>& getLevels() {
        return levels;
    }

    Rok4Format::eformat_data getFormat() {
        return format;
    }

    int getChannels() {
        return channels;
    }

    bool getOnDemand(){
        return onDemand;
    }

    void setOnDemand (bool od) {
        onDemand = od;
    }

    bool getTransparent(){
        return transparent;
    }

    void setTransparent (bool tr) {
        transparent = tr;
    }

    Style *getStyle(){
        return style;
    }

    void setStyle (Style * st) {
        style = st;
    }

    std::vector<Pyramid*> getBPyramids(){
        return basedPyramids;
    }

    void setBPyramids (std::vector<Pyramid*> bp) {
        basedPyramids = bp;
    }

    std::map<std::string, std::map<std::string, std::string> > getALevel(){
        return aLevel;
    }

    void setALevel (std::map<std::string, std::map<std::string, std::string> > aL) {
        aLevel = aL;
    }

    std::string best_level ( double resolution_x, double resolution_y, bool onDemand );

    DataSource* getTile ( int x, int y, std::string tmId, DataSource* errorDataSource = NULL );

    Image* getbbox (ServicesConf& servicesConf, BoundingBox<double> bbox, int width, int height, CRS dst_crs, Interpolation::KernelType interpolation, int& error );

    Image *createReprojectedImage(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesConf& servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

    Image *NoDataOnDemand(std::string bLevel, BoundingBox<double> bbox);

    Pyramid (std::map<std::string, Level*> &levels, TileMatrixSet tms, Rok4Format::eformat_data format, int channels, bool onDemand);

    ~Pyramid();

};



#endif
