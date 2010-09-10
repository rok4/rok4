#!/bin/bash

# LD_LIBRARY_PATH pour librairies proj4
# export LD_LIBRARY_PATH=/usr/local/lib

# building and testing
cd rok4
echo "\n\n\n\n***  BUILD AND TESTS    *****"
make very-clean test
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

echo "arret serveur apache..."
sudo /etc/init.d/apache2 stop
if [ -f /var/run/apache2.pid ] ; then
  echo "[warning] Pb lors de l'arret du service apache ! "
fi

rm -fr $ROK4BASE/bin/*
rm -fr $ROK4BASE/config/*
rm -f $ROK4BASE/builds/rok4-rev${SVN_REVISION}.tgz

cd ../target
cp bin/rok4 $ROK4BASE/bin/
cp -r config/* $ROK4BASE/config/
cp -r docs/* $ROK4BASE/docs/
tar -cvzf rok4-rev${SVN_REVISION}.tgz bin/rok4 docs
mv rok4-rev${SVN_REVISION}.tgz $ROK4BASE/builds/

echo "demarrage serveur apache..."
sudo /etc/init.d/apache2 start
if [ ! -f /var/run/apache2.pid ] ; then
  echo "[error] Pb lors du démarrage du service apache ! "
  exit 3
fi

