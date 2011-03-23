dir=`pwd`

# building and testing
cd lib
echo "\n\n\n\n***  BUILD LIBS   *****"
make
if [ $? -ne 0 ] ; then
  exit 1
fi

cd $dir
cd be4
echo "\n\n\n\n***  BUILD BE4   *****"
make
if [ $? -ne 0 ] ; then
  exit 2
fi

#deploiement
ROK4BASE=/var/www/hudson/rok4-hg

cd ../target
cp bin/*.pl $ROK4BASE/bin/
cp bin/*.pm $ROK4BASE/bin/
cp bin/dalles_base $ROK4BASE/bin
cp bin/merge4tiff $ROK4BASE/bin
cp bin/tiff2tile $ROK4BASE/bin
cp bin/gdalinfo $ROK4BASE/bin
cp config/pyramids/pyramid.xsd $ROK4BASE/bin/
if [ ! -d $ROK4BASE/share/pyramide ] ; then mkdir $ROK4BASE/share/pyramide ; fi
cp share/pyramide/4096_4096_FFFFFF_gray.tif share/pyramide/4096_4096_FFFFFF_rgb.tif share/pyramide/mtd_4096_4096_black_32b.tif $ROK4BASE/share/pyramide
cp docs/be4/dependances_scripts_perl.txt $ROK4BASE/docs/be4
cp docs/be4/how_to_fr.txt $ROK4BASE/docs/be4
