TGTDIR=../../target
BINDIR=${TGTDIR}/bin
SHAREDIR=${TGTDIR}/share/pyramide
LOGDIR=${TGTDIR}/log
TMPDIR=${TGTDIR}/tmp
CONFDIR=${TGTDIR}/config
PYRAMIDSDIR=${CONFDIR}/pyramids

PLSCRIPTS= $(wildcard *.pl)
PLMODULES= $(wildcard *.pm)
IMGSHARE= $(wildcard *.tif)

all: makedir
	cp $(PLSCRIPTS) $(BINDIR)
	cp $(PLMODULES) $(BINDIR)
	cp $(IMGSHARE) $(SHAREDIR)
	cp gdalinfo $(BINDIR)
	chmod +x $(BINDIR)/*.pl
	chmod +x $(BINDIR)/gdalinfo
	cp ../../config/pyramids/pyramid.xsd $(PYRAMIDSDIR)
makedir:
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
	if [ ! -d $(LOGDIR) ] ; then mkdir $(LOGDIR) ; fi
	if [ ! -d $(TMPDIR) ] ; then mkdir $(TMPDIR) ; fi
	if [ ! -d $(CONFDIR) ] ; then mkdir $(CONFDIR) ; fi
	if [ ! -d $(PYRAMIDSDIR) ] ; then mkdir $(PYRAMIDSDIR) ; fi
