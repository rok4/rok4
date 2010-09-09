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

class CDalle /* : public LibtiffImage */ {

	char * nom ;
	double xmin ;
	double ymax ;
	double xmax ;
	double ymin ;
	double resx ;
	double resy ;

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
	double inline getResx() const{return resx;}
	double inline getResy() const{return resy;}
	double inline getXmax() const{return xmax;}
	double inline getXmin() const{return xmin;}
	double inline getYmax() const{return ymax;}
	double inline getYmin() const{return ymin;}
	void inline setResx(const float resx){this->resx = resx;}
	void inline setResy(const float resy){this->resy = resy;}
	void inline setXmax(const int xmax){this->xmax = xmax;}
	void inline setXmin(const int xmin){this->xmin = xmin;}
	void inline setYmax(const int ymax){this->ymax = ymax;}
	void inline setYmin(const int ymin){this->ymin = ymin;}

	int readFromFile(ifstream& file);
};

#endif /* CDALLE_H_ */
