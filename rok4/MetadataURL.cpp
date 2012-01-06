#include "MetadataURL.h"

MetadataURL::MetadataURL(std::string format, std::string href,
                         std::string type) : ResourceLocator(format,href), type(type)
{

}

MetadataURL::MetadataURL(const MetadataURL& origMtdUrl): ResourceLocator(origMtdUrl)
{
    type = origMtdUrl.type;
}


MetadataURL::~MetadataURL()
{

}

