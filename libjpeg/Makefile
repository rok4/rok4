all : configure
	make -C jpeg-8b install-includeHEADERS install-libLTLIBRARIES

clean :
	if [ -e jpeg-8b/Makefile ]; then make -C jpeg-8b uninstall-includeHEADERS uninstall-libLTLIBRARIES distclean; fi

configure :
	cd jpeg-8b; if [ ! -e Makefile ]; then CFLAGS="-O3"; ./configure --enable-shared=no --prefix=`pwd`/../..; fi
