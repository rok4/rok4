#ifndef LEGENDURL_H
#define LEGENDURL_H
#include <string>

class LegendURL
{
private:
	std::string format;
	std::string href;
	int width;
	int height;
	double minScaleDenominator;
	double maxScaleDenominator;
public:
    LegendURL(std::string format, std::string href,int width, int height, double minScaleDenominator, double maxScaleDenominator);
    inline std::string getFormat(){return format;}
    inline std::string getHRef(){return href;}
    
    inline int getWidth(){return width;}
    inline int getHeight(){return height;}
    inline double getMinScaleDenominator(){return minScaleDenominator;}
    inline double getMaxScaleDenominator(){return maxScaleDenominator;}
    
    virtual ~LegendURL();
};

#endif // LEGENDURL_H
