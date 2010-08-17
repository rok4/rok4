all : configure
	make -C zlib-1.2.5 install

clean :
	make -C zlib-1.2.5 distclean

configure :
	cd zlib-1.2.5; if [ ! -e zlib.pc ]; then CFLAGS="-O3"; ./configure --static --prefix=../..; fi
