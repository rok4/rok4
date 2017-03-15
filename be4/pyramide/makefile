# WARNING : You must fixe an environement export for BE4_ENV !
# ie BE4_ENV=../../BE4_INSTALL
# WARNING : You must fixe an environement export for BE4_SRC !
# ie BE4_SRC=../../BE4/SRC

BE4_BIN=$(BE4_ENV)/bin
BE4_LIB=$(BE4_ENV)/lib
BE4_CONF=$(BE4_ENV)/config/be4
BE4_HTML=$(BE4_ENV)/share/doc/Be4/html
BE4_DOC=$(BE4_ENV)/share/doc/Be4
BE4_MAN=$(BE4_ENV)/man
BE4_SHARE=$(BE4_ENV)/share

all: pyramid

pyramid: buildlib.pyr buildbin.pyr

%.pyr: %
	@ echo "> Ok, Pyramid $* !" ;

buildlib:
	@(cd . && perl Makefile.PL PREFIX=$(BE4_ENV) LIB=$(BE4_LIB)/perl5 && make -f Makefile) ;

buildbin: \
./bin/be4.pl \
./bin/joinCache.pl \
./bin/joinCache-s3.pl \
./bin/coord2image.pl \
./bin/be4-simulator.pl \
./bin/tms-converter-gui.pl \
./bin/be4-ceph.pl \
./bin/be4-s3.pl \
./bin/be4-swift.pl \
./bin/pyr2pyr.pl \
./bin/create-layer.pl \
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
	@(if [ ! -d $(BE4_CONF) ] ; then \
                install -d $(BE4_CONF) ;\
        fi)
	@(if [ ! -d $(BE4_MAN) ] ; then \
                install -d $(BE4_MAN) ;\
        fi)
	@(if [ ! -d $(BE4_HTML) ] ; then \
                install -d $(BE4_HTML) ;\
        fi)

buildinstall:
	@( cd . && make -f Makefile install ) ;

test:
	@( cd . && make -f Makefile test ) ;

doc: buildnatural.doc

%.doc: %
	@ echo "> Ok, Doc $* !" ;

buildnatural:
	@( if [ ! -d docs/Natural/Html ] ; then \
                install -d docs/Natural/Html ;\
		install -d docs/Natural/NaturalDocs ;\
	fi)
	@( if [ -f $(BE4_SRC)/../doc/MenuND.txt ] ; then \
		cp $(BE4_SRC)/../doc/MenuND.txt ./docs/Natural/NaturalDocs/Menu.txt ;\
	fi)
	@( naturaldocs -r --input ./ --exclude-input ./blib/ -img $(BE4_SRC)/../doc/images --no-auto-group --documented-only --output HTML ./docs/Natural/Html --project ./docs/Natural/NaturalDocs ) ;
	@( echo "> Ok, NaturalDocs !" ) ;


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
	@echo "    make buildnatural- ."
	@echo "    make buildbin    - ."
	@echo "    make builddir    - ."
	@echo "    make buildconf   - ."
	@echo "    make buildlib    - ."
	@echo " "
	@echo " Information install :"
	@echo " "
	@echo "     prefix      : $(BE4_ENV)"
	@echo "     bin         : $(BE4_BIN)" 
	@echo "     lib         : $(BE4_LIB)" 
	@echo "     config/be4  : $(BE4_CONF)"
	@echo "     share       : $(BE4_SHARE)"
	@echo "     man         : $(BE4_MAN)"
	@echo "     html        : $(BE4_HTML)"
	@echo " "

