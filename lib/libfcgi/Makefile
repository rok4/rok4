TGTDIR=../../target

INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib

THISLIB=libfcgi.a
OBJ= $(patsubst %.c, %.o, $(wildcard *.c)) 
HEADER= $(wildcard *.h)

CC = gcc
CFLAGS = -O3

all: tgtdir fcgi_config.h include archive 
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	cp $(THISLIB) $(LIBDIR)

tgtdir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi

include: tgtdir
	if [ ! -d $(INCDIR) ] ; then mkdir $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

archive : $(THISLIB)($(OBJ))

clean:
	rm -f *.o $(THISLIB) $(LIBDIR)/$(THISLIB) config.log config.status stamp-h1

fcgi_config.h:
	./configure
	rm -f config.log config.status stamp-h1

test:
# TODO : install, uninstall
