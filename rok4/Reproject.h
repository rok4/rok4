#ifndef REPROJECT_H
#define REPROJECT_H

template<int channels> 
class ReprojectedImage : public Image<float, channels> {
private:
	Image<pixel_t> *image;                // image source
	double *gridX, *gridY;
	int step, nbx, nby;

	int dst_index;
	typename pixel_t::data_t *dst_line, *src_data;

	void compute_dst_line(int line);

public:
	typedef typename pixel_t::data_t data_t;

	ReprojectedImage(Image<pixel_t> *image, int width, int height, double *gridX, double *gridY, int step) : Image<pixel_t>(width, height), image(image), gridX(gridX), gridY(gridY), step(step), nbx(2+(width-1)/step), nby(2+(height-1)/step), dst_index(-1) {
		src_data = new typename pixel_t::data_t[image->height*image->linesize*sizeof(data_t)];
		for(int h = 0; h < image->height; h++) image->getline(src_data + h*image->linesize, h); //FIXME : ne pas tout charger en mémoire...
		dst_line = new typename pixel_t::data_t[this->linesize*sizeof(data_t)];
	};
	~ReprojectedImage();
	size_t getline(data_t *buffer, int line, int offset, int nb_pixel);
};

#endif
