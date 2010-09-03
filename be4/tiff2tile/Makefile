TGTDIR=../../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
BINDIR=$(TGTDIR)/bin

CXX = g++
CC  = gcc
#CXXFLAGS = -O3 -Wall
CXXFLAGS = -g -Wall -I"." -I$(INCDIR)
LDFLAGS =
#LDLIBS = -ltiff -lz -ljpeg -static
LDLIBS = -static

# Liste de librairies dépendantes embarquées
DEPLIB= libtiff libz libjpeg

TIFF2TILEOBJ= TiffReader.o TiledTiffWriter.o tiff2tile.o

all: clean tiff2tile

tiff2tile: $(TIFF2TILEOBJ)
	$(CXX) $(TIFF2TILEOBJ) -o tiff2tile $(LDLIBS) $(DEPLIB:%=$(LIBDIR)/%.a)
	mv tiff2tile $(BINDIR)
	cp TiffReader.h $(INCDIR)
	cp TiledTiffWriter.h $(INCDIR)
clean:
	rm -f *.o
