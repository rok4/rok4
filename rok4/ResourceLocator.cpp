#include "ResourceLocator.h"

ResourceLocator::ResourceLocator(std::string format, std::string href) : format(format), href(href)
{

}

ResourceLocator::ResourceLocator(const ResourceLocator& origRL)
{
    format = origRL.format;
    href = origRL.href;
}


ResourceLocator::~ResourceLocator()
{

}

