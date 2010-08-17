INCDIR=../include
LIBDIR=../lib

CXX = g++
CC  = gcc
CXXFLAGS = -O3  -msse2 -I"." -I$(INCDIR) 
#conf de debug:
#CXXFLAGS = -g -O0 -msse2 -I"." 
LDFLAGS = -L$(LIBDIR)
LDLIBS = -limage -ltinyxml -lfcgi -lz -llogger -llog4cxx -lpthread -ljpeg 
LDCPPUNITLIBS= -lcppunit

# CCFLAGS = -g -ansi

TGTDIR=../target
BINDIR=$(TGTDIR)/bin
SRCCONFDIR=../config
TGTCONFDIR=$(TGTDIR)/config
DOCSDIR=../docs

# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTests
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

WMSSOBJ= WMSServer.o ConfLoader.o TileMatrix.o TileMatrixSet.o Layer.o ResponseSender.o Level.o Pyramid.o TiffEncoder.o PNGEncoder.o WMSRequest.o Tile.o

all: clean WMSServer
#
# WMSServer
# compile et genere le serveur WMS
WMSServer: dependencies bindir $(WMSSOBJ)
	$(CXX) $(LDFLAGS) ../lib/libz.a $(WMSSOBJ) -o WMSServer $(LDLIBS)
	cp WMSServer *.o $(FCGIOBJ) $(TIXMLOBJ) $(BINDIR)
	rm $(BINDIR)/WMSServer.o
	cp -r $(SRCCONFDIR)/* $(TGTCONFDIR)

dependencies:
	make -C ../libfcgi	
	make -C ../libtinyxml
	make -C ../libimage
	make -C ../liblogger
	make -C ../libz




#Test : Test.o Image.o Tile.o Logger.o TiffEncoder.o PNGEncoder.o 


#
# cpputest
# compile et execute les tests unitaires cppunit
cpputest: WMSServer cppubin
	$(CPPUBIN)

cppubin: $(CPPUNITOBJ)
	$(CXX) $(LDFLAGS) $(CPPUNITOBJ) $(BINDIR)/*.o -o $(CPPUBIN) $(LDCPPUNITLIBS) $(LDLIBS)
	cp $(CPPUNITOBJ) $(BINDIR)
	make -C ../libimage cpputest


bindir: 
	rm -fr $(TGTDIR)
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(TGTCONFDIR) ] ; then mkdir $(TGTCONFDIR) ; fi
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi

doc : 
	doxygen $(DOCSDIR)/doxygen.cfg

clean:
	rm -f *.o $(CPPUNITDIR)/*.o WMSServer	
	rm -fr $(TGTDIR)

veryclean: clean
	make -C ../libfcgi clean
	make -C ../libtinyxml clean
	make -C ../libimage clean
	make -C ../liblogger clean
	make -C ../libz clean
	
