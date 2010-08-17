#!/bin/bash

# LD_LIBRARY_PATH pour librairies proj4
export LD_LIBRARY_PATH=/usr/local/lib

# building and testing
cd libwmts
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make clean cpputest
if [ $? -ne 0 ] ; then
  exit 1
fi


# documentation
echo "\n\n\n\n***  DOCUMENTATION    *****"
make doc
if [ $? -ne 0 ] ; then
  exit 2
fi


# deploiement
ROK4BASE=/var/www/hudson/rok4
echo "\n\n\n\n***  DEPLOIEMENT    *****"
cd ../target
rm -f $ROK4BASE/bin/WMSServer
cp bin/WMSServer $ROK4BASE/bin/
cp -r config/* $ROK4BASE/config/
cp -r docs/* $ROK4BASE/docs/
rm -f $ROK4BASE/builds/rok4-rev${SVN_REVISION}.tgz
tar -cvzf rok4-rev${SVN_REVISION}.tgz bin/WMSServer docs
mv rok4-rev${SVN_REVISION}.tgz $ROK4BASE/builds/

