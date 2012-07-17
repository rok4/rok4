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

#include "ResampledImage.h"
#include "Logger.h"
#include "Utils.h"
#include <cmath>
#include <cstring>

#include <mm_malloc.h>




ResampledImage::ResampledImage(Image *image, int width, int height, double resx, double resy, double lefttmp, double toptmp, double ratio_x, double ratio_y,  Interpolation::KernelType KT, BoundingBox<double> bbox) :
Image(width, height,resx,resy, image->channels, bbox), image(image) , left(lefttmp), top(toptmp), ratio_x(ratio_x), ratio_y(ratio_y), K(Kernel::getInstance(KT)) {
    
    left += 0.5*ratio_x - 0.5; // Pour prendre en compte que les échantillons 
    top  += 0.5*ratio_y - 0.5; // sont positionnés aux centres des pixels

    Kx = ceil(2 * K.size(ratio_x));
    Ky = ceil(2 * K.size(ratio_y));

    int sz1 = 4*((image->width*channels + 3)/4);  // nombre d'éléments d'une ligne de l'image source arrondie au multiple de 4 supérieur.
    int sz2 = 4*((width*channels + 3)/4);         // nombre d'éléments d'une ligne de l'image calculée arrondie au multiple de 4 supérieur.
    int sz3 = 4*width*Kx;                         // nombre de poids dans Wx
    int sz4 = 4*((width+3)/4);

    int sz = 8 * sz1 * sizeof(float)             // place pour src_line_buffer;
      + sz2 * (Ky+4+4+1) * sizeof(float)    // place pour (Ky+4) lignes de resampled_src_line + dst_line_buffer
      + sz3 * sizeof(float)                 // place pour Wx
      + sz4 * sizeof(int);                  // place pour le tableau xmin

    resampled_line = new float*[Ky+4];
    resampled_line_index = new int[Ky+4];

    __buffer = (float*) _mm_malloc(sz, 16);  // Allocation allignée sur 16 octets pour SSE
    memset(__buffer, 0, sz);

    float* B = (float*) __buffer;

    for(int i = 0; i < 4; i++) {
        src_line_buffer[i] = B; B += sz1;
    }
    mux_src_line_buffer = B; B += 4*sz1;
    mux_resampled_line = B; B += 4*sz2;

    for(int i = 0; i < Ky+4; i++) {
            resampled_line[i] = B; B += sz2;
            resampled_line_index[i] = -1;
    }
    dst_line_buffer = B; B += sz2;

    Wx = B; B += sz3;
    xmin = (int*) B; B += sz4;

    memset(Wx, 0, sz3 * sizeof(float));
    float* W = Wx;
    for(int x = 0; x < width; x++) {
        int nb = Kx;
        xmin[x] = K.weight(W, nb, left + x * ratio_x, ratio_x);     
        for(int i = nb-1; i >= 0; i--) for(int j = 0; j < 4; j++) W[4*i + j] = W[i]; // On copie chaque poids en 4 exemplaires.
        W += 4*Kx;
    }
}



float* ResampledImage::resample_src_line(int line) {
    if(resampled_line_index[line % (Ky+4)] == line) return resampled_line[line % (Ky+4)];
    for(int i = 0; i < 4; i++)
        if(4*(line/4) + i < image->height)
            image->getline(src_line_buffer[i], 4*(line/4) + i);

    multiplex(mux_src_line_buffer, src_line_buffer[0], src_line_buffer[1], src_line_buffer[2], src_line_buffer[3], image->width*image->channels);

    for(int x = 0; x < width; x++) 
        dot_prod(channels, Kx, mux_resampled_line + 4*x*channels, mux_src_line_buffer + 4*xmin[x]*channels, Wx + 4*Kx*x);

    demultiplex(resampled_line[(4*(line/4))%(Ky+4)],
                resampled_line[(4*(line/4)+1)%(Ky+4)],
                resampled_line[(4*(line/4)+2)%(Ky+4)],
                resampled_line[(4*(line/4)+3)%(Ky+4)],
                mux_resampled_line, width*channels);

    for(int i = 0; i < 4; i++) resampled_line_index[(4*(line/4)+i)%(Ky+4)] = 4*(line/4)+i;

    return resampled_line[line % (Ky+4)];
}



int ResampledImage::getline(float* buffer, int line) {
    float weights[Ky];
    int nb_weights = Ky;
    
    int ymin = K.weight(weights, nb_weights, top + line * ratio_y, ratio_y); // On calcule les coefficient d'interpolation
    mult(buffer, resample_src_line(ymin), weights[0], width*channels);
    for(int y = 1; y < nb_weights; y++) {
        add_mult(buffer, resample_src_line(ymin + y), weights[y], width*channels);
    }
    return width*channels;
}

int ResampledImage::getline(uint8_t* buffer, int line) {
	int nb = getline(dst_line_buffer, line);
	convert(buffer, dst_line_buffer, nb);
	return nb;
}


ResampledImage::~ResampledImage() {
	_mm_free(__buffer);
	delete[] resampled_line;
	delete[] resampled_line_index;
	delete image; 
}
