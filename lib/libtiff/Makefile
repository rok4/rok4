all : configure
	make -C tiff-3.8.2 install

clean :
	if [ -e tiff-3.8.2/Makefile ]; then make -C tiff-3.8.2 uninstall distclean; fi

configure :
	cd tiff-3.8.2; if [ ! -e Makefile ]; then CFLAGS="-O3"; ./configure --prefix=`pwd`/../../../target/ --with-zlib-lib-dir=`pwd`/../../../target/ --with-jpeg-lib-dir=`pwd`/../../../target/; fi
