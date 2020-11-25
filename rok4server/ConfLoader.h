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
 * \file ConfLoader.h
 * \~french
 * \brief Définition des fonctions de chargement de la configuration
 * \~english
 * \brief Define configuration loader functions
 */

#ifndef CONFLOADER_H_
#define CONFLOADER_H_

#include <vector>
#include <string>

#include "intl.h"
#include "config.h"

#include "ServerXML.h"
#include "ServicesXML.h"

#include "TileMatrixSet.h"
#include "TileMatrixSetXML.h"

#include "Style.h"
#include "StyleXML.h"
#include "Layer.h"
#include "LayerXML.h"

#include "WebService.h"

/**
 * \author Institut national de l'information géographique et forestière
 * \~french
 * Collection de fonctions gérant le chargement des configurations du serveur
 * \brief Chargement des configurations
 * \~english
 * Function library to load server configuration file
 * \brief Load configuration
 */
class  ConfLoader {
public:
#ifdef UNITTEST
    friend class CppUnitConfLoaderStyle;
    friend class CppUnitConfLoaderTMS;
    friend class CppUnitConfLoaderPyramid;
    friend class CppUnitConfLoaderTechnicalParam;
#endif //UNITTEST
    /**
     * \~french
     * \brief Chargement des paramètres du serveur à partir d'un fichier
     * \param[in] serverConfigFile Nom du fichier d'origine, utilisé comme identifiant
     * \return un objet ServerXML, contenant toutes les informations (NULL sinon)
     * \~english
     * \brief Load server parameter from a file
     * \param[in] serverConfigFile original filename, used as identifier
     * \return a ServerXML object, containing all informations, NULL if failure
     */
    static ServerXML* buildServerConf (std::string serverConfigFile);
    /**
     * \~french
     * \brief Charges les différents Styles présent dans le répertoire styleDir
     * \param[in] styleDir chemin du répertoire contenant les fichiers de Style
     * \param[out] stylesList ensemble des Styles disponibles
     * \param[in] inspire définit si les règles de conformité INSPIRE doivent être utilisées
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] styleDir path to Style directory
     * \param[out] stylesList set of available Styles.
     * \param[in] inspire whether INSPIRE validity rules are enforced
     * \return false if something went wrong
     */
    static bool buildStylesList ( ServerXML* serverXML, ServicesXML* servicesXML);

    /**
     * \~french
     * \brief Création d'un Style à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \param[in] inspire définit si les règles de conformité INSPIRE doivent être utilisées
     * \return un pointeur vers le Style nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Style from a file
     * \param[in] fileName filename, used as identifier
     * \param[in] inspire whether INSPIRE validity rules are enforced
     * \return pointer to the newly created Style, NULL if something went wrong
     */
    static Style* buildStyle ( std::string fileName, ServicesXML* servicesXML);

    /**
     * \~french
     * \brief Charges les différents TileMatrixSet présent dans le répertoire tmsDir
     * \param[in] tmsDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[out] tmsList ensemble des TileMatrixSets disponibles
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] tmsDir path to TileMatrixSet directory
     * \param[out] tmsList set of available TileMatrixSets
     * \return false if something went wrong
     */
    static bool buildTMSList ( ServerXML* serverXML);

    /**
     * \~french
     * \brief Création d'un TileMatrixSet à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \return un pointeur vers le TileMatrixSet nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new TileMatrixSet from a file
     * \param[in] fileName filename, used as identifier
     * \return pointer to the newly created TileMatrixSet, NULL if something went wrong
     */
    static TileMatrixSet* buildTileMatrixSet ( std::string fileName );

    /**
     * \~french
     * \brief Charges les différents Layers présent dans le répertoire layerDir
     * \param[in] layerDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[in] tmsList ensemble des TileMatrixSets disponibles
     * \param[in] stylesList ensemble des Styles disponibles
     * \param[out] layers ensemble des Layers disponibles
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales des services
     * \param[in] proxy
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] layerDir path to TileMatrixSet directory
     * \param[in] tmsList set of available TileMatrixSets
     * \param[in] stylesList set of available Styles
     * \param[out] layers set of available Layers
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global services configuration pointer
     * \param[in] proxy
     * \return false if something went wrong
     */
    static bool buildLayersList (ServerXML* serverXML, ServicesXML* servicesXML );

