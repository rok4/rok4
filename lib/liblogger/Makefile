TGTDIR=../../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib

CXX = g++
CXXFLAGS = -O3

THISLIB=liblogger.a
OBJ= $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
HEADER= $(wildcard *.h)

all: tgtdir include archive
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	cp $(THISLIB) $(LIBDIR)

tgtdir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi

include: tgtdir
	if [ ! -d $(INCDIR) ] ; then mkdir $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

archive : $(THISLIB)($(OBJ))

clean:
	rm -f *.o $(THISLIB) $(LIBDIR)/$(THISLIB)

test:

#TODO : install, uninstall


