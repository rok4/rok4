/*
 * CDalle.cpp
 *
 *  Created on: 6 juil. 2010
 *      Author: alain
 */

using namespace std;
#include "CDalle.h"

CDalle::CDalle(const CDalle& cd) {
	// Ici on passe le parametre par REFERENCE et non par copie !!
	nom = new char [256];
	strcpy (nom, cd.nom );
}


CDalle::CDalle() {
	nom = new char [256];
}

CDalle::CDalle(const char * c, const double i1, const double i2, const double i3,
		const double i4, const double f1, const double f2) {
	nom = new char [256];
	strcpy (nom, c );
}

CDalle::~CDalle() {
	delete [] nom;
}

int CDalle::readFromFile(ifstream& file)
{
	string str;
	std::getline(file,str);
	int n;
	if ((n=sscanf (str.c_str(), "%s %lf %lf %lf %lf %lf %lf", nom, &xmin, &xmax, &ymin, &ymax, &resx, &resy))!=7)
		return -1;
	return n;
}
