#!/bin/bash

# LD_LIBRARY_PATH pour librairies proj4
# export LD_LIBRARY_PATH=/usr/local/lib

# building and testing
cd rok4
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make clean test
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
rm -fr $ROK4BASE/bin/
cp bin/rok4 $ROK4BASE/bin/
cp -r config/* $ROK4BASE/config/
cp -r docs/* $ROK4BASE/docs/
rm -f $ROK4BASE/builds/rok4-rev${SVN_REVISION}.tgz
tar -cvzf rok4-rev${SVN_REVISION}.tgz bin/rok4 docs
mv rok4-rev${SVN_REVISION}.tgz $ROK4BASE/builds/

