/*
 * CDalle.cpp
 *
 *  Created on: 6 juil. 2010
 *      Author: alain
 */

using namespace std;
#include <iostream> //pour cout, endl
#include <string.h>
#include "CDalle.h"

void CDalle::affiche() {
	cout << " -> "  ;
	cout << getNom() << " " ;
	cout << getXmin() << " " << getYmax() << " " ;
	cout << getXmax() << " " << getYmin() << " " ;
	cout << getResx() << " " << getResy() << endl ;
}

CDalle::CDalle(const CDalle& cd) {
	// Ici on passe le parametre par REFERENCE et non par copie !!
	nom = new char [256];
	strcpy (nom, cd.nom );
	xmin=cd.xmin;
	ymax=cd.ymax;
	xmax=cd.xmax;
	ymin=cd.ymin;
	resx=cd.resx;
	resy=cd.resy;
}


CDalle::CDalle() {
	nom = new char [256];
}

CDalle::CDalle(const char * c, const double i1, const double i2, const double i3,
		const double i4, const double f1, const double f2) {

	nom = new char [256];
	strcpy (nom, c );
	xmin=i1;
	ymax=i2;
	xmax=i3;
	ymin=i4;
	resx=f1;
	resy=f2;
}

CDalle::~CDalle() {
	// TODO Auto-generated destructor stub
}

