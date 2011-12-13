#ifndef SERVICESCONF_H_
#define SERVICESCONF_H_

#include <string>
#include <vector>

class ServicesConf {
private:
	// WMS & WMTS
	std::string title;
	std::string abstract;
	std::vector<std::string> keyWords;
	std::string serviceProvider;
	std::string fee;
	std::string accessConstraint;
	// WMS
	std::string name;
	unsigned int maxWidth;
	unsigned int maxHeight;
	std::vector<std::string> formatList;
	// WMTS
	std::string serviceType;
	std::string serviceTypeVersion;
	//Inspire
	bool inspire;
public:

	ServicesConf(std::string name, std::string title, std::string abstract, std::vector<std::string> keyWords,
			std::string serviceProvider, std::string fee, std::string accessConstraint,
			unsigned int maxWidth, unsigned int maxHeight, std::vector<std::string> formatList, std::string serviceType,
			std::string serviceTypeVersion, bool inspire=0) :
				name(name), title(title), abstract(abstract), keyWords(keyWords),
				serviceProvider(serviceProvider), fee(fee), accessConstraint(accessConstraint),
				maxWidth(maxWidth), maxHeight(maxHeight), formatList(formatList), serviceType(serviceType),
				serviceTypeVersion(serviceTypeVersion), inspire(inspire) {};
	//  WMS & WMTS
	std::string inline getAbstract() const	{return abstract;}
	std::string inline getAccessConstraint() const{return accessConstraint;}
	std::string inline getFee() const{return fee;}
	std::vector<std::string> inline getKeyWords() const{return keyWords;}
	std::string inline getServiceProvider() const {return serviceProvider;}
	std::string inline getTitle() const {return title;}
	// WMS
	unsigned int inline getMaxHeight() const{return maxHeight;}
	unsigned int inline getMaxWidth() const{return maxWidth;}
	std::string inline getName() const{return name;}
	std::vector<std::string>* getFormatList() {return &formatList;}
	// WMTS
	std::string inline getServiceType() {return serviceType;}
	std::string inline getServiceTypeVersion() {return serviceTypeVersion;}
	bool inline isInspire(){return inspire;}
};

#endif /* SERVICESCONF_H_ */
