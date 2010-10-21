all : configure
	make -C proj-4.7.0 install

clean :
	if [ -e proj-4.7.0/Makefile ]; then make -C proj-4.7.0  uninstall distclean; fi

configure :
	cd proj-4.7.0; if [ ! -e Makefile ]; then CFLAGS="-O3"; ./configure --enable-shared=no --prefix=`pwd`/../../../target; fi
	export PROJ_LIB=../share/proj/
test:


