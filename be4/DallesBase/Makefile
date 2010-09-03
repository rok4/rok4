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
LDLIBS = -lpthread

# Liste de librairies dépendantes embarquées
DEPLIB= libtiff libz libjpeg liblogger libimage

DALLESBASEOBJ= CDalle.o DallesBase.o

all: clean dalles_base

dalles_base: $(DALLESBASEOBJ)
	$(CXX) $(DALLESBASEOBJ) -o dalles_base $(LDLIBS) $(DEPLIB:%=$(LIBDIR)/%.a)
	cp dalles_base $(BINDIR)

clean:
	rm -f *.o
	rm -f dalles_base
