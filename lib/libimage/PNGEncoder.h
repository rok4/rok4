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

#ifndef _PNGENCODER_
#define _PNGENCODER_

#include "Data.h"
#include "Image.h"
#include "zlib.h"
#include "Palette.h"

/** D */
class PNGEncoder : public DataStream {
private:

	uint8_t* linebuffer;

	z_stream zstream;


protected:
	Image *image;
	int line;
	virtual size_t write_IHDR(uint8_t *buffer, size_t size, uint8_t colortype/* = pixel_t::png_colortype*/);
	virtual size_t write_IDAT(uint8_t *buffer, size_t size);
	virtual size_t write_IEND(uint8_t *buffer, size_t size);
	void addCRC(uint8_t *buffer, uint32_t length);
	Palette* palette;

public:
	/** D */
	PNGEncoder(Image* image, Palette* palette=NULL);
	/** D */
	~PNGEncoder();

	/** D */
	size_t read(uint8_t* buffer, size_t size);
	bool eof();

	std::string getType() {return "image/png";}

	int getHttpStatus() {return 200;}
};


/*

static const uint8_t BLACK[3] = {0, 0, 0};
class ColorizePNGEncoder : public PNGEncoder<pixel_gray> {
  private:
  bool transparent;
  uint8_t PLTE[3*256+12];

  size_t write_PLTE(uint8_t *buffer, size_t size);
  size_t write_tRNS(uint8_t *buffer, size_t size);
  public:
  ColorizePNGEncoder(Image<pixel_gray> *image, bool transparent = true, const uint8_t rgb[3] = BLACK);
  ~ColorizePNGEncoder();
  size_t getdata(uint8_t *buffer, size_t size);

};

 */

#endif

