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

TIFF2GRAYOBJ= tiff2gray.o

all: clean tiff2gray

tiff2gray: $(TIFF2GRAYOBJ)
	$(CXX) $(TIFF2GRAYOBJ) -o tiff2gray $(LDLIBS) $(DEPLIB:%=$(LIBDIR)/%.a)
	mv tiff2gray $(BINDIR)
clean:
	rm -f *.o
