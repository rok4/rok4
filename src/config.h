#ifndef _CONFIG_
#define _CONFIG_

#include <unistd.h>
#include <stdint.h>
#include <cstring>
#include <cstdlib>
#include <algorithm>

#include <cassert>
// Pour déactiver tous les assert, décommenter la ligne suivante
// #define NDEBUG

#include <iostream>
#include "Logger.h"

#define MAX_IMAGE_WIDTH  65536
#define MAX_IMAGE_HEIGHT 65536

#define LOG_CONF_PATH      "../config/logConfig.xml"
#define SERVER_CONF_PATH   "../config/server.conf"
#define SERVICES_CONF_PATH "../config/services.conf"

#define DEFAULT_NB_THREAD  1
#define DEFAULT_LAYER_DIR  "../config/layers/"
#define DEFAULT_TMS_DIR    "../config/tileMatrixSet"
#define DEFAULT_STYLE      "normal"   //FIXME: c'est une valeur bidon en attedant d'avoir de vrai style
#define DEFAULT_OPAQUE     true
#define DEFAULT_RESAMPLING "moyenne"  //FIXME: c'est une valeur bidon en attedant d'avoir de vrai algo d'interpolation
#define DEFAULT_CHANNELS   3


#endif
