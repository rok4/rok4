#ifndef SERVICESCONF_H_
#define SERVICESCONF_H_

#include <string>
#include <vector>

class ServicesConf {
private:
	std::string name;
	std::string title;
	std::string abstract;
	std::vector<std::string> keyWords;
	std::string serviceProvider;
	std::string fee;
	std::string accessConstraint;
	unsigned int layerLimit;
	unsigned int maxWidth;
	unsigned int maxHeight;
	std::vector<std::string> formatList;
public:

    ServicesConf(std::string name, std::string title, std::string abstract, std::vector<std::string> keyWords,
                std::string serviceProvider, std::string fee, std::string accessConstraint, unsigned int layerLimit,
                unsigned int maxWidth, unsigned int maxHeight, std::vector<std::string> formatList) :
                name(name), title(title), abstract(abstract), keyWords(keyWords),
                serviceProvider(serviceProvider), fee(fee), accessConstraint(accessConstraint), layerLimit(layerLimit),
                maxWidth(maxWidth), maxHeight(maxHeight), formatList(formatList) {};
//    virtual ~ServicesConf();
};

#endif /* SERVICESCONF_H_ */
