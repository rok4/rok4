TGTDIR=../../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib

CXX = g++
CC  = gcc
CXXFLAGS = -O3 -msse -msse2 -msse3 -I"." -I"$(INCDIR)"
#conf de debug:
#CXXFLAGS = -pg -O0 -msse2 -I"." -I$(INCDIR)
LDLIBS = -lpthread

DEPLIB= libcppunit libproj liblogger libtiff


# CCFLAGS = -g -ansi
THISLIB=image
BINDIR=${TGTDIR}/bin

# definitions pour CppUnit :
CPPUNITDIR=tests/cppunit
CPPUBIN = $(BINDIR)/CppUnitTestsImage
# les tests unitaires pris en compte sont ceux nommés CppUnit*.cpp
CPPUNIT = $(wildcard ${CPPUNITDIR}/CppUnit*.cpp)
CPPUNITOBJ  = ${CPPUNITDIR}/main.o $(patsubst %.cpp,%.o,  $(CPPUNIT))

OBJ= $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
HEADER= $(wildcard *.h)

all : tgtdir include $(DEPLIB) archive
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	
archive : $(THISLIB).a($(OBJ))
	cp $(THISLIB).a $(LIBDIR)/lib$(THISLIB).a

clean:
	rm -f *.o $(CPPUNITDIR)/*.o $(THISLIB).a $(LIBDIR)/lib$(THISLIB).a $(CPPUBIN)

tgtdir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi

include: tgtdir
	if [ ! -d $(INCDIR) ] ; then mkdir -p $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

bindir: tgtdir
	if [ ! -d $(BINDIR) ] ; then mkdir -p $(BINDIR) ; fi

test: cpputest


# règle générique pour lancer les compilations des librairies dépendnates
lib%:
	test -e $(LIBDIR)/$@.a || make -C ../$@

# Lance le goal clean pour une librairie
clean-lib%:
	make -C ../lib$* clean

# Nettoie aussi les librairies dépendantes
very-clean: clean $(patsubst %, clean-%, $(DEPLIB))




# cpputest
# compile et execute les tests unitaires cppunit
#cpputest: archive cppubin
cpputest: cppubin
	$(CPPUBIN)

cppubin: include bindir $(DEPLIB) archive $(CPPUNITOBJ)
	$(CXX) $(CPPUNITOBJ) $(THISLIB).a -o $(CPPUBIN) $(DEPLIB:%=$(LIBDIR)/%.a) $(LDLIBS)
