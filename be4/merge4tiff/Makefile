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

MERGE4TIFFOBJ= merge4tiff.o

all: clean merge4tiff

merge4tiff: $(MERGE4TIFFOBJ)
	$(CXX) $(MERGE4TIFFOBJ) -o merge4tiff $(LDLIBS) $(DEPLIB:%=$(LIBDIR)/%.a)
	mv merge4tiff $(BINDIR)

clean:
	rm -f *.o

