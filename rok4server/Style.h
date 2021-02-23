/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
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
 * \file Style.h
 * \~french
 * \brief Définition de la classe Style modélisant les styles
 * \~english
 * \brief Define the Style Class handling style definition
 */

class Style;

#ifndef STYLE_H
#define STYLE_H
#include <string>
#include <vector>
#include "LegendURL.h"
#include "Keyword.h"
#include "Palette.h"
#include "Pente.h"
#include "Estompage.h"
#include "Aspect.h"
#include "Interpolation.h"
#include "StyleXML.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Une instance Style représente la façon d'afficher une couche et la métadonnée associée.
 * Il est possible de définir une table de correspondance valeur/couleur ou un estompage.
 * Un style peut contenir uniquement des métadonnées ou définir plusieurs traitements.
 *
 * Exemple de fichier de style complet :
 * \brief Gestion des styles (affichages et métadonnées)
 * \~english
 * A Style represent the way to display a layer and its associated metadata.
 * Two types of data treatment are available, Lookup table to define a value/colour equivalence and relief shading
 *
 * Style file sample :
 * \brief Style handler (display and metadata)
 * \details \~ \code{.xml}
 * <style>
 *       <Identifier>dem</Identifier>
 *       <Title>Éstompage</Title>
 *       <Abstract>Éstompage 315°</Abstract>
 *       <Title>Relief Shadow</Title>
 *       <Abstract>Relief Shadow with 315°</Abstract>
 *       <Keywords>
 *              <Keyword>MNT</Keyword>
 *              <Keyword>DEM</Keyword>
 *      </Keywords>
 *       <LegendURL format="image/png" xlink:simpleLink="simple" xlink:href="http://www.rok4.com/legend.png" height="100" width="100" minScaleDenominator="0" maxScaleDenominator="30"/>
 *       <palette maxValue="255" rgbContinuous="true" alphaContinuous="true">
 *               <colour value="0">
 *                       <red>0</red>
 *                       <green>0</green>
 *                       <blue>0</blue>
 *                       <alpha>64</alpha>
 *               </colour>
 *               <colour value="255">
 *                      <red>255</red>
 *                      <green>255</green>
 *                       <blue>255</blue>
 *                       <alpha>64</alpha>
 *               </colour>
 *       </palette>
 *       <estompage angle="315" exaggeration="2.5" center="126"/>
 * </style>
 * \endcode
 */
class Style {
private :
    /**
     * \~french \brief Identifiant interne du style
     * \details C'est le nom du fichier de style, sans extension. Il est utilisé dans les descripteurs de couches.
     * \~english \brief Internal style identifier
     * \details It's the style file name without extension. It used in layers' descriptors
     */
    std::string id;
    /**
     * \~french \brief Identifiant WMS/WMTS du style
     * \~english \brief WMS/WMTS style identifier
     */
    std::string identifier;
    /**
     * \~french \brief Liste des titres
     * \~english \brief List of titles
     */
    std::vector<std::string> titles;
    /**
     * \~french \brief Liste des résumés
     * \~english \brief List of abstracts
     */
    std::vector<std::string> abstracts;
    /**
     * \~french \brief Liste des mots-clés
     * \~english \brief List of keywords
     */
    std::vector<Keyword> keywords;
    /**
     * \~french \brief Liste des légendes
     * \~english \brief List of legends
     */
    std::vector<LegendURL> legendURLs;
    /**
     * \~french \brief Table de correspondance (valeur -> couleur)
     * \~english \brief Lookup table (value -> colour)
     */
    Palette palette;
    /**
     * \~french \brief Définit si un calcul de pente doit être appliqué
     * \~english \brief Define wether the server must compute a slope
     */
    Pente pente;
    /**
     * \~french \brief Définit si un calcul d'exposition doit être appliqué
     * \~english \brief Define wether the server must compute an aspect
     */
    Aspect aspect;
    /**
     * \~french \brief Définit si un estompage doit être appliqué
     * \~english \brief Define wether the server must compute a relief shadow
     */
    Estompage estompage;

public:

    /**
    * \~french
    * Crée un Style à partir d'un StyleXML
    * \brief Constructeur
    * \param[in] s StyleXML contenant les informations
    * \~english
    * Create a Style from a StyleXML
    * \brief Constructor
    * \param[in] s StyleXML to get informations
    */
    Style ( const StyleXML& s );

    /**
    * \~french
    * Crée un Style à partir d'un autre
    * \param[in] s Style à cloner
    * \~english
    * Create a Style from another
    * \param[in] s Style to clone
    */
    Style ( Style* obj);

    /**
     * \~french
     * \brief Retourne l'identifiant du style
     * \return id
     * \~english
     * \brief Return the style's identifier
     * \return id
     */
    inline std::string getId() {
        return id;
    }

    /**
     * \~french
     * \brief Retourne l'identifiant public du style
     * \return identifier
     * \~english
     * \brief Return the public style's identifier
     * \return identifier
     */
    inline std::string getIdentifier() {
        return identifier;
    }

    /**
     * \~french
     * \brief Retourne la liste des titres
     * \return titres
     * \~english
     * \brief Return the list of titles
     * \return titles
     */
    inline std::vector<std::string> getTitles() {
        return titles;
    }

