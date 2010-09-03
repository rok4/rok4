TGTDIR=../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
BINDIR=$(TGTDIR)/bin
TGTCONFDIR=$(TGTDIR)/config

SRCCONFDIR=../config
DOCSDIR=../docs

CXX = g++
CC  = gcc
CXXFLAGS = -g -O3 -msse -msse2 -msse3 -I"." -I$(INCDIR) 
#conf de debug:
#CXXFLAGS = -pg -O0 -msse2 -I"."  -I$(INCDIR)
LDFLAGS =
LDLIBS = -lpthread

# Liste de librairies dépendantes embarquées
DEPLIB= libimage liblogger libcppunit libz libjpeg libfcgi libtinyxml libproj   

# CCFLAGS = -g -ansi


# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTests
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

WMSSOBJ= WMSServer.o ConfLoader.o TileMatrix.o TileMatrixSet.o Layer.o ResponseSender.o Level.o Pyramid.o TiffEncoder.o PNGEncoder.o WMSRequest.o Tile.o

all: WMSServer
#
# WMSServer
# compile et genere le serveur WMS
WMSServer: bindir $(DEPLIB) $(WMSSOBJ)
	$(CXX) $(LDFLAGS) $(WMSSOBJ) -o WMSServer $(LDLIBS)  $(DEPLIB:%=$(LIBDIR)/%.a)
	cp WMSServer *.o $(BINDIR)
	rm $(BINDIR)/WMSServer.o
	mv $(BINDIR)/WMSServer $(BINDIR)/rok4
	cp -r $(SRCCONFDIR)/* $(TGTCONFDIR)


# règle générique pour lancer les compilations des librairies dépendnates
lib%:
	test -e $(LIBDIR)/$@.a || make -C ../lib/$@

# Lance le goal clean pour une librairie
clean-lib%:
	make -C ../lib/lib$* clean

# Nettoie aussi les librairies dépendantes
very-clean: clean $(patsubst %, clean-%, $(DEPLIB))

test: cpputest

#
# cpputest
# compile et execute les tests unitaires cppunit
cpputest: WMSServer cppubin
	$(CPPUBIN)

cppubin: $(CPPUNITOBJ)
	$(CXX) $(LDFLAGS) $(CPPUNITOBJ) $(BINDIR)/*.o -o $(CPPUBIN) $(DEPLIB:%=$(LIBDIR)/%.a) $(LDLIBS)
	cp $(CPPUNITOBJ) $(BINDIR)
	make -C ../lib/libimage cpputest # lance les tests unitaires sur libimage


bindir: 
	rm -fr $(TGTDIR)
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(TGTCONFDIR) ] ; then mkdir $(TGTCONFDIR) ; fi
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi

doc : 
	doxygen $(DOCSDIR)/doxygen.cfg

clean:
	rm -f *.o $(CPPUNITDIR)/*.o WMSServer	
	rm -fr $(BINDIR)
	
