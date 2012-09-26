# WARNING : You must fixe an environement export for BE4_ENV !
# ie BE4_ENV=../../BE4_INSTALL

BE4_BIN=$(BE4_ENV)/bin
BE4_LIB=$(BE4_ENV)/lib
BE4_CONF=$(BE4_ENV)/config/be4
BE4_HTML=$(BE4_ENV)/html
BE4_DOC=$(BE4_ENV)/doc
BE4_MAN=$(BE4_ENV)/man
BE4_SHARE=$(BE4_ENV)/share

all: pyramid

pyramid: buildlib.pyr buildbin.pyr 

%.pyr: %
	@ echo "> Ok, Pyramid $* !" ;

buildlib:
	@( cd . && perl Makefile.PL PREFIX=$(BE4_ENV) LIB=$(BE4_LIB)/perl5 && make -f Makefile ) ;

buildbin: \
./bin/be4.pl \
./bin/joinCache.pl \
./bin/create-layer.pl
	@(for p in $^ ; do \
                if [ `echo $$p|egrep -c '\.pl'` -ne 0 ] ; then \
                        perl -c -Iblib/lib/  $$p ;\
                fi \
        done)


install: builddir.inst buildconf.inst buildinstall.inst

%.inst: %
	@ echo "> Ok, Install $* !" ;

buildconf: \
./conf/nodata \
./conf/params.properties \
./conf/be4.properties
	@(for p in $^ ; do \
                res=`echo $$p | sed -e 's/conf\///'` ;\
                if [ `echo $$res|grep -E 'properties'` ] ; then \
                        cp $$p $(BE4_CONF) ;\
                fi; \
                if [ `echo $$res|grep -E 'nodata'` ] ; then \
                        cp -r $$p $(BE4_CONF) ;\
                fi; \
        done)

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

doc: buildnatural.doc buildhtml.doc

%.doc: %
	@ echo "> Ok, Doc $* !" ;

buildnatural:
	@( if [ ! -d docs/Natural/Html ] ; then \
                install -d docs/Natural/Html ;\
		install -d docs/Natural/NaturalDocs ;\
	fi)
	@( naturaldocs -r -i bin/ -i lib/ -img docs/Natural/UML -o HTML docs/Natural/Html -p docs/Natural/NaturalDocs ) ;
	@( echo "> Ok, NaturalDocs !" ) ;

buildhtml:
	@( if [ ! -d docs/Html ] ; then \
                install -d docs/Html ;\
        fi)
	@(for f in `find lib -name "*.pm"` ;\
            do \
              h=`echo $$f | sed -e 's/\.pm/\.html/' -e 's!/!-!g'` ;\
              pod2html --index --title=$$f --header --infile=$$f --outfile=docs/Html/$$h ;\
          done)
	@(for f in `find bin -name "*.pl"` ;\
          do \
            h=`echo $$f | sed -e 's/\.pl/\.html/' -e 's!/!-!g'` ;\
            pod2html --index --title=$$f --header --infile=$$f --outfile=docs/Html/$$h ;\
          done)
	@( echo "> Ok, Man2Html !" ) ;

clean:
	@( if [ -d docs/Html ] ; then \
                rm -rf docs/Html ;\
        fi)
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
	@echo "    make buildhtml   - ."
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

