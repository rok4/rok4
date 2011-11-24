#include "Style.h"
#include "Logger.h"
Style::Style(const std::string& id,const std::vector<std::string>& titles,
	      const std::vector<std::string>& abstracts,const std::vector<std::string>& keywords,
	      const std::vector<LegendURL>& legendURLs, Palette& palette) //: id(id), titles(titles),	      abstracts(abstracts), keywords(keywords), legendURLs(legendURLs), palette(palette)
{
		LOGGER_DEBUG("Nouveau Style : " << id);
		this->id = id.c_str();
		this->titles= titles;
		this->abstracts = abstracts;
		this->keywords = keywords;
		this->legendURLs = legendURLs;
		this->palette = palette;
		
}
