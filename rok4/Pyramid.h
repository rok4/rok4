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
#include "WebService.h"

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

    /**
     * \~french \brief Indique si la pyramide contient des tuiles déjà pré-calculées (false)
     * ou si elle est à la demande, donc calcule des tuiles sur demande (true)
     * \~english \brief Indicate if the pyramid has tiles (false)
     * or if tiles are generated on demand (true)
     */
    bool onDemand;

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
     * \~french \brief Créé une image reprojetée mais complétée par du nodata
     * Les données sont prises dans cropBbox mais complété par du nodata sur bbox
     * \~english \brief Create a reprojected image but completed by nodata
     * Used data on cropBBox but completed by nodata on bbox
     */
    Image *createExtendedCompoundImage(std::string l, BoundingBox<double> bbox, BoundingBox<double> cropBBox,CRS dst_crs, ServicesConf& servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

    /**
     * \~french \brief Créé une dalle
     * \~english \brief Create a slab
     */
    Image *createBasedSlab(std::string l, BoundingBox<double> bbox, CRS dst_crs, ServicesConf& servicesConf, int width, int height, Interpolation::KernelType interpolation, int error);

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
    virtual ~Pyramid();

//    virtual bool isThisLevelSpecific(std::string lv);
//    virtual std::map<std::string, std::map<std::string, std::string> > getALevel();
//    virtual std::vector<Pyramid *> getSourcePyramid( std::string lv,bool sp );
//    virtual std::vector<int> getNdValues();
//    virtual Photometric::ePhotometric getPhotometry();



};

class PyramidOnDemand : public Pyramid {

private:

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
     * \~french \brief Si une pyramide est à la demande par niveau,
     * alors elle doit pouvoir tirer ses informations d'une autre pyramide
     * ou d'un service web, voir plusieurs
     * Cet attribut contient donc la liste des services web requêtés
     * \~english \brief If a pyramid is onDemand by level
     * it must contain a reference to other pyramids which have data
     * or web services
     * This is the list of those web services
     */
    std::map<std::string,std::vector<WebService*> > specificWebServices;

public:

    /**
     * \~french \brief Constructeur
     * \param[in] levels de la pyramide
     * \param[in] tms
     * \param[in] format des tuiles
     * \param[in] nombre de canaux des tuiles
     * \param[in] onDemand
     * \param[in] onFly
     * \param[in] specificPyramids
     * \param[in] specificWebServices
     * \~english \brief Constructor
     * \param[in] levels of the pyramid
     * \param[in] tms
     * \param[in] format of the tiles
     * \param[in] number of channels
     * \param[in] onDemand
     * \param[in] onFly
     * \param[in] specificPyramids
     * \param[in] specificWebServices
     */
    PyramidOnDemand(std::map<std::string, Level*> &levels, TileMatrixSet tms, Rok4Format::eformat_data format,
                    int channels, bool onDemand, bool onFly,
                    std::map<std::string,std::vector<Pyramid*> > sPyramids,
                    std::map<std::string,std::vector<WebService*> > specificWebServices) :
        Pyramid(levels,tms,format,channels,onDemand,onFly),
        specificPyramids (sPyramids), specificWebServices (specificWebServices) {}



    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~PyramidOnDemand();

    /**
     * \~french \brief Récupère les pyramides specifiques
     * \return Liste des pyramides specifiques
     * \~english \brief Get the specific pyramids
     * \return List of specific pyramids
     */
    std::map<std::string,std::vector<Pyramid*> >  getSPyramids(){
        return specificPyramids;
    }

    /**
     * \~french \brief Modifie la liste des pyramides specifiques
     * \param[in] Liste des pyramides specifiques
     * \~english \brief Set the specific pyramids
     * \param[in] List of specific pyramids
     */
    void setSPyramids (std::map<std::string,std::vector<Pyramid*> >  sP) {
        specificPyramids = sP;
    }

    /**
     * \~french \brief Récupère les webservices
     * \return Liste des webservices
     * \~english \brief Get the webservices
     * \return List of webservices
     */
    std::map<std::string,std::vector<WebService*> >  getSWebServices(){
        return specificWebServices;
    }

    /**
     * \~french \brief Modifie la liste des webservices
     * \param[in] Liste des webservices
     * \~english \brief Set the webservices
     * \param[in] List of webservices
     */
    void setSWebSerices (std::map<std::string,std::vector<WebService*> >  sP) {
        specificWebServices = sP;
    }

