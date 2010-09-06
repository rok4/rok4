TGTDIR=../../target
BINDIR=${TGTDIR}/bin
SHAREDIR=${TGTDIR}/share/pyramide

PLSCRIPTS= $(wildcard *.pl)
PLMODULES= $(wildcard *.pm)

all: sharedir
	cp $(PLSCRIPTS) $(BINDIR)
	cp $(PLMODULES) $(BINDIR)

sharedir:
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
