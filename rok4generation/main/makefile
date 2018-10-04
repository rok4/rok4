# WARNING : You must fixe an environement export for BE4_ENV !
# ie BE4_ENV=../../BE4_INSTALL
# WARNING : You must fixe an environement export for BE4_SRC !
# ie BE4_SRC=../../BE4/SRC

BE4_BIN=$(BE4_ENV)/bin
BE4_LIB=$(BE4_ENV)/lib

all: pyramid

pyramid: buildlib.pyr buildbin.pyr

%.pyr: %
	@ echo "> Ok, Pyramid $* !" ;

buildlib:
	@(cd . && perl Makefile.PL PREFIX=$(BE4_ENV) LIB=$(BE4_LIB)/perl5 && make -f Makefile) ;

buildbin: \
./bin/be4-file.pl \
./bin/vek4-file.pl \
./bin/be4-ceph.pl \
./bin/be4-s3.pl \
./bin/be4-swift.pl \
./bin/be4-simulator.pl \
./bin/joinCache-file.pl \
./bin/joinCache-s3.pl \
./bin/joinCache-ceph.pl \
./bin/coord2image.pl \
./bin/tms-converter-gui.pl \
./bin/sup-pyr.pl \
./bin/pyr2pyr.pl \
./bin/create-layer.pl \
./bin/create-list.pl \
./bin/exPyr.pl \
./bin/wmtSalaD.pl

	@(for p in $^ ; do \
                if [ `echo $$p|egrep -c '\.pl'` -ne 0 ] ; then \
                        perl -c -W -Iblib/lib/  $$p ;\
                        if [ $$? ] ; then \
	                        exit $$? ;\
                        fi \
                fi \
        done)
	

install: builddir.inst buildinstall.inst

%.inst: %
	@ echo "> Ok, Install $* !" ;


builddir:
	@(if [ ! -d $(BE4_ENV) ] ; then \
                install -d $(BE4_ENV) ;\
        fi)
	@(if [ ! -d $(BE4_BIN) ] ; then \
                install -d $(BE4_BIN) ;\
        fi)
	@(if [ ! -d $(BE4_LIB) ] ; then \
                install -d $(BE4_LIB) ;\
        fi)

buildinstall:
	@( cd . && make -f Makefile install ) ;

test:
	@( cd . && make -f Makefile test ) ;

clean:
	@( if [ -d docs/Natural/Html ] ; then \
                rm -rf docs/Natural/Html ;\
		rm -rf docs/Natural/NaturalDocs ;\
	fi)
	@( cd . && make -f Makefile realclean ) ; 
	@( echo "> Ok, Clean !" ) ;

uninstall: clean
	@( rm -rf $(BE4_ENV))
	@( echo "> Ok, Uninstall !" ) ;

manifest:
	@( cd . && make -f Makefile manifest) ;

help:
	@echo "BE4 Makefile"
	@echo ""
	@echo " target ():"
	@echo "    make           - Compile (local)."
	@echo "    make test      - Test."
	@echo "    make install   - Install into $(BE4_ENV)."
	@echo "    make doc       - Create doc."
	@echo "    make manifest  - ."
	@echo " sub target ():"
	@echo "    make buildinstall- ."
	@echo "    make builddir    - ."
	@echo "    make buildbin    - ."
	@echo "    make builddir    - ."
	@echo "    make buildlib    - ."
	@echo " "
	@echo " Information install :"
	@echo " "
	@echo "     prefix      : $(BE4_ENV)"
	@echo "     bin         : $(BE4_BIN)" 
	@echo "     lib         : $(BE4_LIB)"
	@echo " "