    /**
     * \~french
     * \brief Retourne la liste des résumés
     * \return résumés
     * \~english
     * \brief Return the list of abstracts
     * \return abstracts
     */
    inline std::vector<std::string> getAbstracts() {
        return abstracts;
    }

    /**
     * \~french
     * \brief Retourne la liste des mots-clés
     * \return mots-clés
     * \~english
     * \brief Return the list of keywords
     * \return keywords
     */
    inline std::vector<Keyword>* getKeywords() {
        return &keywords;
    }

    /**
     * \~french
     * \brief Retourne la liste des légendes
     * \return légendes
     * \~english
     * \brief Return the list of legends
     * \return legends
     */
    inline std::vector<LegendURL> getLegendURLs() {
        return legendURLs;
    }

    /**
     * \~french
     * \brief Retourne la table de correspondance
     * \return table de correspondance
     * \~english
     * \brief Return the lookup table
     * \return lookup table
     */
    inline Palette* getPalette() {
        return &palette;
    }

    /**
     * \~french
     * \brief Détermine si le style décrit un estompage
     * \return true si oui
     * \~english
     * \brief Determine if the style describe a relief shadows
     * \return true if it does
     */
    inline bool isEstompage() {
        return estompage.isEstompage();
    }

    /**
     * \~french
     * \brief Retourne l'azimuth du soleil
     * \return azimuth
     * \~english
     * \brief Return the sun azimuth
     * \return azimuth
     */
    inline float getZenith() {
        return estompage.getZenith();
    }

    /**
     * \~french
     * \brief Retourne l'éxagération de la pente
     * \return facteur d'éxagération
     * \~english
     * \brief Return the slope exaggeration
     * \return exaggeration factor
     */
    inline float getAzimuth() {
        return estompage.getAzimuth();
    }

    /**
     * \~french
     * \brief Retourne la valeur d'un pixel de pente nulle
     * \return valeur
     * \~english
     * \brief Return the value of a pixel without slope
     * \return value
     */
    inline float getZFactor() {
        return estompage.getZFactor();
    }
    /**
    * \~french
    * \brief Retourne l'interpolation de l'estompage
    * \return interpolation de l'estompage
    * \~english
    * \brief Return the estompage interpolation
    * \return the estompage interpolation
    */
    inline Interpolation::KernelType getInterpolationOfEstompage() {
      return estompage.getInterpolation();
    }
	
     /**
     * \~french
     * \brief Retourne l'algo de la pente
     * \return algo de la pente
     * \~english
     * \brief Return the algorithm
     * \return the algorithm
     */
    inline std::string getAlgoOfPente() {
        return pente.getAlgo();
    }

    /**
    * \~french
    * \brief Retourne l'unité de la pente
    * \return unit de la pente
    * \~english
    * \brief Return the slope unit
    * \return the unit
    */
   inline std::string getUnitOfPente() {
       return pente.getUnit();
   }

    /**
     * \~french
     * \brief Return vrai si le style est une pente
     * \return bool
     * \~english
     * \brief Return true if the style is a slope
     * \return bool
     */
    inline bool isPente() {
        return pente.getPente();
    }

    /**
    * \~french
    * \brief Retourne l'interpolation de la pente
    * \return interpolation de la pente
    * \~english
    * \brief Return the slope interpolation
    * \return the slope interpolation
    */
    inline Interpolation::KernelType getInterpolationOfPente() {
      return Interpolation::fromString(pente.getInterpolation());
    }

    /**
    * \~french
    * \brief Retourne le noData de la pente
    * \return noData de la pente
    * \~english
    * \brief Return the slope noData
    * \return the slope noData
    */
   inline float getSlopeNoDataOfPente() {
       return pente.getSlopeNoData();
   }

   /**
   * \~french
   * \brief Retourne le noData de la pente
   * \return noData de la pente
   * \~english
   * \brief Return the slope noData
   * \return the slope noData
   */
  inline float getImgNoDataOfPente() {
      return pente.getImgNoData();
  }

  /**
  * \~french
  * \brief Retourne le noData de la pente
  * \return noData de la pente
  * \~english
  * \brief Return the slope noData
  * \return the slope noData
  */
 inline int getMaxSlopeOfPente() {
     return pente.getMaxSlope();
 }

    /**
    * \~french
    * \brief Retourne l'algo de l'exposition
    * \return algo de l'exposition
    * \~english
    * \brief Return the algorithm
    * \return the algorithm
    */
   inline std::string getAlgoOfAspect() {
       return aspect.getAlgo();
   }

   /**
   * \~french
   * \brief Retourne le minSlope de l'exposition
   * \return minSlope de l'exposition
   * \~english
   * \brief Return the minSlope
   * \return the minSlope
   */
   inline float getMinSlopeOfAspect() {
       return aspect.getMinSlope();
   }

   /**
    * \~french
    * \brief Return vrai si le style est une exposition
    * \return bool
    * \~english
    * \brief Return true if the style is an aspect
    * \return bool
    */
   inline bool isAspect() {
       return aspect.getAspect();
   }
	
	
    /**
     * \~french
     * \brief Destructeur par défaut
     * \~english
     * \brief Default destructor
     */
    virtual ~Style();
};


#endif // STYLE_H
