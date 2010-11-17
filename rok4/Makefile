TGTDIR=../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
BINDIR=$(TGTDIR)/bin
TGTCONFDIR=$(TGTDIR)/config
DOCDIR=$(TGTDIR)/docs
DOCROK4DIR=$(DOCDIR)/rok4

SRCCONFDIR=../config
SRCDOCDIR=../docs

CXX = g++
CC  = gcc
CXXFLAGS = -g -O3 -msse -msse2 -msse3 -I"." -I$(INCDIR) 
#conf de debug:
#CXXFLAGS = -pg -O0 -msse2 -I"."  -I$(INCDIR)
LDFLAGS =
LDLIBS = -lpthread

# Liste de librairies dépendantes embarquées
DEPLIB= libz libjpeg libfcgi libtinyxml libcppunit liblogger libimage libproj

# CCFLAGS = -g -ansi


# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTests
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

WMSSOBJ= ServiceException.o Message.o Rok4Server.o ConfLoader.o TileMatrix.o TileMatrixSet.o Layer.o ResponseSender.o Level.o Pyramid.o TiffEncoder.o JPEGEncoder.o PNGEncoder.o Request.o Tile.o CapabilitiesBuilder.o FileDataSource.o CRS.o

all: Rok4Server	noregression
#
# Rok4Server
# compile et genere le serveur WMS
Rok4Server: bindir $(DEPLIB) $(WMSSOBJ)
	$(CXX) $(LDFLAGS) $(WMSSOBJ) -o Rok4Server $(LDLIBS)  $(DEPLIB:%=$(LIBDIR)/%.a)
	cp Rok4Server $(BINDIR)/rok4
	cp -rf $(SRCCONFDIR)/* $(TGTCONFDIR)

# règle générique pour lancer les compilations des librairies dépendnates
lib%:
	test -e $(LIBDIR)/$@.a || make -C ../lib/$@

# Lance le goal clean pour une librairie
clean-lib%:
	make -C ../lib/lib$* clean

# Nettoie aussi les librairies dépendantes
very-clean: clean $(patsubst %, clean-%, $(DEPLIB))

test: cpputest

# cpputest
# compile et execute les tests unitaires cppunit
cpputest: Rok4Server cppubin
	$(CPPUBIN)

cppubin: $(CPPUNITOBJ)
	$(CXX) $(LDFLAGS) $(WMSSOBJ:Rok4Server.o=) $(CPPUNITOBJ) -o $(CPPUBIN) $(DEPLIB:%=$(LIBDIR)/%.a) $(LDLIBS)
	cp $(CPPUNITOBJ) $(BINDIR)
	make -C ../lib/libimage test  # lance les tests unitaires sur libimage
	make -C ../lib/liblogger test # lance les tests unitaires sur libimage

noregression:
	make -C tests/noregression all

#       rm -fr $(TGTDIR)
bindir: 
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(TGTCONFDIR) ] ; then mkdir $(TGTCONFDIR) ; fi
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	if [ ! -d $(DOCDIR) ] ; then mkdir $(DOCDIR) ; fi
	if [ ! -d $(DOCROK4DIR) ] ; then mkdir $(DOCROK4DIR) ; fi

doc : bindir 
	doxygen $(SRCDOCDIR)/doxygen_rok4.cfg

clean:
	rm -f *.o $(CPPUNITDIR)/*.o Rok4Server	
	rm -fr $(BINDIR)
	
