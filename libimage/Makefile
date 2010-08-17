INCDIR=../include
LIBDIR=../lib

CXX = g++
CC  = gcc
CXXFLAGS = -O3  -msse2 -I"." -I$(INCDIR)
#conf de debug:
#CXXFLAGS = -g -O0 -msse2 -I"." 
LDFLAGS = -L$(LIBDIR)
LDLIBS = -llog4cxx
LDCPPUNITLIBS= -lcppunit

# CCFLAGS = -g -ansi
THISLIB=libimage.a
BINDIR=../target/bin

# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTestsImage
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

OBJ= $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
HEADER= $(wildcard *.h)

all : include archive
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	cp $(THISLIB) $(LIBDIR)

archive : $(THISLIB)($(OBJ))

clean:
	rm -f *.o $(CPPUNITDIR)/*.o $(THISLIB)	$(LIBDIR)/$(THISLIB) $(CPPUBIN)

include:
	if [ ! -d $(INCDIR) ] ; then mkdir $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

bindir:
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi

# cpputest
# compile et execute les tests unitaires cppunit
cpputest: archive cppubin
	$(CPPUBIN)

cppubin: bindir $(CPPUNITOBJ)
	$(CXX) $(CPPUNITOBJ) $(THISLIB) -o $(CPPUBIN) $(LDCPPUNITLIBS) $(LDLIBS)
