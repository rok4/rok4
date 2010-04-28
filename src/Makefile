CXX = g++
CC  = g++
CXXFLAGS = -O3  -mfpmath=sse -msse -msse2
#conf de debug:
#CXXFLAGS = -g -O0 -msse2
LDFLAGS =
LDLIBS = -lpthread -lz -lproj -ljpeg


all: clean WMSServer

WMSServer: Logger.o Listener.o ResponseSender.o Layer.o Pyramid.o PNGEncoder.o Image.o WMSRequest.o CompoundImage.o ResampledImage.o

Test : Test.o Image.o Tile.o pixel_type.o Logger.o PNGEncoder.o 

doc : 
	doxygen ../docs/doxygen.cfg

clean:
	rm -f *.o
