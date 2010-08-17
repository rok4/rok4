INCDIR=../include
LIBDIR=../lib

CXX = g++
CXXFLAGS = -O3

THISLIB=liblogger.a
OBJ= $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
HEADER= $(wildcard *.h)

all: include archive
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	cp $(THISLIB) $(LIBDIR)

include:
	if [ ! -d $(INCDIR) ] ; then mkdir $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

archive : $(THISLIB)($(OBJ))

clean:
	rm -f *.o $(THISLIB) $(LIBDIR)/$(THISLIB)


