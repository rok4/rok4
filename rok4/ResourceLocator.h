#ifndef RESOURCELOCATOR_H
#define RESOURCELOCATOR_H
#include <string>
class ResourceLocator
{
private:
        std::string format;
        std::string href;
        int width;
        int height;
        double minScaleDenominator;
        double maxScaleDenominator;
public:
    ResourceLocator(std::string format, std::string href);
    ResourceLocator(const ResourceLocator &origRL);
    inline std::string getFormat(){return format;}
    inline std::string getHRef(){return href;}
    
    virtual ~ResourceLocator();
    
};

#endif // RESOURCELOCATOR_H
