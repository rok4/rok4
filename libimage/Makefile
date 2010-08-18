INCDIR=../include
LIBDIR=../lib

CXX = g++
CC  = gcc
CXXFLAGS = -O3  -msse2 -I"." -I$(INCDIR)
#conf de debug:
#CXXFLAGS = -g -O0 -msse2 -I"." 
LDFLAGS = -L$(LIBDIR)
LDLIBS = -llog4cxx

DEPLIB= libcppunit


# CCFLAGS = -g -ansi
THISLIB=image
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
	cp $(THISLIB).a $(LIBDIR)/lib$(THISLIB).a

archive : $(THISLIB).a($(OBJ))

clean:
	rm -f *.o $(CPPUNITDIR)/*.o $(THISLIB).a $(LIBDIR)/lib$(THISLIB).a $(CPPUBIN)

include:
	if [ ! -d $(INCDIR) ] ; then mkdir $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

bindir:
	if [ ! -d $(BINDIR) ] ; then mkdir $(BINDIR) ; fi




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
cpputest: archive cppubin
	$(CPPUBIN)

cppubin: bindir $(DEPLIB) $(CPPUNITOBJ)
	$(CXX) $(CPPUNITOBJ) $(THISLIB) -o $(CPPUBIN) $(DEPLIB:%=$(LIBDIR)/%.a) $(LDLIBS)
