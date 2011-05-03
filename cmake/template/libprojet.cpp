#include "libprojet.h"
#include <string>
#include <iostream>

LibProjet::LibProjet()
{
	message= std::string("Hello World!");
}

LibProjet::~LibProjet()
{
}

void LibProjet::sayHello()
{
	std::cout << message << std::endl;
}

