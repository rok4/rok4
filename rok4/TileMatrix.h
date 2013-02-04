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
 * \file TileMatrix.h
 * \~french
 * \brief Définition de la classe TileMatrix gérant une matrice de tuile représentant un niveau d'une pyramide (Cf TileMatrixSet)
 * \~english
 * \brief Define the TileMatrix Class handling a matrix of tile reprensenting a level in a pyramid (See TileMatrixSet)
 */

#ifndef TILEMATRIX_H
#define TILEMATRIX_H

#include <string>

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance TileMatrix représente une matrice de tuiles. 
 * Cette matrice décrit un niveau d'une pyramide définit dans un TileMatrixSet
 * 
 * Définition d'un TileMatrix en XML :
 * \brief Gestion d'une matrice de tuile
 * \~english
 * A TileMatrix represent a matrix of tiles. 
 * This matrix is part of a pyramid define in a TileMatrixSet
 * 
 * XML definition of a TileMatrix :
 * \brief Handle a matrix of tiles
 * \details \~ \code{.xml}
 * <tileMatrix>
 *    <id>Lvl 8</id>
 *    <resolution>611.4962262814100</resolution>
 *    <topLeftCornerX>-20037508.3427892480</topLeftCornerX>
 *    <topLeftCornerY>20037508.3427892480</topLeftCornerY>
 *    <tileWidth>256</tileWidth>
 *    <tileHeight>256</tileHeight>
 *    <matrixWidth>256</matrixWidth>
 *    <matrixHeight>256</matrixHeight>
 * </tileMatrix>
 * \endcode
 */
class TileMatrix {
private:
    /**
     * \~french \brief Identifiant
     * \~english \brief Identifier
     */
    std::string id;
    /**
     * \~french \brief Résolution des tuiles
     * \~english \brief Tiles resolution
     */
    double res;
    /**
     * \~french \brief Abcisse du point en haut à gauche dans le système de coordonnées associé.
     * \~english \brief X-coordinate of the top right corner in the linked coordinate system.
     */
    double x0;
    /**
     * \~french \brief Ordonnée du point en haut à gauche dans le système de coordonnées associé.
     * \~english \brief Y-coordinate of the top right corner in the linked coordinate system.
     */
    double y0;
    /**
     * \~french \brief Largeur d'une tuile
     * \~english \brief Tile width
     */
    int tileW;
    /**
     * \~french \brief Longueur d'une tuile
     * \~english \brief Tile height 
     */
    int tileH;
    /**
     * \~french \brief Nombre de tuile dans la matrice en largeur
     * \~english \brief Tile number in the matrix width
     */
    long int matrixW;
    /**
     * \~french \brief Nombre de tuile dans la matrice en longueur
     * \~english \brief Tile number in the matrix height
     */
    long int matrixH;
public:
    /**
     * \~french
     * \brief Crée un TileMatrix à partir des ses éléments constitutifs
     * \param[in] id identifiant
     * \param[in] res résolution des tuiles
     * \param[in] x0 abscisse du point en haut à gauche dans le système de coordonnées associé.
     * \param[in] y0 ordonnée du point en haut à gauche dans le système de coordonnées associé.
     * \param[in] tileW largeur d'une tuile
     * \param[in] tileH longueur d'une tuile
     * \param[in] matrixW nombre de tuile dans la matrice en largeur
     * \param[in] matrixH nombre de tuile dans la matrice en longueur
     * \~english
     * \brief Create a TileMatrix
     * \param[in] id identifier
     * \param[in] res tiles resolution
     * \param[in] x0 x-coordinate of the top right corner in the linked coordinate system.
     * \param[in] y0 y-coordinate of the top right corner in the linked coordinate system.
     * \param[in] tileW tile width
     * \param[in] tileH tile height
     * \param[in] matrixW tile number in the matrix width
     * \param[in] matrixH tile number in the matrix height
     */
    TileMatrix ( std::string id,double res,double x0,double y0,int tileW, int tileH,long int matrixW, long int matrixH ) :
            id ( id ), res ( res ), x0 ( x0 ), y0 ( y0 ), tileW ( tileW ), tileH ( tileH ), matrixW ( matrixW ), matrixH ( matrixH ) {};
     /**
     * \~french
     * Crée un TileMatrix à partir d'un autre 
     * \brief Constructeur de copie
     * \param[in] t TileMatrix à copier
     * \~english
     * Create a TileMatrix from another
     * \brief Copy Constructor
     * \param[in] t TileMatrix to copy
     */
    TileMatrix ( const TileMatrix& t );

    /**
     * \~french
     * \brief Affectation
     * \~english
     * \brief Assignement
     */
    TileMatrix& operator= (TileMatrix const& other);
    /**
     * \~french
     * \brief Test d'egalite de 2 TileMatrix
     * \return true si tous les attributs sont identiques, false sinon
     * \~english
     * \brief Test whether 2 TileMatrix are equals
     * \return true if all their attributes are identical
     */
    bool operator== ( const TileMatrix& other ) const;
    /**
     * \~french
     * \brief Test d'inégalite de 2 TileMatrix
     * \return true s'ils ont un attribut différent, false sinon
     * \~english
     * \brief Test whether 2 TileMatrix are different
     * \return true if one of their attributes is different
     */
    bool operator!= ( const TileMatrix& other ) const;
    /**
     * \~french
     * \brief Retourne l'indentifiant
     * \return identifiant
     * \~english
     * \brief Return the identifier
     * \return identifier
     */
    std::string getId();
    /**
     * \~french
     * \brief Retourne la résolution d'une tuile
     * \return résolution
     * \~english
     * \brief Return the tile's resolution
     * \return resolution
     */
    double getRes();
    /**
     * \~french
     * \brief Retourne l'abscisse du point en haut à gauche dans le système de coordonnées associé.
     * \return abscisse 
     * \~english
     * \brief Return the x-coordinate of the top right corner in the linked coordinate system.
     * \return x-coordinate
     */
    double getX0();
    /**
     * \~french
     * \brief Retourne l'ordonnée du point en haut à gauche dans le système de coordonnées associé.
     * \return ordonnée
     * \~english
     * \brief Return the y-coordinate of the top right corner in the linked coordinate system.
     * \return y-coordinate
     */
    double getY0();
    /**
     * \~french
     * \brief Retourne la largeur d'une tuile
     * \return largeur
     * \~english
     * \brief Return the tile width
     * \return width
     */
    int getTileW();
    /**
     * \~french
     * \brief Retourne la longueur d'une tuile
     * \return longueur
     * \~english
     * \brief Return the tile height
     * \return height
     */
    int getTileH();
    /**
     * \~french
     * \brief Retourne le nombre de tuile dans la largeur de la matrice
     * \return nombre de tuile en largeur
     * \~english
     * \brief Return the number of tile in the matrix width
     * \return number of tile in width
     */
    long int getMatrixW();
    /**
     * \~french
     * \brief Retourne le nombre de tuile dans la longueur de la matrice
     * \return nombre de tuile en longueur
     * \~english
     * \brief Return the number of tile in the matrix height
     * \return number of tile in height
     */
    long int getMatrixH();
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~TileMatrix();
};

#endif /* TILEMATRIX_H_ */
