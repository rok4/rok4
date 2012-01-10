#ifndef LEGENDURL_H
#define LEGENDURL_H
#include "ResourceLocator.h"



class LegendURL : public ResourceLocator
{
private:
	int width;
	int height;
	double minScaleDenominator;
	double maxScaleDenominator;
public:
    LegendURL(std::string format, std::string href,int width, int height, double minScaleDenominator, double maxScaleDenominator);
    LegendURL(const LegendURL &origLUrl);
    inline int getWidth(){return width;}
    inline int getHeight(){return height;}
    inline double getMinScaleDenominator(){return minScaleDenominator;}
    inline double getMaxScaleDenominator(){return maxScaleDenominator;}
    
    virtual ~LegendURL();
};

#endif // LEGENDURL_H
