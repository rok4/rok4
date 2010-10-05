#ifndef _MESSAGE_
#define _MESSAGE_

#include "Data.h"
#include <string.h> // Pour memcpy

class MessageDataSource : public DataSource {
private:
	std::string message;
	std::string type;
public:
	MessageDataSource(std::string message, std::string type) : message(message), type(type) {}
	const uint8_t* get_data(size_t& size) {size=message.length();return (const uint8_t*)message.data();}
	std::string gettype() {return type.c_str();}
	bool release_data() {}
};


class MessageDataStream : public DataStream {
private:
	std::string message;
	std::string type;
	uint32_t pos;
public:
	MessageDataStream(std::string message, std::string type) : message(message), type(type), pos(0) {}

	size_t read(uint8_t *buffer, size_t size){
		if(size > message.length() - pos) size = message.length() - pos;
		memcpy(buffer,(uint8_t*)message.data(),size);
		pos+=size;
		return size;
	}
	bool eof() {return (pos==message.length());}
	std::string gettype() {return type.c_str();}
};

#endif
