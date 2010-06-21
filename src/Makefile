CXX = g++
CC  = gcc
#CXXFLAGS = -O3  -mfpmath=sse -msse -msse2 -I"."
#conf de debug:
CXXFLAGS = -g -O0 -msse2 -I"."
LDFLAGS =
LDLIBS = -lpthread -lz -lproj -ljpeg -llog4cxx
LDCPPUNITLIBS= -lcppunit

# CCFLAGS = -g -ansi

TGTDIR=../target
BINDIR=$(TGTDIR)/bin
SRCCONFDIR=../config
TGTCONFDIR=$(TGTDIR)/config
DOCSDIR=../docs
#
#
# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTests
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

FCGIOBJ= libfcgi/fcgiapp.o libfcgi/os_unix.o
WMSSOBJ= WMSServer.o Logger.o ResponseSender.o Layer.o Pyramid.o PNGEncoder.o Image.o WMSRequest.o CompoundImage.o ResampledImage.o

all: clean WMSServer
#
# WMSServer
# compile et genere le serveur WMS
WMSServer: bindir $(WMSSOBJ)
	make -C libfcgi
	$(CXX) $(WMSSOBJ) $(FCGIOBJ) -o WMSServer $(LDLIBS)
	rm -f WMSServer.o
	mv WMSServer *.o $(FCGIOBJ) $(BINDIR)
	cp $(SRCCONFDIR)/* $(TGTCONFDIR)

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
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(TGTCONFDIR) ] ; then mkdir $(TGTCONFDIR) ; fi
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi

doc : 
	doxygen $(DOCSDIR)/doxygen.cfg

clean:
	rm -f *.o $(CPPUNITDIR)/*.o libfcgi/*.o WMSServer	
	rm -fr $(TGTDIR)
	make -C libfcgi clean
