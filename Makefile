TGTDIR=./target

all: rok4

clean:
	rm -fr ${TGTDIR}

libs:
	make -C lib 

rok4: clean
	make -C rok4/

be4:
	make -C be4/




