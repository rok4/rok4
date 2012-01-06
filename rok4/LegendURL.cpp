#include "LegendURL.h"

LegendURL::LegendURL(std::string format, std::string href, int width, int height,
		     double minScaleDenominator, double maxScaleDenominator) : 
		     ResourceLocator(format, href) , width(width), height(height),
		     minScaleDenominator(minScaleDenominator), maxScaleDenominator(maxScaleDenominator)
{

}

LegendURL::LegendURL(const LegendURL& origLUrl): ResourceLocator(origLUrl)
{
    height = origLUrl.height;
    width = origLUrl.width;
    maxScaleDenominator = origLUrl.maxScaleDenominator;
    minScaleDenominator = origLUrl.minScaleDenominator;
}


LegendURL::~LegendURL()
{

}

