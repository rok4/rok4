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

class Level;

#ifndef LEVEL_H
#define LEVEL_H

#include "Image.h"
#include "EmptyImage.h"
#include "BoundingBox.h"
#include "TileMatrix.h"
#include "Data.h"
#include "StoreDataSource.h"
#include "CRS.h"
#include "Format.h"
#include "Interpolation.h"
#include "Context.h"
#include "Source.h"
#include "PyramidXML.h"
#include "LevelXML.h"
#include "ServicesXML.h"
#include "Table.h"

/**
 */

class Level {
private:

    std::string racine;
    Context* context;
    int pathDepth;        //used only for file context
    TileMatrix* tm;
    Rok4Format::eformat_data format; //format d'image des tuiles
    int maxTileSize;
    uint32_t maxTileRow;
    uint32_t minTileRow;
    uint32_t maxTileCol;
    uint32_t minTileCol;
    uint32_t tilesPerWidth;   //nombre de tuiles par dalle dans le sens de la largeur
    uint32_t tilesPerHeight;  //nombre de tuiles par dalle dans le sens de la hauteur

    /******************* PYRAMIDE VECTEUR *********************/
    std::vector<Table> tables;
    
    /******************* PYRAMIDE RASTER (OD) *********************/
    int channels;
    std::vector<Source*> sSources;
    // Sans stockage
    bool onDemand;
    // Avec stockage
    bool onFly;

    int* nodataValue;


    DataSource* getEncodedTile ( int x, int y );
    DataSource* getDecodedTile ( int x, int y );

protected:
    /**
     * Renvoie une image de taille width, height
     *
     * le coin haut gauche de cette image est le pixel offsetx, offsety de la tuile tilex, tilex.
     * Toutes les coordonnées sont entière depuis le coin haut gauche.
     */
    Image* getwindow ( ServicesXML* servicesConf, BoundingBox<int64_t> src_bbox, int& error );

public:
    TileMatrix* getTm() ;
    Rok4Format::eformat_data getFormat() ;
    int getChannels() ;
    uint32_t getMaxTileRow() ;
    uint32_t getMinTileRow() ;
    uint32_t getMaxTileCol() ;
    uint32_t getMinTileCol() ;
    void setMaxTileRow(uint32_t mm ) ;
    void setMinTileRow(uint32_t mm ) ;
    void setMaxTileCol(uint32_t mm ) ;
    void setMinTileCol(uint32_t mm ) ;
    double getRes() ;
    std::string getId() ;
    uint32_t getTilesPerWidth() ;
    uint32_t getTilesPerHeight() ;
    std::vector<Source*> getSources() ;
    std::vector<Table>* getTables() ;

    std::string getPath (int tilex, int tiley );
    std::string getDirPath ( int tilex, int tiley );
    int createDirPath ( std::string path );
    Context* getContext() ;

    bool isOnDemand();
    bool isOnFly();

    Image* getbbox ( ServicesXML* servicesConf, BoundingBox<double> bbox, int width, int height, Interpolation::KernelType interpolation, int& error );

    Image* getbbox ( ServicesXML* servicesConf, BoundingBox<double> bbox, int width, int height, CRS src_crs, CRS dst_crs, Interpolation::KernelType interpolation, int& error );
    /**
     * Renvoie la tuile x, y numéroté depuis l'origine.
     * Le coin haut gauche de la tuile (0,0) est (Xorigin, Yorigin)
     * Les indices de tuiles augmentes vers la droite et vers le bas.
     * Des indices de tuiles négatifs sont interdits
     *
     * La tuile contenant la coordonnées (X, Y) dans le srs d'origine a pour indice :
     * x = floor((X - X0) / (tile_width * resolution_x))
     * y = floor((Y - Y0) / (tile_height * resolution_y))
     */

    DataSource* getTile (int x, int y);

    Image* getTile ( int x, int y, int left, int top, int right, int bottom );

    BoundingBox<double> tileIndicesToSlabBbox(int tileCol, int tileRow);
    BoundingBox<double> tileIndicesToTileBbox(int tileCol, int tileRow);
    BoundingBox<double> TMLimitsToBbox();
    int getSlabHeight();
    int getSlabWidth();

    /** D */
    Level ( LevelXML* l, PyramidXML* p );
    Level ( Level* obj, ServerXML* sxml, TileMatrixSet* tms );

    /*
     * Destructeur
     */
    ~Level();

};

#endif





