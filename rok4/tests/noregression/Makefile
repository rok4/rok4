TGTDIR=../../../target
CONFDIR=${TGTDIR}/config
LAYERDIR=${CONFDIR}/layers
PYRDIR=${CONFDIR}/pyramids
SHAREDIR=${TGTDIR}/share
NOREGDIR=${SHAREDIR}/noregression


LAYERS= $(wildcard layers/*.lay)
PYRAMIDS= $(wildcard pyramids/*)
TEMPLATES= $(wildcard share/*)

all: makedir
	cp $(LAYERS) $(LAYERDIR)
	cp -r $(PYRAMIDS) $(PYRDIR)
	cp $(TEMPLATES) $(NOREGDIR)

makedir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(CONFDIR) ] ; then mkdir $(CONFDIR) ; fi
	if [ ! -d $(LAYERDIR) ] ; then mkdir $(LAYERDIR) ; fi
	if [ ! -d $(PYRDIR) ] ; then mkdir $(PYRDIR) ; fi
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
	if [ ! -d $(NOREGDIR) ] ; then mkdir $(NOREGDIR) ; fi

