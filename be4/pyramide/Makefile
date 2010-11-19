TGTDIR=../../target
BINDIR=${TGTDIR}/bin
SHAREDIR=${TGTDIR}/share/pyramide
LOGDIR=${TGTDIR}/log
TMPDIR=${TGTDIR}/tmp

PLSCRIPTS= $(wildcard *.pl)
PLMODULES= $(wildcard *.pm)
IMGSHARE= $(wildcard *.tif)

all: makedir
	cp $(PLSCRIPTS) $(BINDIR)
	chmod +x $(BINDIR)/$(PLSCRIPTS)
	cp $(PLMODULES) $(BINDIR)
	cp $(IMGSHARE) $(SHAREDIR)
	cp gdalinfo $(BINDIR)
	chmod +x $(BINDIR)/gdalinfo
makedir:
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
	if [ ! -d $(LOGDIR) ] ; then mkdir $(LOGDIR) ; fi
	if [ ! -d $(TMPDIR) ] ; then mkdir $(TMPDIR) ; fi
