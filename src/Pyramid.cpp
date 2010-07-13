#include <cmath>
#include "Pyramid.h"
#include "Logger.h"

HttpResponse* Pyramid::gettile(int x, int y, std::string tmId) {
/*  assert(x >= 0);
  assert(y >= 0);
  assert(z >= 0);
  assert(z < (int) levels.size());
*/
  return levels[tmId]->gettile(x, y);
}

std::string Pyramid::best_level(double resolution_x, double resolution_y) {

  // TODO: A REFAIRE !!!!
  double resolution = sqrt(resolution_x * resolution_y);

  std::map<std::string, Level*>::iterator it(levels.begin()), itend(levels.end());
  std::string best_h = it->first;
  double best = resolution_x / it->second->getRes();
  ++it;
  for (;it!=itend;++it){
	  LOGGER_DEBUG("level teste:" << it->first << "\tmeilleur level:" << best_h << "\tnbre de level:" << levels.size() << "\tlevel.res:" << it->second->getRes());
	  double d = resolution / it->second->getRes();
      if((best < 0.8 && d > best) ||
        (best >= 0.8 && d >= 0.8 && d < best)) {
        best = d;
        best_h = it->first;
      }
  }
  return best_h;
}


Image* Pyramid::getbbox(BoundingBox<double> bbox, int width, int height, const char *dst_crs) {
  // on calcule la résolution de la requete dans le crs source selon une diagonale de l'image.
  double resolution_x = (bbox.xmax - bbox.xmin) / width;
  double resolution_y = (bbox.ymax - bbox.ymin) / height;
  std::string l = best_level(resolution_x, resolution_y);
  return levels[l]->getbbox(bbox, width, height);


  LOGGER_DEBUG( "best_level=" << l << " resolution requete=" << resolution_x << " " << resolution_y);
/*  
  if(!dst_crs) return levels[h]->getbbox(bbox, width, height);
  else return levels[h]->getbbox(bbox, width, height, dst_crs);
  */
}

//TODO: destructeur


