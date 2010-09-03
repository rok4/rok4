/*
 * CDalle.h
 *
 *  Created on: 6 juil. 2010
 *      Author: alain
 */

#ifndef CDALLE_H_
#define CDALLE_H_
#include "LibtiffImage.h"

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

	void affiche();
	int getline(uint8_t* buffer, int line) { return 0; }
	int getline(float* buffer, int line) { return 0; }

	char *getNom() const
	{
		return nom;
	}

	void setNom(char *nom)
	{
		this->nom = nom;
	}

	double getResx() const
	{
		return resx;
	}

	double getResy() const
	{
		return resy;
	}

	double getXmax() const
	{
		return xmax;
	}

	double getXmin() const
	{
		return xmin;
	}

	double getYmax() const
	{
		return ymax;
	}

	double getYmin() const
	{
		return ymin;
	}

	void setResx(const float resx)
	{
		this->resx = resx;
	}

	void setResy(const float resy)
	{
		this->resy = resy;
	}

	void setXmax(const int xmax)
	{
		this->xmax = xmax;
	}

	void setXmin(const int xmin)
	{
		this->xmin = xmin;
	}

	void setYmax(const int ymax)
	{
		this->ymax = ymax;
	}

	void setYmin(const int ymin)
	{
		this->ymin = ymin;
	}

};

#endif /* CDALLE_H_ */
