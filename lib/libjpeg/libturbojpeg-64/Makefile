TGTDIR=../../../target

INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib

THISLIB=jpeg
HEADERS=$(wildcard *.h)

all:
	cp lib$(THISLIB).a $(LIBDIR)
	cp $(HEADERS) $(INCDIR)

clean:
	rm -f $(LIBDIR)/lib$(THISLIB).a
	rm -f $(HEADERS:%=$(LIBDIR)/%)

