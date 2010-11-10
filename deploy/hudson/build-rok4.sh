#!/bin/bash


# building and testing
cd rok4
echo "\n\n\n\n***  BUILD AND TESTS    *****"
#make very-clean test
make very-clean
make Rok4Server
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
rm -fr $ROK4BASE/tests
mkdir $ROK4BASE/tests

# Copie des donnees pour les tests de non-regression
make noregression

cd ../target
cp bin/rok4 $ROK4BASE/bin/
cp -r config/* $ROK4BASE/config/
cp -r docs/* $ROK4BASE/docs/
tar -cvzf rok4-rev${SVN_REVISION}.tgz bin/rok4 docs
mv rok4-rev${SVN_REVISION}.tgz $ROK4BASE/builds/
cp -r ../rok4/tests/html/* $ROK4BASE/tests


# copie page de test

echo "demarrage serveur apache..."
sudo /etc/init.d/apache2 start
echo "on laisse le temps de demarrer... c'est long, mais compare aux test unitaires, c'est rien..."
sleep 20
if [ ! -f /var/run/apache2.pid ] ; then
  echo "[error] Pb lors du démarrage du service apache ! "
  exit 3
fi

# Lancement des tests de non regression
cd ../rok4/tests/noregression
bash tests.sh
