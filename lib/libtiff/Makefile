TGTDIR=../../target
INCDIR=${TGTDIR}/include
LIBDIR=${TGTDIR}/lib
SRCLIBDIR=..

DEPLIB=libz libjpeg

all : $(DEPLIB) configure
	make -C tiff-3.8.2 install

# règle générique pour lancer les compilations des librairies dépendnates
lib%:
	test -e $(LIBDIR)/$@.a || make -C $(SRCLIBDIR)/$@

clean :
	if [ -e tiff-3.8.2/Makefile ]; then make -C tiff-3.8.2 uninstall distclean; fi

configure :
	cd tiff-3.8.2; if [ ! -e Makefile ]; then CFLAGS="-O3"; ./configure --prefix=`pwd`/../$(TGTDIR) --with-zlib-lib-dir=`pwd`/../$(LIBDIR) --with-zlib-include-dir=`pwd`/../$(INCDIR) --with-jpeg-lib-dir=`pwd`/../$(LIBDIR) --with-jpeg-include-dir=DIR=`pwd`/../$(INCDIR) --enable-shared=no; fi
