#include <proj_api.h>
#include <pthread.h>

#include "Grid.h"
#include "Logger.h"

#include <algorithm>

static pthread_mutex_t mutex_proj= PTHREAD_MUTEX_INITIALIZER;

Grid::Grid(int width, int height, BoundingBox<double> bbox) : width(width), height(height), bbox(bbox) 
{
  nbx = 2 + (width-1)/step;
  nby = 2 + (height-1)/step;

  double ratio_x = (bbox.xmax - bbox.xmin)/double(width);
  double ratio_y = (bbox.ymax - bbox.ymin)/double(height);
  double left = bbox.xmin + 0.5 * ratio_x;
  double top  = bbox.ymax - 0.5 * ratio_y;
  double stepx = step * ratio_x;
  double stepy = step * ratio_y;
  
  gridX = new double[nbx*nby];
  gridY = new double[nbx*nby];

  for(int y = 0; y < nby; y++) for(int x = 0; x < nbx; x++) {
    gridX[nbx*y + x] = left + x*stepx;
    gridY[nbx*y + x] = top  - y*stepy;
  }
}

Grid::~Grid() {
  delete[] gridX;
  delete[] gridY;
}

void Grid::affine_transform(double Ax, double Bx, double Ay, double By) {
  for(int i = 0; i < nbx*nby; i++) {
    gridX[i] = Ax * gridX[i] + Bx;
    gridY[i] = Ay * gridY[i] + By;
  }
  update_bbox();
}

#include <cstring>
//TODO : cas d'un fichier de conf charge dynamiquement (option -f)
char PROJ_LIB2[1024] = "../config/proj/";
const char *pj_finder2(const char *name) {
  strcpy(PROJ_LIB2 + 15, name);
  return PROJ_LIB2;
}

/**
 * Effectue une reprojection sur les coordonnées de la grille
 * 
 * (GridX[i], GridY[i]) = proj(GridX[i], GridY[i])
 * ou proj est une projection de from_srs vers to_srs
 */

// TODO : la projection est faite 2 fois. essayer de ne la faire faire qu'une fois.

bool Grid::reproject(std::string from_srs, std::string to_srs) {
	LOGGER_DEBUG(from_srs<<" -> " <<to_srs);

  	pthread_mutex_lock (& mutex_proj);

	pj_set_finder( pj_finder2 );
	LOGGER_DEBUG(pj_finder2("test"));

  	projPJ pj_src, pj_dst;  
  	if(!(pj_src = pj_init_plus(  ("+init=" + from_srs +" +wktext" ).c_str()))) {
    		int *err = pj_get_errno_ref();
    		char *msg = pj_strerrno(*err);     
    		LOGGER_DEBUG("erreur d initialisation " << from_srs << " " << msg);
    		pthread_mutex_unlock (& mutex_proj);
    		return false;
  	}
  	if(!(pj_dst = pj_init_plus(  ("+init=" + to_srs +" +wktext" ).c_str()))) {
    		int *err = pj_get_errno_ref();
    		char *msg = pj_strerrno(*err);     
    		LOGGER_DEBUG("erreur d initialisation " << to_srs << " " << msg);
		pj_free(pj_src);
    		pthread_mutex_unlock (& mutex_proj);
    		return false;
	}

//	LOGGER_DEBUG(from_srs<<" "<<to_srs);
//	LOGGER_DEBUG("Avant "<<gridX[0]<<" "<<gridY[0]);

	if(pj_is_latlong(pj_src)) for(int i = 0; i < nbx*nby; i++) {
    		gridX[i] *= DEG_TO_RAD;
    		gridY[i] *= DEG_TO_RAD;
	}

	int code=pj_transform(pj_src, pj_dst, nbx*nby, 0, gridX, gridY, 0);

//	LOGGER_DEBUG("Apres "<<gridX[0]<<" "<<gridY[0]);

	pj_free(pj_src);
	pj_free(pj_dst);
	pthread_mutex_unlock (& mutex_proj);

	if (code!=0){
		LOGGER_DEBUG("Code erreur proj4 :"<<code);
		return false;
	}
	for (int i=0;i<nbx*nby;i++){
		if (gridX[i]==HUGE_VAL || gridY[i]==HUGE_VAL){
			LOGGER_DEBUG("Valeurs retournees par pj_transform invalides");
			
			return false;
		}
	}

	update_bbox();

	return true;
}


inline void update(BoundingBox<double> &B, double x, double y) {
  B.xmin = std::min(B.xmin, x);
  B.xmax = std::max(B.xmax, x);
  B.ymin = std::min(B.ymin, y);
  B.ymax = std::max(B.ymax, y);
}


void Grid::update_bbox() {
  
  bbox.xmin = gridX[0];
  bbox.xmax = gridX[0];
  bbox.ymin = gridY[0];
  bbox.ymax = gridY[0];

  for(int i = 0; i < nbx - 1; i++) update(bbox, gridX[i], gridY[i]);
  for(int i = 0; i < nby - 1; i++) update(bbox, gridX[i*nbx], gridY[i*nbx]);
  double wx = (step - ((width - 1)%step))/double(step);
  for(int i = 0; i < nby; i++) update(bbox, wx * gridX[i*nbx + nbx - 2] + (1.-wx) * gridX[i*nbx + nbx - 1],
                                            wx * gridY[i*nbx + nbx - 2] + (1.-wx) * gridY[i*nbx + nbx - 1]);   
  double wy = (step - ((height - 1)%step))/double(step);
  for(int i = 0; i < nbx; i++) update(bbox, wy * gridX[(nby-2)*nbx + i] + (1.-wy) * gridX[(nby-1)*nbx + i],
                                            wy * gridY[(nby-2)*nbx + i] + (1.-wy) * gridY[(nby-1)*nbx + i]);
}


int Grid::interpolate_line(int line, float* X, float* Y, int nb) {
  int ky = line / step;
  double w = (step - (line%step))/double(step);   
  double LX[nbx], LY[nbx];
  for(int i = 0; i < nbx; i++) {
    LX[i] = w*gridX[ky*nbx + i] + (1-w)*gridX[ky * nbx + nbx + i];
    LY[i] = w*gridY[ky*nbx + i] + (1-w)*gridY[ky * nbx + nbx + i];
  }

  for(int i = 0; i < nb; i++) {
    int kx = i / step;
    double w = (step - (i%step))/double(step);   
    X[i] = w*LX[kx] + (1-w)*LX[kx+1];
    Y[i] = w*LY[kx] + (1-w)*LY[kx+1];
  }
  return nb;
}


