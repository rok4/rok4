#ifndef COMPOUND_IMAGE_H
#define COMPOUND_IMAGE_H

#include "Image.h"
#include <vector>

class CompoundImage : public Image { 
	private:

		static int compute_width (std::vector<std::vector<Image*> > &images);
		static int compute_height(std::vector<std::vector<Image*> > &images);

		std::vector<std::vector<Image*> > images;

		/** Indice y des tuiles courantes */
		int y;

		/** ligne correspondnat au haut des tuiles courantes*/
		int top;

		template<typename T> 
			inline int _getline(T* buffer, int line);

	public:

		/** D */
		int getline(uint8_t* buffer, int line);

		/** D */
		int getline(float* buffer, int line);

		/** D */
		CompoundImage(std::vector< std::vector<Image*> >& images);

		/** D */
		virtual ~CompoundImage();

};

#endif
