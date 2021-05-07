/*
 * Copyright � (2011) Institut national de l'information
 *                    g�ographique et foresti�re
 *
 * G�oportail SAV <contact.geoservices@ign.fr>
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
 * \file Pente.h
 ** \~french
 * \brief D�finition de la classe Pente
 ** \~english
 * \brief Define class Pente
 */

#ifndef PENTE_H
#define PENTE_H

#include <boost/log/trivial.hpp>

#include <string>


class Pente {

private:

     /** \~french
     * \brief algo : choix de l'algorithme de calcul de pentes par l'utilisateur ("H" pour Horn)
     ** \~english
     * \brief algo : slope calculation algorithm chosen by the user ("H" for Horn)
     */
    std::string algo;

    /** \~french
    * \brief isPente : indique que style est calcul de pente a partir de la donn�e
    ** \~english
    * \brief isPente : indicate that the style is a slope computed from data
    */
    bool isPente;

    /** \~french
    * \brief unit : unit� de la pente
    ** \~english
    * \brief unit : slope unit
    */
    std::string unit;

    /** \~french
    * \brief interpolation : interpolation utilis�e pour r�-�chantilloner les donn�es sources
    ** \~english
    * \brief interpolation : interpolation used for resampling source data
    */
    std::string interpolation;

    /** \~french
    * \brief noData : valeur de noData pour la pente
    ** \~english
    * \brief noData : value of noData for the slope
    */
    int slopeNoData;

    /** \~french
    * \brief noData : valeur de noData pour l'image source
    ** \~english
    * \brief noData : value of noData for the source image
    */
    float imgNoData;

    /** \~french
    * \brief maxSlope : valeur max pour la pente
    ** \~english
    * \brief maxSlope : max value for the slope
    */
    int maxSlope;



public:

    /**
     * \~french
     * \brief Constructeur sans arguments
     * \~english
     * \brief Constructor without arguments
     */
    Pente(): algo ("H"), isPente (false), unit ("degree"), interpolation ("linear"), slopeNoData (0), imgNoData (-99999), maxSlope (90) {

    }

    /**
     * \~french
     * \brief Constructeurs avec des arguments
     * \~english
     * \brief Constructor with arguments
     */
    Pente(std::string a,bool p = false,std::string u = "degree", std::string in = "linear", int nd = 0, float ndi = -99999, int ms = 90) : algo (a), isPente (p), unit (u), interpolation (in), slopeNoData (nd), imgNoData (ndi), maxSlope (ms) {

    }

    /**
     * \~french
     * \brief Destructeur
     * \~english
     * \brief Destructor
     */
    ~Pente() {

    }

    /**
     * \~french
     * \brief Modifie l'algo
     * \~english
     * \brief Set algo
     */
    void setAlgo(std::string n_algo){
        algo = n_algo;
    }

    /**
     * \~french
     * \brief Modifie la pr�sence de la pente
     * \~english
     * \brief Set pente
     */
    void setPente(bool n_pente){
        isPente=n_pente;
    }

    /**
     * \~french
     * \brief Renvoie l'algo
     * \~english
     * \brief Get algo
     */
    std::string getAlgo(){
        return algo;
    }

    /**
     * \~french
     * \brief Renvoie la pr�sence de la pente
     * \~english
     * \brief Get pente
     */
    bool getPente(){
        return isPente;
    }

    /**
     * \~french
     * \brief Modifie unit
     * \~english
     * \brief Set unit
     */
    void setUnit(std::string u){
        unit = u;
    }

    /**
     * \~french
     * \brief Renvoie unit
     * \~english
     * \brief Get unit
     */
    std::string getUnit(){
        return unit;
    }

    /**
     * \~french
     * \brief Modifie interpolation
     * \~english
     * \brief Set interpolation
     */
    void setInterpolation(std::string u){
        interpolation = u;
    }

    /**
     * \~french
     * \brief Renvoie interpolation
     * \~english
     * \brief Get interpolation
     */
    std::string getInterpolation(){
        return interpolation;
    }

    /**
     * \~french
     * \brief Modifie noData de la pente
     * \~english
     * \brief Set noData of the slope
     */
    void setSlopeNoData(int n){
        slopeNoData = n;
    }

    /**
     * \~french
     * \brief Renvoie noData de la pente
     * \~english
     * \brief Get noData of the slope
     */
    int getSlopeNoData(){
        return slopeNoData;
    }

    /**
     * \~french
     * \brief Modifie noData de l'image
     * \~english
     * \brief Set noData of image
     */
    void setImgNoData(float n){
        imgNoData = n;
    }

    /**
     * \~french
     * \brief Renvoie noData de l'image
     * \~english
     * \brief Get noData of image
     */
    float getImgNoData(){
        return imgNoData;
    }

    /**
     * \~french
     * \brief Modifie maxSlope de la pente
     * \~english
     * \brief Set maxSlope of the slope
     */
    void setMaxSlope(int n){
        maxSlope = n;
    }

    /**
     * \~french
     * \brief Renvoie maxSlope de la pente
     * \~english
     * \brief Get maxSlope of the slope
     */
    int getMaxSlope(){
        return maxSlope;
    }


};
#endif

