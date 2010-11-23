#!/bin/bash

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
ROK4BASE=/var/www/hudson/rok4

cd ../target
cp bin/*.pl $ROK4BASE/bin/
cp bin/*.pm $ROK4BASE/bin/
cp bin/gdalinfo $ROK4BASE/bin
cp config/pyramids/pyramid.xsd $ROK4BASE/bin/
