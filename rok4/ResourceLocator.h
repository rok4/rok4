#ifndef RESOURCELOCATOR_H
#define RESOURCELOCATOR_H
#include <string>
class ResourceLocator
{
private:
        std::string format;
        std::string href;
public:
    ResourceLocator(std::string format, std::string href);
    ResourceLocator(const ResourceLocator &origRL);
    inline const std::string getFormat() const {return format;}
    inline const std::string getHRef() const {return href;}
    bool operator==(const ResourceLocator& other) const;
    bool operator!=(const ResourceLocator& other) const;

    virtual ~ResourceLocator();
    
};

#endif // RESOURCELOCATOR_H