    /**
     * \~french \brief Informe sur la spécificité d'un level
     * \param[in] level id
     * \return true si spécifique
     * \~english \brief Tell if a level is specific
     * \param[in] level id
     * \return true if specific
     */
    bool isThisLevelSpecific ( std::string lv );

    /**
     * \~french \brief Informe sur la spécificité d'un level
     * \param[in] level id
     * \return true si spécifique
     * \~english \brief Tell if a level is specific
     * \param[in] level id
     * \return true if specific
     */
    bool isThisLevelSpecificFromPyramids ( std::string lv );

    /**
     * \~french \brief Informe sur la spécificité d'un level
     * \param[in] level id
     * \return true si spécifique
     * \~english \brief Tell if a level is specific
     * \param[in] level id
     * \return true if specific
     */
    bool isThisLevelSpecificFromWebServices ( std::string lv );

    /**
     * \~french \brief Renvoit la liste des pyramides de base
     * ATTENTION: ne doit être utilisé que si on est sur qu'il existe
     * \param[in] level id
     * \return pyramids
     * \~english \brief Return the list of based pyramids
     * WARNING: should be used only if it exists
     * \param[in] level id
     * \return pyramids
     */
    std::vector<Pyramid *> getSourcePyramid( std::string lv);

    /**
     * \~french \brief Renvoit la liste des webservices de base
     * ATTENTION: ne doit être utilisé que si on est sur qu'il existe
     * \param[in] level id
     * \return WebServices
     * \~english \brief Return the list of based webservices
     * WARNING: should be used only if it exists
     * \param[in] level id
     * \return WebServices
     */
    std::vector<WebService *> getSourceWebServices( std::string lv);

};



class PyramidOnFly : public PyramidOnDemand {

private:

    /**
     * \~french \brief Donne la photométrie des images de la pyramide
     * Non Obligatoire sauf pour les pyramides avec stockage
     * \~english \brief Give the photometry of images
     * Not Mandatory, only used for pyramid onFly
     */
    Photometric::ePhotometric photo;

    /**
     * \~french \brief Donne les valeurs des noData pour la pyramide
     * Non Obligatoire sauf pour les pyramides avec stockage
     * \~english \brief Give the noData value
     * Not Mandatory, only used for pyramid onFly
     */
    std::vector<int> ndValues;

public:

    /**
     * \~french \brief Constructeur
     * \param[in] levels de la pyramide
     * \param[in] tms
     * \param[in] format des tuiles
     * \param[in] nombre de canaux des tuiles
     * \param[in] onDemand
     * \param[in] onFly
     * \param[in] transparence
     * \param[in] style
     * \~english \brief Constructor
     * \param[in] levels of the pyramid
     * \param[in] tms
     * \param[in] format of the tiles
     * \param[in] number of channels
     * \param[in] onDemand
     * \param[in] onFly
     * \param[in] transparence
     * \param[in] style
     */
    PyramidOnFly(std::map<std::string, Level*> &levels, TileMatrixSet tms, Rok4Format::eformat_data format,
                 int channels, bool onDemand, bool onFly, Photometric::ePhotometric ph, std::vector<int> ndv,
                 std::map<std::string,std::vector<Pyramid*> > sPyramids,
                 std::map<std::string,std::vector<WebService*> > specificWebServices) :
        PyramidOnDemand(levels,tms,format,channels,onDemand,onFly,sPyramids,specificWebServices),
        photo (ph),
        ndValues (ndv) {}


    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~PyramidOnFly() {
    }

    /**
     * \~french \brief Indique la photometrie de la pyramide
     * \return photo
     * \~english \brief Indicate the photometry of the pyramid
     * \return photo
     */
    Photometric::ePhotometric getPhotometry();

    /**
     * \~french \brief Modifie le paramètre onDemand
     * \param[in] booleen
     * \~english \brief Modify onDemand
     * \param[in] boolean
     */
    void setPhotometry (Photometric::ePhotometric ph) {
       photo = ph;
    }

    /**
     * \~french \brief Indique les valeurs de noData
     * \return ndValues
     * \~english \brief Indicate the noData values
     * \return ndValues
     */
    std::vector<int> getNdValues() {
        return ndValues;
    }

    /**
     * \~french \brief Modifie le paramètre onDemand
     * \param[in] booleen
     * \~english \brief Modify onDemand
     * \param[in] boolean
     */
    void setNdValues (std::vector<int> ndv) {
       ndValues = ndv;
    }


};

#endif
