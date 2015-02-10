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
 
#include "Line.h"
#include <tgmath.h>

/* ------------------------------------------------------------------------------------------------ */
/* --------------------------------- DÉFINITION DES FONCTIONS ------------------------------------- */
/* ------------------------------------------------------------------------------------------------ */

void Line::alphaBlending ( Line* above ) {
    // Initialisation des pointeurs courants sur les pixels et l'alpha (final et above)
    float* pix = samples;
    float* al = alpha;
    float* pixAb = above->samples;
    float* alAb = above->alpha;
    for ( int i = 0; i < width; i++, pix += 3, pixAb += 3, al++, alAb++ ) {
        if ( *alAb == 0. || ! above->mask[i] ) {
            // Le pixel de la ligne du dessus est transparent, ou n'est pas de la donnée, il ne change donc pas le pixel du dessous.
            continue;
        }

        if ( *al == 0. ) {
            // Le pixel de la ligne du dessous est complètement transparent, le résultat est donc égal au pixel du dessus
            *al = *alAb;
            memcpy(pix, pixAb, 3*sizeof(float));
            continue;
        }

        float a = *alAb + *al * ( 1. - *alAb );

        pix[0] = ( *alAb * pixAb[0] + *al * pix[0] * ( 1 - *alAb ) ) / a;
        pix[1] = ( *alAb * pixAb[1] + *al * pix[1] * ( 1 - *alAb ) ) / a;
        pix[2] = ( *alAb * pixAb[2] + *al * pix[2] * ( 1 - *alAb ) ) / a;
        *al = a;
    }
}

void Line::useMask ( Line* above ) {
    float* pix = samples;
    float* pixAb = above->samples;
    for ( int i = 0; i < width; i++, pix += 3, pixAb += 3 ) {
        if ( above->mask[i] ) {
            alpha[i] = above->alpha[i];
            memcpy(pix, pixAb, 3 * sizeof(float));
        }
    }
}

void Line::multiply ( Line* above ) {
    for ( int i = 0; i < width; i++ ) {
        if ( ! above->mask[i] ) {
            // Le pixel de la ligne du dessus n'est pas de la donnée, il ne change donc pas le pixel du dessous.
            continue;
        }

        alpha[i] *= above->alpha[i];
        samples[3*i] = samples[3*i] * above->samples[3*i] / coeff;
        samples[3*i+1] = samples[3*i+1] * above->samples[3*i+1] / coeff;
        samples[3*i+2] = samples[3*i+2] * above->samples[3*i+2] / coeff;
    }
}


/* --------------------------------- SPÉCIALISATION DE TEMPLATE ----------------------------------- */

// -------------- UINT8

template <>
void Line::store ( uint8_t* imageIn, uint8_t* maskIn, int srcSpp, uint8_t* transparent ) {
    memcpy ( mask, maskIn, width );
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            if ( imageIn[i] == transparent[0] && imageIn[i] == transparent[1] && imageIn[i] == transparent[2] ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = 1.0;
            }
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = (float) imageIn[i];
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            if ( imageIn[2*i] == transparent[0] && imageIn[2*i] == transparent[1] && imageIn[2*i] == transparent[2] ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = ( float ) imageIn[2*i+1] / 255.;
            }
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = (float) imageIn[2*i];
        }
        break;
    case 3:
        convert( samples, imageIn, 3*width );
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( imageIn+3*i, transparent, 3 ) ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = 1.0;
            }
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            convert( samples+i*3, imageIn+i*4, 3 );
            if ( ! memcmp ( imageIn+4*i, transparent, 3 ) ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = ( float ) imageIn[i*4+3] / 255.;
            }
        }
        break;
    }
}

template <>
void Line::store ( uint8_t* imageIn, uint8_t* maskIn, int srcSpp ) {
    memcpy ( mask, maskIn, width );
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            alpha[i] = 1.0;
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = (float) imageIn[i];
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            alpha[i] = ( float ) imageIn[2*i+1] / 255.;
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = (float) imageIn[2*i];
        }
        break;
    case 3:
        convert( samples, imageIn, 3*width );
        for ( int i = 0; i < width; i++ ) {
            alpha[i] = 1.0;
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            convert( samples+i*3, imageIn+i*4, 3 );
            alpha[i] = ( float ) imageIn[i*4+3] / 255.;
        }
        break;
    }
}

// -------------- FLOAT

template <>
void Line::store ( float* imageIn, uint8_t* maskIn, int srcSpp, float* transparent ) {
    memcpy ( mask, maskIn, width );
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
            //if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) ) {
	    if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) || fabsf((*(samples+3*i)-*transparent)/(*transparent))<0.001 ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = 1.0;
            }
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) ) || fabsf((*(samples+3*i)-*transparent)/(*transparent))<0.001 ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = imageIn[2*i+1];
            }
        }
        break;
    case 3:
        memcpy ( samples, imageIn, sizeof ( float ) *3*width );
        for ( int i = 0; i < width; i++ ) {
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) )
	      || (fabsf((*(samples+3*i)-*transparent)/(*transparent))<0.001 && fabsf((*(samples+3*i+1)-*(transparent+1))/(*(transparent+1))<0.001) && fabsf((*(samples+3*i+2)-*(transparent+2))/(*(transparent+2)))<0.001 ) ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = 1.0;
            }
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            memcpy ( samples+i*3, imageIn+i*4, sizeof ( float ) *3 );
            if ( ! memcmp ( samples+3*i, transparent, 3*sizeof ( float ) )
	      || (fabsf((*(samples+3*i)-*transparent)/(*transparent))<0.001 && fabsf((*(samples+3*i+1)-*(transparent+1))/(*(transparent+1))<0.001) && fabsf((*(samples+3*i+2)-*(transparent+2))/(*(transparent+2)))<0.001 ) ) {
                alpha[i] = 0.0;
            } else {
                alpha[i] = imageIn[i*4+3];
            }
        }
        break;
    }
}

template <>
void Line::store ( float* imageIn, uint8_t* maskIn, int srcSpp ) {
    memcpy ( mask, maskIn, width );
    switch ( srcSpp ) {
    case 1:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[i];
            alpha[i] = 1.0;
        }
        break;
    case 2:
        for ( int i = 0; i < width; i++ ) {
            samples[3*i] = samples[3*i+1] = samples[3*i+2] = imageIn[2*i];
            alpha[i] = imageIn[2*i+1];
        }
        break;
    case 3:
        memcpy ( samples, imageIn, sizeof ( float ) *3*width );
        for ( int i = 0; i < width; i++ ) {
            alpha[i] = 1.0;
        }
        break;
    case 4:
        for ( int i = 0; i < width; i++ ) {
            memcpy ( samples+i*3,imageIn+i*4,sizeof ( float ) *3 );
            alpha[i] = imageIn[i*4+3];
        }
        break;
    }
}
