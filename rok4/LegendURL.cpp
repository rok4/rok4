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

bool LegendURL::operator==(const LegendURL& other) const
{
    return (this->width == other.width && this->height == other.height 
        && this->minScaleDenominator == other.minScaleDenominator 
        && this->maxScaleDenominator == other.maxScaleDenominator 
        && this->getFormat().compare( other.getFormat() ) == 0
        && this->getHRef().compare( other.getHRef() ) == 0 );
}

bool LegendURL::operator!=(const LegendURL& other) const
{
    return !(*this == other);
}


LegendURL::~LegendURL()
{

}

