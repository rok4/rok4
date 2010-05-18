CXX = g++
CC  = g++
CXXFLAGS = -O3  -mfpmath=sse -msse -msse2
#conf de debug:
#CXXFLAGS = -g -O0 -msse2
LDFLAGS =
LDLIBS = -lpthread -lz -lproj -ljpeg

CCFLAGS = -g -ansi


all: clean WMSServer

WMSServer: Logger.o Listener.o ResponseSender.o Layer.o Pyramid.o PNGEncoder.o Image.o WMSRequest.o CompoundImage.o ResampledImage.o

Test : Test.o Image.o Tile.o pixel_type.o Logger.o PNGEncoder.o 

# CppUnit Stuff
LDCPPUNITLIBS= -lcppunit
CPPUNIT = $(wildcard CppUnit*.cpp)
CPPUNITO  = $(patsubst %.cpp,%.o,  $(CPPUNIT))

uobjects= cppunit.o ${CPPUNITO}

cppunit : $(uobjects)
	${CXX} -o CppUnitTests $(uobjects) ${LDCPPUNITLIBS} ${LDLIBS}
	./CppUnitTests

doc : 
	doxygen ../docs/doxygen.cfg

clean:
	rm -f *.o CppUnitTests
