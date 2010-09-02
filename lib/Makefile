INSTDIR=../target
TGTDIR=../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
BINDIR=${TGTDIR}/bin

LIBS= libcppunit libproj libfcgi libjpeg libtinyxml libz liblogger libimage



#all: tgtdir ldir incdir bindir $(LIBS)
all: tgtdir ldir incdir bindir 
	$(foreach lib, $(LIBS), test -e $(LIBDIR)/$(lib).a || make -C $(lib) ;)

clean: $(patsubst %, clean-%, $(LIBS))
	rm -fr ${INCDIR}
	rm -fr ${LIBDIR}
	#rm -f *.o $(CPPUNITDIR)/*.o $(THISLIB).a $(LIBDIR)/lib$(THISLIB).a $(CPPUBIN)

test: 
	$(foreach lib, $(LIBS), make -C $(lib) test ;)

tgtdir:
	if [ ! -d $(TGTDIR) ] ; then mkdir $(TGTDIR) ; fi

incdir: 
	if [ ! -d $(INCDIR) ] ; then mkdir -p $(INCDIR) ; fi

ldir: 
	if [ ! -d $(LIBDIR) ] ; then mkdir -p $(LIBDIR) ; fi

bindir: 
	if [ ! -d $(BINDIR) ] ; then mkdir -p $(BINDIR) ; fi


# Lance le goal clean pour une librairie
clean-lib%:
	make -C lib$* clean





