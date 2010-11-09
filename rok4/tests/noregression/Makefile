TGTDIR=../../../target
CONFDIR=${TGTDIR}/config
LAYERDIR=${CONFDIR}/layers
PYRDIR=${CONFDIR}/pyramids
SHAREDIR=${TGTDIR}/share

LAYERS= $(wildcard layers/*.lay)

all: makedir
	cp $(LAYERS) $(LAYERDIR)
	cp -rf pyramids/* $(PYRDIR)
makedir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(CONFDIR) ] ; then mkdir $(CONFDIR) ; fi
	if [ ! -d $(LAYERDIR) ] ; then mkdir $(LAYERDIR) ; fi
	if [ ! -d $(PYRDIR) ] ; then mkdir $(PYRDIR) ; fi
