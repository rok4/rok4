CXX = g++
CC  = g++
#CXXFLAGS = -O3  -mfpmath=sse -msse -msse2 -I"."
#conf de debug:
CXXFLAGS = -g -O0 -msse2
LDFLAGS =
LDLIBS = -lpthread -lz -lproj -ljpeg
LDCPPUNITLIBS= -lcppunit

# CCFLAGS = -g -ansi

BINDIR=../bin
DOCSDIR=../docs
#
#
# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTests
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

WMSSOBJ= WMSServer.o Logger.o Listener.o ResponseSender.o Layer.o Pyramid.o PNGEncoder.o Image.o WMSRequest.o CompoundImage.o ResampledImage.o


all: clean WMSServer
#
# WMSServer
# compile et genere le serveur WMS
WMSServer: bindir $(WMSSOBJ)
	$(CXX) $(WMSSOBJ) -o WMSServer $(LDLIBS)
	rm -f WMSServer.o
	mv WMSServer *.o $(BINDIR)

Test : Test.o Image.o Tile.o pixel_type.o Logger.o PNGEncoder.o 


#
# cpputest
# compile et execute les tests unitaires cppunit
cpputest: clean WMSServer cppubin
	$(CPPUBIN)

cppubin: $(CPPUNITOBJ)
	$(CXX) $(CPPUNITOBJ) $(BINDIR)/*.o -o $(CPPUBIN) $(LDCPPUNITLIBS) $(LDLIBS)
	mv $(CPPUNITOBJ) $(BINDIR)

bindir:
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi

doc : 
	doxygen $(DOCSDIR)/doxygen.cfg

clean:
	rm -f *.o $(CPPUNITDIR)/*.o $(BINDIR)/* WMSServer