    /**
     * \~french
     * \brief Création d'un Layer à partir d'un fichier
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSets connus
     * \param[in] stylesList liste des Styles connus
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales du services
     * \param[in] proxy
     * \return un pointeur vers le Layer nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Layer from a file
     * \param[in] fileName original filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \param[in] stylesList known Styles
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global service configuration pointer
     * \param[in] proxy
     * \return pointer to the newly created Layer, NULL if something went wrong
     */
    static Layer * buildLayer (std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML );


    /**
     * \~french
     * \brief Chargement des paramètres des services à partir d'un fichier
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \return un pointeur vers le ServicesConf nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Load service parameters from a file
     * \param[in] fileName original filename, used as identifier
     * \return pointer to the newly created ServicesConf, NULL if something went wrong
     */
    static ServicesXML* buildServicesConf ( std::string servicesConfigFile );

    
    /**
     * \~french
     * \brief Vérifie que le CRS ou un équivalent se trouve dans la liste des CRS autorisés
     * \~english
     * \brief Check if the CRS or an equivalent is in the allowed CRS list
     */
    static bool isCRSAllowed(std::vector<std::string> restrictedCRSList, std::string crs, std::vector<CRS> equiCRSList);
    
    /**
     * \~french
     * \brief Retourne la liste des CRS équivalents et valable dans proj4
     * \~english
     * \brief Return the list of the equivalents CRS who are Proj4 compatible
     */
    static std::vector<CRS> getEqualsCRS(std::vector<std::string> listofequalsCRS, std::string crs);

    /**
     * \~french
     * \brief Chargement de la liste des CRS équivalents à partir du fichier listofequalscrs.txt dans le dossier Proj
     * \~english
     * \brief Load equivalents CRS list from listofequalscrs.txt file in Proj directory
     */
    static std::vector<std::string> loadListEqualsCRS();

    /**
     * \~french
     * \brief Chargement d'une liste à partir d'un fichier
     * \~english
     * \brief Load strings list form file
     */
    static std::vector<std::string> loadStringVectorFromFile(std::string file);

    /**
     * \~french
     * \brief Création d'une Pyramide à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSet connus
     * \param[in] tmsList liste des TileMatrixSets connus
     * \param[in] times vrai si premier appel, faux sinon
     * \param[in] proxy
     * \return un pointeur vers la Pyramid nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Create a new Pyramid from a file
     * \param[in] fileName filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \param[in] times true if first call, false in other cases
     * \param[in] stylesList available style list
     * \param[in] proxy
     * \return pointer to the newly created Pyramid, NULL if something went wrong
     */
    static Pyramid* buildPyramid (std::string fileName, ServerXML* serverXML, ServicesXML* servicesXML, bool times);
    /**
    * \~french
    * \brief Retourne une pyramide en fonction des paramètres lus dans la configuration
    * \~english
    * \brief Return a Pyramid from the configuration
    */
    static Pyramid* buildBasedPyramid( TiXmlElement* pElemBP, ServerXML* serverXML, ServicesXML* servicesXML, std::string levelOD, TileMatrixSet* tmsOD, std::string parentDir);


    /**
    * \~french
    * \brief Retourne un WebService en fonction des paramètres lus dans la configuration
    * \~english
    * \brief Return a WebService from the configuration
    */
    static WebService *parseWebService(TiXmlElement* sWeb, CRS pyrCRS, Rok4Format::eformat_data pyrFormat, Proxy proxy_default, ServicesXML* servicesXML );
    /**
     * \~french
     * \brief Test l'existence d'un fichier
     * \~english
     * \brief Test a file
     */
    static bool doesFileExist(std::string file);
    /**
     * \~french
     * \brief Récupère le nom d'un fichier
     * \~english
     * \brief Get file name
     */
    static std::string getFileName(std::string file, std::string extension);
    /**
    * \~french
    * \brief Retourne la date de derniere modification d'un fichier
    * \~english
    * \brief Return the last modification date of a file
    */
    static time_t getLastModifiedDate(std::string file);

    /**
    * \~french
    * \brief Retourne le contenu d'une balise xml
    * \~english
    * \brief Return the content of a xml tag
    */
    static std::string getTagContentOfFile(std::string file, std::string tag);

    /**
    * \~french
    * \brief Retourne la liste des fichiers dans le dossier
    * \~english
    * \brief Return list of file in a directory
    */
    static std::vector<std::string> listFileFromDir(std::string directory, std::string extension);

};

#endif /* CONFLOADER_H_ */
