all : configure
	make -C cppunit-1.12.1 install

clean :
	if [ -e cppunit-1.12.1/Makefile ]; then make -C cppunit-1.12.1 uninstall distclean; fi

configure :
	cd cppunit-1.12.1; if [ ! -e Makefile ]; then CFLAGS="-O3"; ./configure --enable-shared=no --enable-doxygen=no --prefix=`pwd`/../../../target; fi

test:

# TODO : install, uninstall
