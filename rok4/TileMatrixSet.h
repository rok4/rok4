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
 * \file TileMatrix.h
 * \~french
 * \brief Définition de la classe TileMatrixSet gérant une pyramide de matrices (Cf TileMatrix)
 * \~english
 * \brief Define the TileMatrixSet Class handling a pyramid of matrix (See TileMatrix)
 */

#ifndef TILEMATRIXSET_H_
#define TILEMATRIXSET_H_

#include <string>
#include <vector>
#include <map>
#include "TileMatrix.h"
#include "CRS.h"
#include "Keyword.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance TileMatrixSet représente une pyramide de TileMatrix définie dans un même système de coordonnées.
 *
 * Définition d'un TileMatrixSet en XML :
 * \brief Gestion d'une pyramid de matrices de tuiles
 * \~english
 * A TileMatrixSet represent a pyramid of TileMatrix in the same coordinate system.
 *
 * XML definition of a TileMatrix :
 * \brief Handle pyramid of matrix of tiles
 * \details \~ \code{.xml}
 * <tileMatrixSet>
 *     <crs>EPSG:3857</crs>
 *     <tileMatrix>
 *         <id>0</id>
 *         <resolution>156543.0339280410</resolution>
 *         <topLeftCornerX>-20037508.3427892480</topLeftCornerX>
 *         <topLeftCornerY>20037508.3427892480</topLeftCornerY>
 *         <tileWidth>256</tileWidth>
 *         <tileHeight>256</tileHeight>
 *         <matrixWidth>1</matrixWidth>
 *         <matrixHeight>1</matrixHeight>
 *     </tileMatrix>
 *     <tileMatrix>
 *         <id>1</id>
 *         <resolution>78271.51696402048</resolution>
 *         <topLeftCornerX>-20037508.3427892480</topLeftCornerX>
 *         <topLeftCornerY>20037508.3427892480</topLeftCornerY>
 *         <tileWidth>256</tileWidth>
 *         <tileHeight>256</tileHeight>
 *         <matrixWidth>2</matrixWidth>
 *         <matrixHeight>2</matrixHeight>
 *     </tileMatrix>
 *     <tileMatrix>
 *         <id>2</id>
 *         <resolution>39135.75848201023</resolution>
 *         <topLeftCornerX>-20037508.3427892480</topLeftCornerX>
 *         <topLeftCornerY>20037508.3427892480</topLeftCornerY>
 *         <tileWidth>256</tileWidth>
 *         <tileHeight>256</tileHeight>
 *         <matrixWidth>4</matrixWidth>
 *         <matrixHeight>4</matrixHeight>
 *     </tileMatrix>
 * </tileMatrixSet>
 * \endcode
 */
class TileMatrixSet {
private:
    /**
     * \~french \brief Identifiant
     * \~english \brief Identifier
     */
    std::string id;
    /**
     * \~french \brief Titre
     * \~english \brief Title
     */
    std::string title;
    /**
     * \~french \brief Résumé
     * \~english \brief Abstract
     */
    std::string abstract;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keyWords;
    /**
     * \~french \brief Système de coordonnées associé
     * \~english \brief Linked coordinates system
     */
    CRS crs;
    /**
     * \~french \brief Liste des TileMatrix
     * \~english \brief List of TileMatrix
     */
    std::map<std::string, TileMatrix> tmList;
public:
    /**
     * \~french
     * \brief Crée un TileMatrixSet à partir des ses éléments constitutifs
     * \param[in] id identifiant
     * \param[in] title titre
     * \param[in] abstract résumé
     * \param[in] keyWords liste des mots-clés
     * \param[in] crs système de coordonnées associé
     * \param[in] tmList liste des TileMatrix
     * \~english
     * \brief Create a TileMatrixSet
     * \param[in] id identifier
     * \param[in] title title
     * \param[in] abstract abstract
     * \param[in] keyWords list of keywords
     * \param[in] crs linked coordinates systems
     * \param[in] tmList list of TileMatrix
     */
    TileMatrixSet ( std::string id, std::string title, std::string abstract, std::vector<Keyword> & keyWords, CRS& crs, std::map<std::string, TileMatrix> & tmList ) :
        id ( id ), title ( title ), abstract ( abstract ), keyWords ( keyWords ), crs ( crs ), tmList ( tmList ) {};
    /**
     * \~french
     * Crée un TileMatrixSet à partir d'un autre
     * \brief Constructeur de copie
     * \param[in] t TileMatrixSet à copier
     * \~english
     * Create a TileMatrixSet from another
     * \brief Copy Constructor
     * \param[in] t TileMatrixSet to copy
     */
    TileMatrixSet ( const TileMatrixSet& t ) : id ( t.id ),title ( t.title ),abstract ( t.abstract ),keyWords ( t.keyWords ),crs ( t.crs ),tmList ( t.tmList ) {}
    /**
     * \~french
     * La comparaison ignore les mots-clés et les TileMatrix
     * \brief Test d'egalite de 2 TileMatrixSet
     * \return true si tous les attributs sont identiques et les listes de taille identiques, false sinon
     * \~english
     * Rapid comparison of two TileMatrixSet, Keywords and TileMatrix are not verified
     * \brief Test whether 2 TileMatrixSet are equals
     * \return true if attributes are equal and lists have the same size
     */
    bool operator== ( const TileMatrixSet& other ) const;
    /**
     * \~french
     * La comparaison ignore les mots-clés et les TileMatrix
     * \brief Test d'inégalite de 2 TileMatrixSet
     * \return true si tous les attributs sont identiques et les listes de taille identiques, false sinon
     * \~english
     * Rapid comparison of two TileMatrixSet, Keywords and TileMatrix are not verified
     * \brief Test whether 2 TileMatrixSet are different
     * \return true if one of their attribute is different or lists have different size
     */
    bool operator!= ( const TileMatrixSet& other ) const;
    /**
     * \~french
     * \brief Retourne la liste des TileMatrix
     * \return liste de TileMatrix
     * \~english
     * \brief Return the list of TileMatrix
     * \return liste of TileMatrix
     */
    std::map<std::string, TileMatrix>* getTmList();
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
     * \brief Retourne le titre
     * \return titre
     * \~english
     * \brief Return the title
     * \return title
     */
    std::string getTitle() {
        return title;
    }
    /**
     * \~french
     * \brief Retourne le résumé
     * \return résumé
     * \~english
     * \brief Return the abstract
     * \return abstract
     */
    std::string getAbstract() {
        return abstract;
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
     * \brief Retourne le système de coordonnées utilisé
     * \return crs
     * \~english
     * \brief Return the linked coordinates system
     * \return crs
     */
    CRS getCrs() const {
        return crs;
    }

    ///\TODO
    int best_scale ( double resolution_x, double resolution_y );

    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    ~TileMatrixSet() {}
};

#endif /* TILEMATRIXSET_H_ */
