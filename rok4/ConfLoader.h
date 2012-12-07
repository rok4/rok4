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
#include "ServicesConf.h"
#include "Style.h"
#include "Layer.h"
#include "TileMatrixSet.h"
#include "tinyxml.h"


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
    friend class CppUnitConfLoaderServicesConf;
    friend class CppUnitConfLoaderTechnicalParam;
#endif //UNITTEST
    /**
     * \~french
     * \brief Chargement des paramètres du serveur à partir d'un fichier
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[out] logOutput type d'enregistreur d'évenement
     * \param[out] logFilePrefix préfixe du fichier contenant le journal des évènements
     * \param[out] logFilePeriod période de rotation des fichiers du journal
     * \param[out] logLevel niveau de criticité des évènements
     * \param[out] nbThread nombre de processus léger concurrent
     * \param[out] supportWMTS support des requêtes de type WMTS
     * \param[out] supportWMS support des requêtes de type WMS
     * \param[out] reprojectionCapability support des requêtes avec reprojection
     * \param[out] servicesConfigFile chemin du fichier de configuration du service
     * \param[out] layerDir chemin du répertoire contenant les fichiers de Layer
     * \param[out] tmsDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[out] styleDir chemin du répertoire contenant les fichiers de Style
     * \param[out] socket adresse et port d'écoute du serveur, vide si définit par un appel FCGI
     * \param[out] backlog profondeur de la file d'attente
     * \return faux en cas d'erreur
     * \~english
     * \brief Load server parameter from a file
     * \param[in] fileName original filename, used as identifier
     * \param[out] logOutput type of logger
     * \param[out] logFilePrefix log file prefix
     * \param[out] logFilePeriod log file rotation period
     * \param[out] logLevel log criticity level
     * \param[out] nbThread concurrent thread number
     * \param[out] supportWMTS support WMTS request
     * \param[out] supportWMS support WMS request
     * \param[out] reprojectionCapability support request with reprojection
     * \param[out] serverConfigFile path to service configuration file
     * \param[out] layerDir path to Layer directory
     * \param[out] tmsDir path to TileMatrixSet directory
     * \param[out] styleDir path to Style directory
     * \param[out] socket listening address and port, empty if defined by a FCGI call
     * \param[out] backlog listen queue depth
     * \return false if something went wrong
     */
    static bool getTechnicalParam ( std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int &nbThread, bool& supportWMTS, bool& supportWMS, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir, std::string& socket, int& backlog);
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
    static bool buildStylesList ( std::string styleDir, std::map<std::string,Style*> &stylesList,bool inspire );
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
    static bool buildTMSList ( std::string tmsDir,std::map<std::string, TileMatrixSet*> &tmsList );
    /**
     * \~french
     * \brief Charges les différents Layers présent dans le répertoire layerDir
     * \param[in] layerDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[in] tmsList ensemble des TileMatrixSets disponibles
     * \param[in] stylesList ensemble des Styles disponibles
     * \param[out] layers ensemble des Layers disponibles
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales du services
     * \return faux en cas d'erreur
     * \~english
     * \brief Load Styles from the styleDir directory
     * \param[in] layerDir path to TileMatrixSet directory
     * \param[in] tmsList set of available TileMatrixSets
     * \param[in] stylesList set of available Styles
     * \param[out] layers set of available Layers
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global service configuration pointer
     * \return false if something went wrong
     */
    static bool buildLayersList ( std::string layerDir,std::map<std::string, TileMatrixSet*> &tmsList, std::map<std::string,Style*> &stylesList, std::map<std::string,Layer*> &layers, bool reprojectionCapability, ServicesConf* servicesConf );
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
    static ServicesConf * buildServicesConf ( std::string servicesConfigFile );

