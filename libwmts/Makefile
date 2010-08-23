
INCDIR=../include
LIBDIR=../lib


CXX = g++
CC  = gcc
CXXFLAGS = -O3 -msse -msse2 -msse3 -I"." -I$(INCDIR) 
#conf de debug:
#CXXFLAGS = -pg -O0 -msse2 -I"."  -I$(INCDIR)
LDFLAGS = -pg
LDLIBS = -llog4cxx -lpthread

# Liste de librairies dépendantes embarquées
DEPLIB= libcppunit libz libjpeg libfcgi libtinyxml liblogger libimage libproj

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

all: WMSServer
#
# WMSServer
# compile et genere le serveur WMS
WMSServer: $(DEPLIB) bindir $(WMSSOBJ)
	$(CXX) $(LDFLAGS) $(WMSSOBJ) -o WMSServer $(LDLIBS)  $(DEPLIB:%=$(LIBDIR)/%.a)
	cp WMSServer *.o $(FCGIOBJ) $(TIXMLOBJ) $(BINDIR)
	rm $(BINDIR)/WMSServer.o
	cp -r $(SRCCONFDIR)/* $(TGTCONFDIR)





# règle générique pour lancer les compilations des librairies dépendnates
lib%:
	test -e $(LIBDIR)/$@.a || make -C ../$@

# Lance le goal clean pour une librairie
clean-lib%:
	make -C ../lib$* clean

# Nettoie aussi les librairies dépendantes
very-clean: clean $(patsubst %, clean-%, $(DEPLIB))







#
# cpputest
# compile et execute les tests unitaires cppunit
cpputest: WMSServer cppubin
	$(CPPUBIN)

cppubin: $(DEPLIB) $(CPPUNITOBJ)
	$(CXX) $(LDFLAGS) $(CPPUNITOBJ) $(BINDIR)/*.o -o $(CPPUBIN) $(DEPLIB:%=$(LIBDIR)/%.a) $(LDLIBS)
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
	
