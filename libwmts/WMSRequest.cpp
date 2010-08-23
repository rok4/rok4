#include "WMSRequest.h"
#include "Logger.h"
  
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
    LOGGER_DEBUG(key << " " << h);

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
	LOGGER_DEBUG( "Box = " <<bb[0] << " " <<  bb[1] << " "<< bb[2] << " "<< bb[3] );
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
      case 0x6a672ebf : tilematrix = value; break;      // tilematrix
      case 0x27843415 : tilematrixset = value; break;   // tilematrixset
      default         : break;
    }


  }

  WMSRequest::WMSRequest(char* strquery) :  /*query(0),*/ width(-1), height(-1), bbox(0), service(0), request(0), crs(0), layers(0), styles(0), format(0), transparent(false), tilerow(-1), tilecol(-1), tilematrix(""), tilematrixset(0) {

    url_decode(strquery);
  //  LOGGER_DEBUG( query << " " << strquery );

    for(int pos = 0; strquery[pos];) {
      char* key = strquery + pos;
      for(;strquery[pos] && strquery[pos] != '=' && strquery[pos] != '&'; pos++); // on trouve le premier "=", "&" ou 0
      char* value = strquery + pos;
      for(;strquery[pos] && strquery[pos] != '&'; pos++); // on trouve le suivant "&" ou 0
      if(*value == '=') *value++ = 0;  // on met un 0 à la place du '=' entre key et value
      if(strquery[pos]) strquery[pos++] = 0; // on met un 0 à la fin du char* value
      parseparam(key, value);
    }

    if(crs) for(int i = 0; crs[i]; i++) crs[i] = tolower(crs[i]);
  }

WMSRequest::~WMSRequest() {
  if(bbox) delete bbox;
}

