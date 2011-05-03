#include "MirrorImage.h"
#include "Logger.h"
#include "Utils.h"

#ifndef __max
#define __max(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __min
#define __min(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

/**
Creation d'une MirrorImage a partir de 4 images
retourne NULL en cas d erreur
*/

MirrorImage* mirrorImageFactory::createMirrorImage(Image* pImage0, Image* pImage1, Image* pImage2, Image* pImage3)
{
	std::vector<Image*> images;
	images.push_back(pImage0);
	images.push_back(pImage1);
	images.push_back(pImage2);
	images.push_back(pImage3);

	int i;
	for (i=0;i<4;i++)
		if (images[i]!=NULL)
			break;
	if (i==4)
		return NULL;
	int w=images[i]->width;
	int h=images[i]->height;
	int c=images[i]->channels;

	double xmin,xmax,ymin,ymax;
	if (i==0){
		xmin=images[i]->getxmax();
		xmax=images[i]->getxmax()+w*images[i]->getresx();
		ymin=images[i]->getymin();
		ymax=images[i]->getymax();
	}
	else if (i==1){
                xmin=images[i]->getxmin();
                xmax=images[i]->getxmax();
                ymin=images[i]->getymin()-h*images[i]->getresy();
                ymax=images[i]->getymin();
        }
	else if (i==2){
                xmin=images[i]->getxmin()-w*images[i]->getresx();
                xmax=images[i]->getxmin();
                ymin=images[i]->getymin();
                ymax=images[i]->getymax();
        }
	else if (i==3){
                xmin=images[i]->getxmin();
                xmax=images[i]->getxmax();
                ymin=images[i]->getymax();
                ymax=images[i]->getymax()+h*images[i]->getresy();
        }
return NULL;
LOGGER_DEBUG(xmin<<" "<<ymin<<" "<<xmax<<" "<<ymax);
return NULL;
	BoundingBox<double> bbox(xmin,ymin,xmax,ymax);

	bbox.print();
return NULL;

	return new MirrorImage(w,h,c,bbox,images);
}

MirrorImage::MirrorImage(int width, int height, int channels, BoundingBox<double> bbox, std::vector<Image*>& images) : Image(width,height,channels,bbox), images(images)
{
}

int MirrorImage::getline(uint8_t* buffer, int line)
{
	uint32_t line_size=width*channels;
	uint8_t* buf0=new uint8_t[line_size],*buf1=new uint8_t[line_size],*buf2=new uint8_t[line_size],*buf3=new uint8_t[line_size];
	if (images[0]!=NULL)
		images[0]->getline(buf0,line);
	if (images[1]!=NULL)
		images[1]->getline(buf1,height-line-1);
	if (images[2]!=NULL)
		images[2]->getline(buf2,line);
	if (images[3]!=NULL)
		images[3]->getline(buf3,height-line-1);

	int c0,c1,j;
	double r=(double)width/height;
	// Debut de la ligne
	c0=0;
	c1=__min(line, (height-line))*r;
	if (images[0]!=NULL){
		for (j=c0;j<c1;j++)
			memcpy(&buffer[j*channels],&buf0[line_size-j*channels-1],sizeof(uint8_t)*channels);
	}
	else if (line<height/2){
		if (images[1]!=NULL && c1>c0)
			memcpy(buffer,buf1,(c1-c0)*channels);
	}
	else if (images[3]!=NULL && c1>c0)
             memcpy(buffer,buf3,(c1-c0)*channels);

	// Milieu de la ligne
	c0=c1+1;	
	c1=width-c1-1;
	if (c1>c0){
		// Partie haute
		if (line<height/2){
			if (images[1]!=NULL)
				memcpy(&buffer[c0],&buf1[c0],(c1-c0)*channels);
			else if (images[0]!=NULL)
				for (j=c0;j<c1;j++)
					memcpy(&buffer[j*channels],&buf0[line_size-j*channels-1],sizeof(uint8_t)*channels);
			else if (images[2]!=NULL && c1>width/2)
				memcpy(&buffer[(width/2)*channels],&buf2[(width/2)*channels],(c1-width/2)*channels);
		}
		// Partie basse
		else
			if (images[3]!=NULL)
                                memcpy(&buffer[c0],&buf3[c0],(c1-c0)*channels);
                        else if (images[0]!=NULL)
                                for (j=c0;j<c1;j++)
                                        memcpy(&buffer[j*channels],&buf0[line_size-j*channels-1],sizeof(uint8_t)*channels);
                        else if (images[2]!=NULL && c1>width/2)
                                memcpy(&buffer[(width/2)*channels],&buf2[(width/2)*channels],(c1-width/2)*channels);
	}	

	// Fin de la ligne
	c0=c1;
	c1=width-1;
	if (images[2]!=NULL){
                for (j=c0;j<c1;j++)
                        memcpy(&buffer[j*channels],&buf2[line_size-j*channels-1],sizeof(uint8_t)*channels);
        }
        else if (line<height/2){
                if (images[1]!=NULL && c1>c0)
                        memcpy(buffer,buf1,(c1-c0)*channels);
        }
        else if (images[3]!=NULL && c1>c0)
             memcpy(buffer,buf3,(c1-c0)*channels);

        return width*channels;
}

int MirrorImage::getline(float* buffer, int line)
{
        return width*channels;
}

MirrorImage::~MirrorImage()
{
}
