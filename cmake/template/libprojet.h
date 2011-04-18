#include <string>

/**
\class LibProjet
\brief Template Hello World
Un projet template pour s'approprier CMake
@author IGN
*/
class LibProjet {
public:
	
	LibProjet();
	virtual ~LibProjet();

	/** Le message */
	std::string message;
	
	/** affiche le message sur la sortie standard*/
	virtual void sayHello();

};
