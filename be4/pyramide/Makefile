TGTDIR=../../target
BINDIR=${TGTDIR}/bin
SHAREDIR=${TGTDIR}/share
SHAREPYRAMIDEDIR=${SHAREDIR}/pyramide
LOGDIR=${TGTDIR}/log
TMPDIR=${TGTDIR}/tmp
CONFDIR=${TGTDIR}/config
PYRAMIDSDIR=${CONFDIR}/pyramids
DOCDIR=${TGTDIR}/docs
DOCBE4DIR=${DOCDIR}/be4

PLSCRIPTS= $(wildcard *.pl)
PLMODULES= $(wildcard *.pm)
IMGSHARE= $(wildcard *.tif)

all: makedir
	cp $(PLSCRIPTS) $(BINDIR)
	cp $(PLMODULES) $(BINDIR)
	cp $(IMGSHARE) $(SHAREPYRAMIDEDIR)
	pngs="`ls $(SHAREPYRAMIDEDIR)/*.tif | grep _png`";for png in `echo $(pngs)`;do pngout=`echo $(png) | sed -e 's#_png##'`;`echo $(png)`;done; 
	#$(BINDIR)/tiffcp -c none $(png) $(pngout);done;
	cp gdalinfo $(BINDIR)
	chmod +x $(BINDIR)/*.pl
	chmod +x $(BINDIR)/gdalinfo
	cp ../../config/pyramids/pyramid.xsd $(PYRAMIDSDIR)
	cp dependances_scripts_perl.txt $(DOCBE4DIR)
	cp how_to_fr.txt $(DOCBE4DIR)
makedir:
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
	if [ ! -d $(SHAREPYRAMIDEDIR) ] ; then mkdir $(SHAREPYRAMIDEDIR) ; fi
	if [ ! -d $(LOGDIR) ] ; then mkdir $(LOGDIR) ; fi
	if [ ! -d $(TMPDIR) ] ; then mkdir $(TMPDIR) ; fi
	if [ ! -d $(CONFDIR) ] ; then mkdir $(CONFDIR) ; fi
	if [ ! -d $(PYRAMIDSDIR) ] ; then mkdir $(PYRAMIDSDIR) ; fi
	if [ ! -d $(DOCDIR) ] ; then mkdir $(DOCDIR) ; fi
	if [ ! -d $(DOCBE4DIR) ] ; then mkdir $(DOCBE4DIR) ; fi
