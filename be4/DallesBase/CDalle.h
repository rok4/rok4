/*
 * CDalle.h
 *
 *  Created on: 6 juil. 2010
 *      Author: alain
 */

#ifndef CDALLE_H_
#define CDALLE_H_
#include <string.h>
#include <iostream>
#include <fstream>
#include "Image.h"

class dalleImage  : public Image  {

	friend 

	char * nom ;
public:
	CDalle();
	CDalle(const char * c, const double i1, const double i2, const double i3,
			const double i4, const double f1, const double f2);
	virtual ~CDalle();
	CDalle(const CDalle& );

	int getline(uint8_t* buffer, int line) { return 0; }
	int getline(float* buffer, int line) { return 0; }

	char inline *getNom() const{return nom;}
	void inline setNom(char *nom){strcpy(this->nom,nom);}

	int readFromFile(ifstream& file);
};

#endif /* CDALLE_H_ */
