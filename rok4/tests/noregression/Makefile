TGTDIR=../../../target
CONFDIR=${TGTDIR}/config
LAYERDIR=${CONFDIR}/layers
PYRDIR=${CONFDIR}/pyramids
TMSDIR=${CONFDIR}/tileMatrixSet
SHAREDIR=${TGTDIR}/share

LAYERS=$(wildcard layers/*.lay)
TMS=$(wildcard tileMatrixSet/*.tms) 

all: makedir
	cp $(LAYERS) $(LAYERDIR)
	cp -rf pyramids/* $(PYRDIR)
	cp $(TMS) $(TMSDIR)
makedir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(CONFDIR) ] ; then mkdir $(CONFDIR) ; fi
	if [ ! -d $(LAYERDIR) ] ; then mkdir $(LAYERDIR) ; fi
	if [ ! -d $(PYRDIR) ] ; then mkdir $(PYRDIR) ; fi
	if [ ! -d $(TMSDIR) ] ; then mkdir $(TMSDIR) ; fi
