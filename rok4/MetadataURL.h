#ifndef METADATAURL_H
#define METADATAURL_H
#include "ResourceLocator.h"

class MetadataURL : public ResourceLocator
{
private:
    std::string type;
    
public:
    MetadataURL(std::string format, std::string href, std::string type);
    MetadataURL(const MetadataURL & origMtdUrl);
    inline std::string getType(){return type;}
    virtual ~MetadataURL();
};

#endif // METADATAURL_H
