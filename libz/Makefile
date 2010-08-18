all : configure
	make -C zlib-1.2.5 install

clean :
	if [ -e zlib-1.2.5/zlib.pc ]; then make -C zlib-1.2.5 uninstall distclean; fi

configure :
	cd zlib-1.2.5; if [ ! -e zlib.pc ]; then CFLAGS="-O3"; ./configure --static --prefix=../..; fi
