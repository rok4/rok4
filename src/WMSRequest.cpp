#include "WMSRequest.h"
#include "Logger.h"
#include <cstdio>
  
  /*
   * Cette fonction décode une url
   * remplacer les %xx par leur caractère
   */
//  void WMSRequest::url_decode(char *src) {
  //  return;
 // }



/* converts hex char (0-9, A-Z, a-z) to decimal.
 * returns 0xFF on invalid input.
 */
char hex2int(unsigned char hex) {
	hex = hex - '0';
	if (hex > 9) {
		hex = (hex + '0' - 1) | 0x20;
		hex = hex - 'a' + 11;
	}
	if (hex > 15)
		hex = 0xFF;

	return hex;
}


void WMSRequest::url_decode(char *src) {
	unsigned char high, low;
	char* dst = src;

	while ((*src) != '\0') {
		if (*src == '+') {
			*dst = ' ';
		} else if (*src == '%') {
			*dst = '%';

			high = hex2int(*(src + 1));
			if (high != 0xFF) {
				low = hex2int(*(src + 2));
				if (low != 0xFF) {
					high = (high << 4) | low;

					/* map control-characters out */
					if (high < 32 || high == 127) high = '_';

					*dst = high;
					src += 2;
				}
			}
		} else {
			*dst = *src;
		}

		dst++;
		src++;
	}

	*dst = '\0';
}


  void WMSRequest::parseparam(char* key, char* value) {
//    url_decode(key);
    uint32_t h = 0;
    for(int i = 0; key[i]; i++) h = h*13 ^ tolower(key[i]);
    LOGGER(DEBUG) << key << " ";
    std::cerr.setf(std::ios::hex);
    LOGGER(DEBUG) << h << std::endl;
    std::cerr.setf(std::ios::dec);


    switch(h) {
      case 0x00000062 : // b
      case 0x00030993 : // bbox
        double bb[4];
        for(int i = 0, pos = 0; i < 4 && value[pos]; i++) {
          bb[i] = atof(value + pos);
          for(; value[pos] && value[pos] != ','; pos++);
          if(i < 3 && value[pos] != ',') break;
          pos++;
        }
        if(bbox) delete bbox;
	LOGGER(DEBUG) << "Box = " <<bb[0] << " " <<  bb[1] << " "<< bb[2] << " "<< bb[3] << std::endl;
        bbox = new BoundingBox<double>(bb[0], bb[1], bb[2], bb[3]);
        break;
      case 0x00000077 :                                 // w
      case 0x00368fa2 : width   = atoi(value); break;   // width
      case 0x00000068 :                                 // h
      case 0x02402fa3 : height  = atoi(value); break;   // height
      case 0x00000066 :                                 // f
      case 0x0249cb0f : format  = value; break;         // format
      case 0x0000006c :                                 // l
      case 0x002b9753 :                                 // layer
      case 0x0236af44 : layers  = value; break;         // layers
      case 0x20775835 : service = value; break;         // service
      case 0x00004682 :                                 // crs
      case 0x00004912 : crs     = value; break;         // srs
      case 0x00000073 :                                 // s
      case 0x00305ba7 :                                 // style
      case 0x0274a708 : styles  = value; break;         // styles
      case 0x200db1dd : request = value; break;         // request
      case 0xcbd39bae : transparent = strncmp(value, "false", 5); break; // transparent 

      case 0x00000078 :                                 // x
      case 0x1f8271f8 : tilecol = atoi(value); break;   // tilecol
      case 0x00000079 :                                 // y
      case 0x1f8262ba : tilerow = atoi(value); break;   // tilerow
      case 0x0000007a :                                 // z
      case 0x6a672ebf : tilematrix = atoi(value); break;// tilematrix
      case 0x27843415 : tilematrixset = value; break;   // tilematrixset
      default         : break;
    }


  }

  WMSRequest::WMSRequest(int conn_fd) : query(0), width(-1), height(-1), bbox(0), service(0), request(0), crs(0), layers(0), styles(0), format(0), transparent(false), tilerow(-1), tilecol(-1), tilematrix(-1), tilematrixset(0) {

    if(load(conn_fd) < 0) {
      return; // charge les données depuis conn_fd et retourne si erreur.
    }
    url_decode(query);
    LOGGER(DEBUG) << query << " " << int(conn_fd) << std::endl;

    for(int pos = 0; query[pos];) {
      char* key = query + pos;
      for(;query[pos] && query[pos] != '=' && query[pos] != '&'; pos++); // on trouve le premier "=", "&" ou 0
      char* value = query + pos;
      for(;query[pos] && query[pos] != '&'; pos++); // on trouve le suivant "&" ou 0
      if(*value == '=') *value++ = 0;  // on met un 0 à la place du '=' entre key et value
      if(query[pos]) query[pos++] = 0; // on met un 0 à la fin du char* value
      parseparam(key, value);
    }
  }


WMSRequest::~WMSRequest() {
  delete[] buffer;
  if(bbox) delete[] bbox;
}



// spec SCGI http://python.ca/scgi/protocol.txt
int WMSRequest::load(int conn_fd) {
  int buffer_size = 655361;
  buffer = new char[buffer_size];

  int pos = 0; // index de lecture dans les données brutes recues de conn_fd;
  int data_read = 0;

  // lire un début de données, normaleemnt on ne passe qu'une seule fois ici.
  while(data_read < 6) {
    int r = read(conn_fd, buffer + data_read, buffer_size - data_read);    
    if(r < 0) {
      perror("HTTPReaquest (read)");
      return r;
    }
    data_read += r;
  }

  for(; pos < 6 && pos < data_read && buffer[pos] >= '0' && buffer[pos] <= '9'; pos++); // identifier les premiers caractère chiffre (0-9)
  if(pos >= 6) return -1;                       // erreur SCGI header >= 100000 octets
  if(buffer[pos++] != ':') return -1;           // cf spec 
  int header_size = atoi(buffer);                // le debut doit être la taille du header
  if(header_size <= 0 || header_size + pos >= buffer_size) return -1; // header trop gros ou nul

  while(data_read < header_size + pos) {         // On lit suffisament de données pour avoir tout le header scgi
    int r = read(conn_fd, buffer + data_read, buffer_size - data_read);    
    if(r < 0) {
      perror("HTTPReaquest (read)");
      return r;
    }
    data_read += r;
  }
  
  LOGGER(DEBUG) << "Reader: " << pos << " " << header_size << " " << data_read << std::endl;


  for(int header_end = header_size + pos; pos < header_end; pos++) { // initialise les variables d'environement CGI.
    //      LOGGER(DEBUG) << buffer + pos << std::endl;
    uint32_t h; // calculer un hash des la variable
    for(h = 0; pos < header_end && buffer[pos]; pos++) h = h*13 ^ buffer[pos];
    pos++;

    switch(h) {
      //      case 0xd1bef2e0: break; // hash("REQUEST_URI") 
      case 0xa56ea420: query = buffer + pos; break; // hash("QUERY_STRING")
      default: break;
    }
    for(;pos < header_end && buffer[pos]; pos++); // on lit la valeur du champs
  }

  if(!query) return -1;  
  return 1;
}

