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

#include "ReprojectedImage.h"

#include <string>
#include "Image.h"
#include "Grid.h"
#include "Logger.h"
#include "Kernel.h"

#include "Utils.h"
#include <cmath>


ReprojectedImage::ReprojectedImage(Image *image,  BoundingBox<double> bbox, Grid* grid,  Interpolation::KernelType KT) 
	: Image(grid->width, grid->height,image->channels, bbox), image(image), grid(grid), K(Kernel::getInstance(KT)) {

		double res_x = image->getresx();
		double res_y = image->getresy();

		grid->bbox.print();
		grid->affine_transform(1./res_x, -image->getbbox().xmin/res_x - 0.5, -1./res_y, image->getbbox().ymax/res_y - 0.5);
		grid->bbox.print();
		ratio_x = (grid->bbox.xmax - grid->bbox.xmin) / double(width); 
		ratio_y = (grid->bbox.ymax - grid->bbox.ymin) / double(height);

		LOGGER_DEBUG("ratiox="<<ratio_x<<" ratioy="<<ratio_y<<" width="<<width<<" height="<<height);

		Kx = ceil(2 * K.size(ratio_x));
		Ky = ceil(2 * K.size(ratio_y));

		int sz1 = 4*((image->width*channels + 3)/4);  // nombre d'éléments d'une ligne de l'image source arrondie au multiple de 4 supérieur.
		int sz2 = 4*((width*channels + 3)/4);         // nombre d'éléments d'une ligne de l'image calculée arrondie au multiple de 4 supérieur.
		int sz3 = 4*((width+3)/4);
		int sz4 = 4*((Kx+3)/4);
		int sz5 = 4*((Ky+3)/4);

		int sz = sz1 * image->height * sizeof(float)             // place pour src_line_buffer;
			+ sz2 * 8 * sizeof(float)    // place pour (Ky+4) lignes de resampled_src_line + dst_line_buffer
			+ sz3 * 8 * sizeof(float)
			+ sz4 * (1028 + 4*channels) * sizeof(float)
			+ sz5 * (1028 + 4*channels) * sizeof(float);


		__buffer = (float*) _mm_malloc(sz, 16);  // Allocation allignée sur 16 octets pour SSE
		memset(__buffer, 0, sz);

		float* B = __buffer;

		src_line_buffer = new float*[image->height];
		for(int i = 0; i < image->height; i++) {
			src_line_buffer[i] = B; B += sz1;
		}

		for(int i = 0; i < 4; i++) {
			dst_line_buffer[i] = B; B += sz2;
		}

		mux_dst_line_buffer = B; B += 4*sz2;
		for(int i = 0; i < 4; i++) {
			X[i] = B; B += sz3;
			Y[i] = B; B += sz3;
		}

		dst_line_index = -1;

		for(int i = 0; i < 1024; i++) {
			Wx[i] = B; B += sz4;
			Wy[i] = B; B += sz5;
		}
		WWx = B; B += 4*sz4;
		WWy = B; B += 4*sz5;
		TMP1 = B; B += 4*channels*sz4;
		TMP2 = B; B += 4*channels*sz5;

		for(int i = 0; i < 1024; i++) {
			int nbx = Kx, nby = Ky;
			xmin[i] = K.weight(Wx[i], nbx, 1./2048. + double(i)/1024., ratio_x, image->width);
			ymin[i] = K.weight(Wy[i], nby, 1./2048. + double(i)/1024., ratio_y, image->height);
		}

		// TODO : ne pas charger toute l'image source au démarrage.
		for(int y = 0; y < image->height; y++)
			image->getline(src_line_buffer[y], y);
	}


float* ReprojectedImage::compute_dst_line(int line) {

	if(line/4 == dst_line_index) return dst_line_buffer[line%4];
	dst_line_index = line/4;

	for(int i = 0; i < 4; i++) {
		if(4*dst_line_index+i < height) grid->interpolate_line(4*dst_line_index+i, X[i], Y[i], width);
		else {
			memcpy(X[i], X[0], width*sizeof(float));
			memcpy(Y[i], Y[0], width*sizeof(float));
		}
	}

	int Ix[4], Iy[4];

	for(int x = 0; x < width; x++) {
		for(int i = 0; i < 4; i++) {
			Ix[i] = (X[i][x] - floor(X[i][x])) * 1024;
			Iy[i] = (Y[i][x] - floor(Y[i][x])) * 1024;
		}

		multiplex(WWx, Wx[Ix[0]], Wx[Ix[1]], Wx[Ix[2]], Wx[Ix[3]], Kx);
		multiplex(WWy, Wy[Iy[0]], Wy[Iy[1]], Wy[Iy[2]], Wy[Iy[3]], Ky);

		int y0 = (int)(Y[0][x]) + ymin[Iy[0]];
		int y1 = (int)(Y[1][x]) + ymin[Iy[1]];
		int y2 = (int)(Y[2][x]) + ymin[Iy[2]];
		int y3 = (int)(Y[3][x]) + ymin[Iy[3]];
		int dx0 = ((int)(X[0][x]) + xmin[Ix[0]])*channels;
		int dx1 = ((int)(X[1][x]) + xmin[Ix[1]])*channels;
		int dx2 = ((int)(X[2][x]) + xmin[Ix[2]])*channels;
		int dx3 = ((int)(X[3][x]) + xmin[Ix[3]])*channels;

		for(int j = 0; j < Ky; j++) {
			multiplex_unaligned(TMP1,
					src_line_buffer[y0 + j] + dx0,
					src_line_buffer[y1 + j] + dx1,
					src_line_buffer[y2 + j] + dx2,
					src_line_buffer[y3 + j] + dx3,
					Kx * channels);
			dot_prod(channels, Kx, TMP2 + 4*j*channels, TMP1, WWx);
		}
		dot_prod(channels, Ky, mux_dst_line_buffer + 4*x*channels, TMP2, WWy);
	}
	demultiplex(dst_line_buffer[0], dst_line_buffer[1], dst_line_buffer[2], dst_line_buffer[3], mux_dst_line_buffer, width*channels);
	return dst_line_buffer[line%4]; 
}

int ReprojectedImage::getline(uint8_t* buffer, int line) {
	const float* dst_line = compute_dst_line(line);
	convert(buffer, dst_line, width*channels);
	return width*channels;
}

int ReprojectedImage::getline(float* buffer, int line) {
	const float* dst_line = compute_dst_line(line);
	convert(buffer, dst_line, width*channels);
	return width*channels;
}



