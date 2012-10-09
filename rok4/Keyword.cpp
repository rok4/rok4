#include "Keyword.h"
Keyword::Keyword ( std::string content, std::map< std::string, std::string > attributes ) : content ( content ), attributes ( attributes ) {

}


Keyword::Keyword ( const Keyword& origKW ) {
    content = origKW.content;
    attributes = origKW.attributes;
}

bool Keyword::operator!= ( const Keyword& other ) const {
    return ! ( *this == other );
}

Keyword& Keyword::operator= ( const Keyword& other ) {
    if ( this != &other ) {
        this->content = other.content;
        this->attributes = other.attributes;
    }
    return *this;

}

bool Keyword::operator== ( const Keyword& other ) const {
    return ( this->content.compare ( other.content ) == 0 );

}

Keyword::~Keyword() {

}
