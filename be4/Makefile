TGTDIR=../target
SHAREDIR=${TGTDIR}/share
DOCDIR=${TGTDIR}/share/doc
DOCBE4DIR=${TGTDIR}/share/doc/be4

TOOLS= DallesBase merge4tiff pyramide tiff2tile
DOCS= $(wildcard *.txt)

all: dirs
	$(foreach tool, $(TOOLS), make -C $(tool);)
	if [ ! -z $(DOCS) ] ; then cp $(DOCS) $(DOCDIR) ; fi

clean:
	make -C * clean

dirs:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi
	if [ ! -d $(SHAREDIR) ] ; then mkdir $(SHAREDIR) ; fi
	if [ ! -d $(DOCDIR) ] ; then mkdir $(DOCDIR) ; fi
	if [ ! -d $(DOCBE4DIR) ] ; then mkdir $(DOCBE4DIR) ; fi
