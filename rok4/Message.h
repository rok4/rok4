#ifndef _MESSAGE_
#define _MESSAGE_

#include "Data.h"
#include <string.h> // Pour memcpy
#include "ServiceException.h"
#include <vector>


class MessageDataSource : public DataSource {
private:
	std::string type;
protected:
	std::string message;
public:
	MessageDataSource(std::string message, std::string type) : message(message), type(type) {}
	const uint8_t* getData(size_t& size) {size=message.length();return (const uint8_t*)message.data();}
        int getHttpStatus() {return 200;}
	std::string getType() {return type.c_str();}
	bool releaseData() {}
};

/**
 * @class SERDataSource
 * genere un MessageDataSource a partir de ServiceExceptions
 */
class SERDataSource : public MessageDataSource {
private:
	int httpStatus ;
public:
/**
	* Constructeur à partir d'un ServiceException
	* @param sex le ServiceException
	*/
	SERDataSource(ServiceException *sex) ;
/**
	* Constructeur à partir d'un tableau de ServiceException
	* @param sexcp le tableau des ServiceExceptions
	*/
	SERDataSource(std::vector<ServiceException*> *sexcp) ;
/**
 * getter pour la propriete Message
 */
	std::string getMessage() {return this->message;} ;
/**
 * getter pour la propriete httpStatus
 */
	int getHttpStatus() {return this->httpStatus;}
};

class MessageDataStream : public DataStream {
private:
	std::string type;
	uint32_t pos;
protected:
	std::string message;

public:
	MessageDataStream(std::string message, std::string type) : message(message), type(type), pos(0) {}

	size_t read(uint8_t *buffer, size_t size){
		if(size > message.length() - pos) size = message.length() - pos;
		memcpy(buffer,(uint8_t*)(message.data()+pos),size);
		pos+=size;
		return size;
	}
	bool eof() {return (pos==message.length());}
	std::string getType() {return type.c_str();}
	int getHttpStatus() {return 200;}
};

/**
 * @class SERDataStream
 * genere un MessageDataStream a partir de ServiceExceptions
 */
class SERDataStream : public MessageDataStream {
private:
	int httpStatus ;
public:
/**
	* Constructeur à partir d'un ServiceException
	* @param sex le ServiceException
	*/
	SERDataStream(ServiceException *sex) ;
/**
	* Constructeur à partir d'un tableau de ServiceException
	* @param sexcp le tableau des ServiceExceptions
	*/
	SERDataStream(std::vector<ServiceException*> *sexcp) ;
/**
 * getter pour la propriete Message
 */
	std::string getMessage() {return this->message;} ;
/**
 * getter pour la propriete httpStatus
 */
	int getHttpStatus() {return this->httpStatus;}
};

#endif
