TGTDIR=../target
DOCDIR=${TGTDIR}/share/doc/be4

TOOLS= DallesBase merge4tiff pyramide tiff2tile

all: docdir
	$(foreach tool, $(TOOLS), make -C $(tool);)

clean:
	make -C * clean

docdir:
	if [ ! -d $(DOCDIR) ] ; then mkdir $(DOCDIR) ; fi
