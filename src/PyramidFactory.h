#ifndef PYRAMIDFACTORY_H_
#define PYRAMIDFACTORY_H_

#include <cstring>
#include <libxml2/libxml/tree.h>
#include "Pyramid.h"

class PyramidFactory {
private:
  static int validation_schema(xmlDocPtr doc, const char *xml_schema, bool afficher_erreurs);
public:
  static Pyramid * make(std::string configFileName);
};

#endif /* PYRAMIDFACTORY_H_ */
