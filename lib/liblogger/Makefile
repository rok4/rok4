TGTDIR=../../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
BINDIR=${TGTDIR}/bin
TESTDIR=tests/cppunit


BINTEST=$(TESTDIR)/test

CXX = g++
CXXFLAGS = -O0 -g -I$(INCDIR)
LDLIBS = -lpthread


THISLIB=logger
OBJ= $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
HEADER= $(wildcard *.h)


all: libdir include archive
	if [ ! -d $(LIBDIR) ] ; then mkdir $(LIBDIR) ; fi
	cp $(THISLIB).a $(LIBDIR)/lib$(THISLIB).a

libdir:
	if [ ! -d $(LIBDIR) ] ; then mkdir -p $(LIBDIR) ; fi

include:
	if [ ! -d $(INCDIR) ] ; then mkdir -p $(INCDIR) ; fi
	cp $(HEADER) $(INCDIR)

bindir:
	if [ ! -d $(BINDIR) ] ; then mkdir -p $(BINDIR) ; fi

archive : $(THISLIB).a($(OBJ))

clean:
	rm -f *.o $(THISLIB).a $(LIBDIR)/$(THISLIB).a $(TESTDIR)/*.o $(BINTEST)


# règle générique pour lancer les compilations des librairies dépendnates
lib%:
	test -e $(LIBDIR)/$@.a || make -C ../$@

$(BINTEST): include bindir archive libcppunit
	$(CXX) $(CXXFLAGS) $(TESTDIR)/*.cpp $(THISLIB).a $(LIBDIR)/libcppunit.a $(LDLIBS) -o $(BINTEST)

test: $(BINTEST)
	$(BINTEST)
        

#TODO : install, uninstall


