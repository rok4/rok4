TGTDIR=../target
DOCDIR=${TGTDIR}/share/doc/be4

TOOLS= DallesBase merge4tiff pyramide tiff2tile
DOCS= $(wildcard *.txt)

all: docdir
	$(foreach tool, $(TOOLS), make -C $(tool);)
	cp $(DOCS) $(DOCDIR)

clean:
	make -C * clean

docdir:
	if [ ! -d $(DOCDIR) ] ; then mkdir $(DOCDIR) ; fi
