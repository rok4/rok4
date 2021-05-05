/*
 * Copyright © (2011) Institut national de l'information
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

#ifndef ASPECTIMAGE_H
#define ASPECTIMAGE_H

#include "Image.h"
#include <string>


class AspectImage : public Image {

private:

    /** \~french
    * \brief Image d'origine utilisée pour calculer l'exposition
    ** \~english
    * \brief Origin image used to compute the aspect
    */
    Image* origImage;

    /** \~french
    * \brief Buffer contenant l'exposition calculée
    ** \~english
    * \brief Buffer of the aspect
    */
    float* aspect;

    /** \~french
    * \brief Buffer temporaire
    ** \~english
    * \brief Temporary buffer
    */
    float* bufferTmp;

    /** \~french
    * \brief Matrice de convolution
    ** \~english
    * \brief Convolution matrix
    */
    float matrix[9];

    /** \~french
    * \brief Résolution de l'image d'origine et donc finale
    ** \~english
    * \brief Resolution of the image
    */
    float resolution;

    /** \~french
    * \brief algo : choix de l'algorithme de calcul d'expositions  par l'utilisateur ("H" pour Horn)
    ** \~english
    * \brief algo : aspect calculation algorithm chosen by the user ("H" for Horn)
    */
    std::string algo;

    /** \~french
    * \brief minSlope : indique la valeur de pente à partir de laquelle l'exposition est calculee
    ** \~english
    * \brief minSlope : indicate the value of slope from which aspect is computed
    */
    float minSlope;




    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    int _getline ( uint8_t* buffer, int line );

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    int _getline ( uint16_t* buffer, int line );

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    int _getline ( float* buffer, int line );

    /** \~french
    * \brief Récupère la ligne dans l'image d'origine
    ** \~english
    * \brief Get line in origin image
    */
    int getOrigLine ( uint8_t* buffer, int line );

    /** \~french
    * \brief Récupère la ligne dans l'image d'origine
    ** \~english
    * \brief Get line in origin image
    */
    int getOrigLine ( uint16_t* buffer, int line );

    /** \~french
    * \brief Récupère la ligne dans l'image d'origine
    ** \~english
    * \brief Get line in origin image
    */
    int getOrigLine ( float* buffer, int line );

    /** \~french
    * \brief Génére l'image de l'exposition
    ** \~english
    * \brief Generate aspect
    */
    void generate();

    /** \~french
    * \brief Génére une ligne de l'image de l'exposition
    ** \~english
    * \brief Generate one line of the aspect
    */
    void generateLine ( int line, float* line1, float* line2 , float* line3 );

public:

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    virtual int getline ( float* buffer, int line );

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    virtual int getline ( uint8_t* buffer, int line );

    /** \~french
    * \brief Récupère la ligne
    ** \~english
    * \brief Get line
    */
    virtual int getline ( uint16_t* buffer, int line );

    /** \~french
    * \brief Constructeur
    ** \~english
    * \brief Construtor
    */
    AspectImage ( int width, int height, int channels, BoundingBox<double> bbox, Image* image, float resolution, std::string algo, float minSlope);

    /** \~french
    * \brief Destructeur
    ** \~english
    * \brief Destructor
    */
    virtual ~AspectImage();

};

#endif // ASPECTIMAGE_H
