#ifndef _FILEMANAGER_
#define _FILEMANAGER_


#include <fcntl.h>
#include "Logger.h"
#include <cstdio>

//#include "TiledFile.h"
//#include <map>

// Classe qui lit les tuiles d'un fichier tuilé.
// A coder plus efficacement.


class FileManager {
  public:

    static const uint8_t* gettile(std::string filename, uint32_t &tile_size, uint32_t posoff, uint32_t posindex) {
      LOGGER(DEBUG) << "gettile: " << filename << " " << posoff << " " << posindex << " " << std::endl;
      uint8_t *data = 0;
      int fildes = open(filename.c_str(), O_RDONLY);
      if(fildes < 0) {
        //perror("open");
        return 0;
      }
      uint32_t pos;
      if(pread(fildes, &pos, sizeof(pos), posoff) < 0) {
        perror("read");
        LOGGER(DEBUG) << " Erreur pread " <<std::endl;
        close(fildes);
        return 0;
      }
      LOGGER(DEBUG) << "fildes=" << fildes << " tile_size= "<<tile_size<< " size="<< sizeof(tile_size) << "offset="<<posoff    <<" pos=" << pos <<std::endl;
      if(pread(fildes, &tile_size, sizeof(tile_size), posindex) < 0){
        perror("read");
        close(fildes);
        return 0;
      }
      if(tile_size > 1048576)
	{
	LOGGER(DEBUG) << " erreur : tile_size trop grand " << std::endl ;
	close(fildes);
	return 0;  // max 1Mo / tuile
	}
      data = new uint8_t[tile_size];
      if(pread(fildes, data, tile_size, pos) < 0) {
      LOGGER(DEBUG) << " erreur pread " << std::endl;
        delete[] data;
        close(fildes);
        perror("read");
        return 0;
      }

      close(fildes);
      return data;
    }

    static int releasetile(const uint8_t* data) {
	
      delete[] data;
      return 1;
    }
};

/*

   class FileManager {
   private:
// const int max_open_file;
std::map<std::string, TiledFile*> F;
std::map<const uint8_t*, TiledFile*> P;
pthread_mutex_t mutex;


public:

const uint8_t* gettile(std::string filename, int n, uint32_t &tile_size) {      
pthread_mutex_lock(&mutex);

LOGGER(DEBUG) << "gettile" << filename << " " << n << " " << tile_size;

        TiledFile *PF = F[filename];
        if(!PF) {
          PF = new TiledFile(filename);     
          F[filename] = PF;
          P[PF->filemmap] = PF;
        }
        PF->usagecount++;
       const uint8_t *ret = PF->gettile(n, tile_size);

      LOGGER(DEBUG) << " " << (unsigned int)ret << " " << tile_size << " " << PF->usagecount << std::endl;

      pthread_mutex_unlock(&mutex);
      return ret;
    }



    int releasetile(const uint8_t* data) {
      pthread_mutex_lock(&mutex);
      assert(P.size() > 0);
      std::map<const uint8_t*, TiledFile*>::iterator it = P.upper_bound(data);
      assert(it != P.begin());
      it--;
      TiledFile* tf = it->second;
      assert(data < tf->filemmap + tf->filesize);
      
      LOGGER(DEBUG) << "releasetile" << tf->filename << " " << (unsigned int)data << " " << tf->usagecount << std::endl;

      if(!--(tf->usagecount)) {
        P.erase(it);
        F.erase(tf->filename);
        delete tf;
      }
      pthread_mutex_unlock(&mutex);
      return 1;
    }

    FileManager() {
      pthread_mutex_init(&mutex, 0);      
    }

};
*/

#endif
