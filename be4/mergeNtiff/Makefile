TGTDIR=../../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
BINDIR=$(TGTDIR)/bin
CONFIGDIR=$(TGTDIR)/config
LOGDIR=$(TGTDIR)/log
TMPDIR=$(TGTDIR)/tmp

CXX = g++
CC  = gcc
#CXXFLAGS = -O3 -Wall
CXXFLAGS = -g -Wall -I"." -I$(INCDIR)
LDFLAGS =
#LDLIBS = -ltiff -lz -ljpeg -static
LDLIBS = -lpthread

# Liste de librairies dépendantes embarquées
DEPLIB= libtiff libz libjpeg liblogger libimage

MERGENTIFFOBJ= mergeNtiff.o

all: clean mergeNtiff

mergeNtiff: $(MERGENTIFFOBJ)
	if [ ! -d $(LOGDIR) ] ; then mkdir $(LOGDIR) ; fi
	if [ ! -d $(TMPDIR) ] ; then mkdir $(TMPDIR) ; fi
	$(CXX) $(MERGENTIFFOBJ) -o mergeNtiff $(LDLIBS) $(DEPLIB:%=$(LIBDIR)/%.a)
	mv mergeNtiff $(BINDIR)

clean:
	rm -f *.o
	rm -f mergeNtiff
