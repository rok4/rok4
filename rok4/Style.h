#ifndef STYLE_H
#define STYLE_H
#include <string>
#include <vector>
#include "LegendURL.h"
#include "Palette.h"


class Style
{
private :
	std::string id;
	std::vector<std::string> titles;
	std::vector<std::string> abstracts;
	std::vector<std::string> keywords;
	std::vector<LegendURL> legendURLs;
	Palette palette;
public:
	Style(const std::string& id,const std::vector<std::string>& titles,
	      const std::vector<std::string>& abstracts,const  std::vector<std::string>& keywords,
	      const std::vector<LegendURL>& legendURLs, Palette& palette); 
	inline std::string getId(){return id;}
	inline std::vector<std::string> getTitles() { return titles;}
	inline std::vector<std::string> getAbstracts() { return abstracts;}
	inline std::vector<std::string> getKeywords() { return keywords;}
	inline std::vector<LegendURL> getLegendURLs(){return legendURLs;}
	inline Palette* getPalette(){return &palette;}
};

#endif // STYLE_H
