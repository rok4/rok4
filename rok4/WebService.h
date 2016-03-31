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

#ifndef WEBSERVICE_H
#define WEBSERVICE_H
#include <string>
#include <map>
#include "BoundingBox.h"
#include "curl/curl.h"
#include "Image.h"
#include "Data.h"

struct MemoryStruct {
  uint8_t *memory;
  size_t size;
};


static size_t WriteInMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (uint8_t*)realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    LOGGER_ERROR("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

/**
* @class WebService
* @brief Implementation de WebService utilisable par le serveur
* Cette classe mère contient des informations de base pour requêter un
* service quelconque. Puis elle se spécifie en WMS.
* D'autres services pourront ainsi être ajouté.
*/

class WebService {

protected:

    /**
     * \~french \brief URL du serveur
     * \~english \brief Server URL
     */
    std::string url;

    /**
     * \~french \brief Proxy utilisé
     * \~english \brief Proxy used
     */
    std::string proxy;

    /**
     * \~french \brief noProxy utilisé
     * \~english \brief noProxy used
     */
    std::string noProxy;

    /**
     * \~french \brief Temps d'attente lors de l'envoi d'une requête
     * \~english \brief Waiting time for a request
     */
    int timeout;

    /**
     * \~french \brief Nombre d'essais
     * \~english \brief Number of try
     */
    int retry;

    /**
     * \~french \brief Temps d'attente entre chaque essais
     * \~english \brief Time between each try
     */
    int interval;

    /**
     * \~french \brief Utilisateur pour identification
     * \~english \brief User for identification
     */
    std::string user;

    /**
     * \~french \brief Hash du mot de passe
     * \~english \brief Password hash
     */
    std::string pwd;

    /**
     * \~french \brief Referer
     * \~english \brief Referer
     */
    std::string referer;

    /**
     * \~french \brief User-Agent
     * \~english \brief User-Agent
     */
    std::string userAgent;

public:

    /**
     * \~french \brief Récupère l'url
     * \return url
     * \~english \brief Get the url
     * \return url
     */
    std::string getUrl(){
        return url;
    }

    /**
     * \~french \brief Modifie l'url
     * \param[in] url
     * \~english \brief Set the url
     * \param[in] url
     */
    void setUrl (std::string u) {
        url = u;
    }

    /**
     * \~french \brief Récupère le proxy
     * \return proxy
     * \~english \brief Get the proxy
     * \return proxy
     */
    std::string getProxy(){
        return proxy;
    }

    /**
     * \~french \brief Modifie le proxy
     * \param[in] proxy
     * \~english \brief Set the proxy
     * \param[in] proxy
     */
    void setProxy (std::string u) {
        proxy = u;
    }

    /**
     * \~french \brief Récupère le user
     * \return user
     * \~english \brief Get the user
     * \return user
     */
    std::string getUser(){
        return user;
    }

    /**
     * \~french \brief Modifie le user
     * \param[in] user
     * \~english \brief Set the user
     * \param[in] user
     */
    void setUser (std::string u) {
        user = u;
    }

    /**
     * \~french \brief Récupère le pwd
     * \return pwd
     * \~english \brief Get the pwd
     * \return pwd
     */
    std::string getPassword(){
        return pwd;
    }

    /**
     * \~french \brief Modifie le pwd
     * \param[in] pwd
     * \~english \brief Set the pwd
     * \param[in] pwd
     */
    void setPassword (std::string u) {
        pwd = u;
    }

    /**
     * \~french \brief Récupère le referer
     * \return referer
     * \~english \brief Get the referer
     * \return referer
     */
    std::string getReferer(){
        return referer;
    }

    /**
     * \~french \brief Modifie le referer
     * \param[in] referer
     * \~english \brief Set the referer
     * \param[in] referer
     */
    void setReferer (std::string u) {
        referer = u;
    }

    /**
     * \~french \brief Récupère le userAgent
     * \return userAgent
     * \~english \brief Get the userAgent
     * \return userAgent
     */
    std::string getUserAgent(){
        return userAgent;
    }

    /**
     * \~french \brief Modifie le userAgent
     * \param[in] userAgent
     * \~english \brief Set the userAgent
     * \param[in] userAgent
     */
    void setUserAgent (std::string u) {
        userAgent = u;
    }

    /**
     * \~french \brief Récupère le timeout
     * \return timeout
     * \~english \brief Get the timeout
     * \return timeout
     */
    int getTimeOut(){
        return timeout;
    }

    /**
     * \~french \brief Modifie le timeout
     * \param[in] timeout
     * \~english \brief Set the timeout
     * \param[in] timeout
     */
    void setTimeOut (int u) {
        timeout = u;
    }

    /**
     * \~french \brief Récupère le retry
     * \return retry
     * \~english \brief Get the retry
     * \return retry
     */
    int getRetry(){
        return retry;
    }

    /**
     * \~french \brief Modifie le retry
     * \param[in] retry
     * \~english \brief Set the retry
     * \param[in] retry
     */
    void setRetry (int u) {
        retry = u;
    }

    /**
     * \~french \brief Récupère le interval
     * \return interval
     * \~english \brief Get the interval
     * \return interval
     */
    int getInterval(){
        return interval;
    }

    /**
     * \~french \brief Modifie le interval
     * \param[in] interval
     * \~english \brief Set the interval
     * \param[in] interval
     */
    void setInterval (int u) {
        interval = u;
    }
    /**
     * \~french
     * \brief Récupération des données à partir d'une URL
     * \~english
     * \brief Taking Data from an URL
     */
    RawDataSource * performRequest(std::string request);


    /**
     * \~french \brief Constructeur
     * \~english \brief Constructor
     */
    WebService(std::string url, std::string proxy, std::string noProxy, int retry, int interval, int timeout);

    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    virtual ~WebService();

};

class WebMapService: public WebService {

private:

    /**
     * \~french \brief version du WMS
     * \~english \brief WMS version
     */
    std::string version;

    /**
     * \~french \brief couches
     * \~english \brief layers
     */
    std::string layers;

    /**
     * \~french \brief styles associées à chaque couche
     * \~english \brief styles associated to each layer
     */
    std::string styles;

    /**
     * \~french \brief format de retour de la requête
     * \~english \brief response format
     */
    std::string format;

    /**
     * \~french \brief crs de la requete
     * \~english \brief request crs
     */
    std::string crs;

    /**
     * \~french \brief nombre de canaux d'une image accessible par le wms
     * \~english \brief channels of images accessible by the wms
     */
    int channels;

    /**
     * \~french \brief emprise des données
     * \~english \brief data bbox
     */
    BoundingBox<double> bbox;

    /**
     * \~french \brief Donne les valeurs des noData pour le WebMapService
     * \~english \brief Give the noData value
     */
    std::vector<int> ndValues;

    /**
     * \~french \brief options à ajouter dans la requête
     * \~english \brief options to add in the request
     */
    std::map<std::string,std::string> options;

public:

    /**
     * \~french \brief Récupère le version
     * \return version
     * \~english \brief Get the version
     * \return version
     */
    std::string getVersion(){
        return version;
    }

    /**
     * \~french \brief Modifie le version
     * \param[in] version
     * \~english \brief Set the version
     * \param[in] version
     */
    void setVersion (std::string u) {
        version = u;
    }

    /**
     * \~french \brief Récupère la bbox
     * \return bbox
     * \~english \brief Get the bbox
     * \return bbox
     */
    BoundingBox<double> getBbox(){
        return bbox;
    }

    /**
     * \~french \brief Modifie la bbox
     * \param[in] bbox
     * \~english \brief Set the bbox
     * \param[in] bbox
     */
    void setBbox (BoundingBox<double> u) {
        bbox = u;
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
     * \~french \brief Modifie le paramètre noData
     * \param[in] noData
     * \~english \brief Modify noData
     * \param[in] noData
     */
    void setNdValues (std::vector<int> ndv) {
       ndValues = ndv;
    }
    /**
     * \~french \brief Récupère channels
     * \return channels
     * \~english \brief Get the channels
     * \return channels
     */
    int getChannels(){
        return channels;
    }

    /**
     * \~french \brief Modifie channels
     * \param[in] channels
     * \~english \brief Set the channels
     * \param[in] channels
     */
    void setChannels (int u) {
        channels = u;
    }
    /**
     * \~french \brief Récupère le layers
     * \return layers
     * \~english \brief Get the layers
     * \return layers
     */
    std::string getLayers(){
        return layers;
    }

    /**
     * \~french \brief Modifie le layers
     * \param[in] layers
     * \~english \brief Set the layers
     * \param[in] layers
     */
    void setLayers (std::string u) {
        layers = u;
    }

    /**
     * \~french \brief Récupère le styles
     * \return styles
     * \~english \brief Get the styles
     * \return styles
     */
    std::string getStyles(){
        return styles;
    }

    /**
     * \~french \brief Modifie le styles
     * \param[in] styles
     * \~english \brief Set the styles
     * \param[in] styles
     */
    void setStyles (std::string u) {
        styles = u;
    }

    /**
     * \~french \brief Récupère le format
     * \return format
     * \~english \brief Get the format
     * \return format
     */
    std::string getFormat(){
        return format;
    }

    /**
     * \~french \brief Modifie le format
     * \param[in] format
     * \~english \brief Set the format
     * \param[in] format
     */
    void setFormat (std::string u) {
        format = u;
    }

    /**
     * \~french \brief Récupère le crs
     * \return crs
     * \~english \brief Get the crs
     * \return crs
     */
    std::string getCrs(){
        return crs;
    }

    /**
     * \~french \brief Modifie le crs
     * \param[in] crs
     * \~english \brief Set the crs
     * \param[in] crs
     */
    void setCrs (std::string u) {
        crs = u;
    }

    /**
     * \~french \brief Récupère les options
     * \return options
     * \~english \brief Get the options
     * \return options
     */
    std::map<std::string,std::string> getOptions(){
        return options;
    }

    /**
     * \~french \brief Modifie les options
     * \param[in] options
     * \~english \brief Set the options
     * \param[in] options
     */
    void setOptions (std::map<std::string,std::string> u) {
        options = u;
    }

    /**
     * \~french \brief teste l'existence d'une option
     * \param[in] option
     * \~english \brief Check option
     * \param[in] options name
     */
    bool hasOption (std::string paramName);

    /**
     * \~french \brief Récupère une option
     * \param[in] option
     * \~english \brief Get one option
     * \param[in] option name
     */
    std::string getOption ( std::string paramName );

    /**
     * \~french \brief Récupère une option
     * \param[in] bbox
     * \param[in] width
     * \param[in] height
     * \~english \brief Get one option
     * \param[in] bbox
     * \param[in] width
     * \param[in] height
     */
    std::string createWMSGetMapRequest (BoundingBox<double> bbox, int width, int height );

    /**
     * \~french
     * \brief Creation d'une image à partir d'une URL
     * \~english
     * \brief tCreate an Image from an URL
     */
    Image * createImageFromRequest(int width, int height, BoundingBox<double> askBbox);

    /**
     * \~french
     * \brief Creation d'une image à partir d'une URL
     * Pour les dalles
     * \~english
     * \brief tCreate an Image from an URL
     * Used for slab
     */
    Image * createSlabFromRequest(int width, int height, BoundingBox<double> askBbox);

    /**
     * \~french \brief Constructeur
     * \~english \brief Constructor
     */
    WebMapService(std::string url, std::string proxy, std::string noProxy,int retry, int interval, int timeout, std::string version,std::string layers, std::string styles,std::string format, int channels,
                                 std::string crs, BoundingBox<double> bbox, std::vector<int> ndValues,
                                 std::map<std::string,std::string> options) : WebService(url,proxy,noProxy,retry,interval,timeout),
        version (version), layers (layers), styles (styles), format (format),
        crs (crs), channels (channels), bbox (bbox), ndValues (ndValues),options (options) {}
    /**
     * \~french \brief Destructeur
     * \~english \brief Destructor
     */
    ~WebMapService();

};

#endif // WEBSERVICE_H
