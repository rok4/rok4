TGTDIR=../../target
BINDIR=${TGTDIR}/bin
SHAREDIR=${TGTDIR}/share/pyramide

PLSCRIPTS= $(wildcard *.pl)
PLMODULES= $(wildcard *.pm)
IMGSHARE= $(wildcard *.tif)

all: sharedir
	cp $(PLSCRIPTS) $(BINDIR)
	cp $(PLMODULES) $(BINDIR)
	cp $(IMGSHARE) $(SHAREDIR)

sharedir:
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
