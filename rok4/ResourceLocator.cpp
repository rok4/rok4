#include "ResourceLocator.h"

ResourceLocator::ResourceLocator(std::string format, std::string href) : format(format), href(href)
{

}

ResourceLocator::ResourceLocator(const ResourceLocator& origRL)
{
    format = origRL.format;
    href = origRL.href;
}

bool ResourceLocator::operator==(const ResourceLocator& other) const
{
    return ( ( this->format.compare( other.format ) == 0 ) 
        && ( this->href.compare( other.href ) == 0 ) );
}

bool ResourceLocator::operator!=(const ResourceLocator& other) const
{
    return !(*this == other);
}


ResourceLocator::~ResourceLocator()
{

}