private:
    /**
     * \~french
     * \brief Création d'un Style à partir de sa représentation XML
     * \param[in] doc Racine du document XML
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[in] inspire définit si les règles de conformité INSPIRE doivent être utilisées
     * \return un pointeur vers le style nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Style from its XML representation
     * \param[in] doc XML root
     * \param[in] fileName original filename, used as identifier
     * \param[in] inspire whether INSPIRE validity rules are enforced
     * \return pointer to the newly created Style, NULL if something went wrong
     */
    static Style* parseStyle ( TiXmlDocument* doc,std::string fileName,bool inspire );
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
    static Style* buildStyle ( std::string fileName,bool inspire );
    /**
     * \~french
     * \brief Création d'un TileMatrixSet à partir de sa représentation XML
     * \param[in] doc Racine du document XML
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \return un pointeur vers le TileMatrixSet nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new TileMatrixSet from its XML representation
     * \param[in] doc XML root
     * \param[in] fileName original filename, used as identifier
     * \return pointer to the newly created TileMatrixSet, NULL if something went wrong
     */
    static TileMatrixSet* parseTileMatrixSet ( TiXmlDocument* doc,std::string fileName );
    /**
     * \~french
     * \brief Création d'un TileMatrixSet à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \return un pointeur vers le style nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new TileMatrixSet from a file
     * \param[in] fileName filename, used as identifier
     * \return pointer to the newly created TileMatrixSet, NULL if something went wrong
     */
    static TileMatrixSet* buildTileMatrixSet ( std::string fileName );
    /**
     * \~french
     * \brief Création d'une Pyramide à partir de sa représentation XML
     * \param[in] doc Racine du document XML
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSets connus
     * \return un pointeur vers la Pyramid nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Create a new Pyramid from its XML representation
     * \param[in] doc XML root
     * \param[in] fileName original filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \return pointer to the newly created Pyramid, NULL if something went wrong
     */
    static Pyramid* parsePyramid ( TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList );
    /**
     * \~french
     * \brief Création d'une Pyramide à partir d'un fichier
     * \param[in] fileName Nom du fichier, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSet connus
     * \return un pointeur vers la Pyramid nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Create a new Pyramid from a file
     * \param[in] fileName filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \return pointer to the newly created Pyramid, NULL if something went wrong
     */
    static Pyramid* buildPyramid ( std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList );
    /**
     * \~french
     * \brief Création d'un Layer à partir de sa représentation XML
     * \param[in] doc Racine du document XML
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSets connus
     * \param[in] stylesList liste des Styles connus
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales du services
     * \return un pointeur vers le Layer nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Layer from its XML representation
     * \param[in] doc XML root
     * \param[in] fileName original filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \param[in] stylesList known Styles
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global service configuration pointer
     * \return pointer to the newly created Layer, NULL if something went wrong
     */
    static Layer * parseLayer ( TiXmlDocument* doc,std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability,ServicesConf* servicesConf );
    /**
     * \~french
     * \brief Création d'un Layer à partir d'un fichier
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[in] tmsList liste des TileMatrixSets connus
     * \param[in] stylesList liste des Styles connus
     * \param[in] reprojectionCapability définit si le serveur est capable de reprojeter des données
     * \param[in] servicesConf pointeur vers les configurations globales du services
     * \return un pointeur vers le Layer nouvellement instancié, NULL en cas d'erreur
     * \~english
     * \brief Create a new Layer from a file
     * \param[in] fileName original filename, used as identifier
     * \param[in] tmsList known TileMatrixSets
     * \param[in] stylesList known Styles
     * \param[in] reprojectionCapability whether the server can handle reprojection
     * \param[in] servicesConf global service configuration pointer
     * \return pointer to the newly created Layer, NULL if something went wrong
     */
    static Layer * buildLayer ( std::string fileName, std::map<std::string, TileMatrixSet*> &tmsList,std::map<std::string,Style*> stylesList , bool reprojectionCapability,ServicesConf* servicesConf );
    /**
     * \~french
     * \brief Chargement des paramètres du serveur à partir de sa représentation XML
     * \param[in] doc Racine du document XML
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \param[out] logOutput type d'enregistreur d'évenement
     * \param[out] logFilePrefix préfixe du fichier contenant le journal des évènements
     * \param[out] logFilePeriod période de rotation des fichiers du journal
     * \param[out] logLevel niveau de criticité des évènements
     * \param[out] nbThread nombre de processus léger concurrent
     * \param[out] supportWMTS support des requêtes de type WMTS
     * \param[out] supportWMS support des requêtes de type WMS
     * \param[out] reprojectionCapability support des requêtes avec reprojection
     * \param[out] servicesConfigFile chemin du fichier de configuration du service
     * \param[out] layerDir chemin du répertoire contenant les fichiers de Layer
     * \param[out] tmsDir chemin du répertoire contenant les fichiers de TileMatrixSet
     * \param[out] styleDir chemin du répertoire contenant les fichiers de Style
     * \param[out] socket adresse et port d'écoute du serveur, vide si définit par un appel FCGI
     * \param[out] backlog profondeur de la file d'attente
     * \return faux en cas d'erreur
     * \~english
     * \brief Load server parameter from its XML representation
     * \param[in] doc XML root
     * \param[in] fileName original filename, used as identifier
     * \param[out] logOutput type of logger
     * \param[out] logFilePrefix log file prefix
     * \param[out] logFilePeriod log file rotation period
     * \param[out] logLevel log criticity level
     * \param[out] nbThread concurrent thread number
     * \param[out] supportWMTS support WMTS request
     * \param[out] supportWMS support WMS request
     * \param[out] reprojectionCapability support request with reprojection
     * \param[out] serverConfigFile path to service configuration file
     * \param[out] layerDir path to Layer directory
     * \param[out] tmsDir path to TileMatrixSet directory
     * \param[out] styleDir path to Style directory
     * \param[out] socket listening address and port, empty if defined by a FCGI call
     * \param[out] backlog listen queue depth
     * \return false if something went wrong
     */
    static bool parseTechnicalParam ( TiXmlDocument* doc,std::string serverConfigFile, LogOutput& logOutput, std::string& logFilePrefix, int& logFilePeriod, LogLevel& logLevel, int& nbThread, bool& supportWMTS, bool& supportWMS, bool& reprojectionCapability, std::string& servicesConfigFile, std::string &layerDir, std::string &tmsDir, std::string &styleDir, std::string& socket, int& backlog);
    /**
     * \~french
     * \brief Chargement des paramètres des services à partir de leur représentation XML
     * \param[in] doc Racine du document XML
     * \param[in] fileName Nom du fichier d'origine, utilisé comme identifiant
     * \return un pointeur vers le ServicesConf nouvellement instanciée, NULL en cas d'erreur
     * \~english
     * \brief Load service parameters from their XML representation
     * \param[in] doc XML root
     * \param[in] fileName original filename, used as identifier
     * \return pointer to the newly created ServicesConf, NULL if something went wrong
     */
    static ServicesConf * parseServicesConf ( TiXmlDocument* doc,std::string servicesConfigFile );
};

#endif /* CONFLOADER_H_ */
