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
	unsigned int maxWidth;
	unsigned int maxHeight;
	std::vector<std::string> formatList;
public:

	ServicesConf(std::string name, std::string title, std::string abstract, std::vector<std::string> keyWords,
			std::string serviceProvider, std::string fee, std::string accessConstraint,
			unsigned int maxWidth, unsigned int maxHeight, std::vector<std::string> formatList) :
				name(name), title(title), abstract(abstract), keyWords(keyWords),
				serviceProvider(serviceProvider), fee(fee), accessConstraint(accessConstraint),
				maxWidth(maxWidth), maxHeight(maxHeight), formatList(formatList) {};
	std::string getAbstract() const
	{
		return abstract;
	}

	std::string getAccessConstraint() const
	{
		return accessConstraint;
	}

	std::string getFee() const
	{
		return fee;
	}

	std::vector<std::string> getFormatList() const
    		{
		return formatList;
    		}

	std::vector<std::string> getKeyWords() const
    		{
		return keyWords;
    		}

	unsigned int getMaxHeight() const
	{
		return maxHeight;
	}

	unsigned int getMaxWidth() const
	{
		return maxWidth;
	}

	std::string getName() const
	{
		return name;
	}

	std::string getServiceProvider() const
	{
		return serviceProvider;
	}

	std::string getTitle() const
	{
		return title;
	}
};

#endif /* SERVICESCONF_H_ */
