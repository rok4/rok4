CC = gcc

all: clean libfcgi

clean:
	rm -f config.log config.status stamp-h1 *.o

fcgi_config.h:
	./configure
	rm -f config.log config.status stamp-h1	

libfcgi : fcgi_config.h fcgiapp.o os_unix.o

