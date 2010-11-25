#!/bin/bash


# building and testing
cd rok4
echo "\n\n\n\n***  BUILD AND TESTS    *****"
#make very-clean test
make very-clean
make Rok4Server test
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

echo "-> Arret serveur apache..."
sudo /etc/init.d/apache2 stop
if [ -f /var/run/apache2.pid ] ; then
  echo "[warning] Pb lors de l'arret du service apache ! "
fi

echo "-> Suppression/creation des repertoires d'installation"
rm -fr $ROK4BASE/bin
mkdir $ROK4BASE/bin
rm -fr $ROK4BASE/config
mkdir $ROK4BASE/config
rm -fr $ROK4BASE/docs
mkdir $ROK4BASE/docs
rm -fr $ROK4BASE/share
mkdir $ROK4BASE/share
rm -fr $ROK4BASE/log
mkdir $ROK4BASE/log
chmod o+w $ROK4BASE/log
if [ ! -d $ROK4BASE/tmp ] ; then mkdir $ROK4BASE/tmp ; fi
chmod o+w $ROK4BASE/tmp
if [ ! -d $ROK4BASE/builds ] ; then mkdir $ROK4BASE/builds ; fi
rm -f $ROK4BASE/builds/rok4-rev${SVN_REVISION}.tgz
rm -fr $ROK4BASE/tests
mkdir $ROK4BASE/tests

# Copie des donnees pour les tests de non-regression
echo "-> Copie des donnees pour les tests de non-regression"
make noregression

# Copie des fichiers dans les repertoires d installation
echo "-> Copie des fichiers dans les repertoires d installation"
cd ../target/bin
bins=`ls | grep -v "\.o"`
cp $bins $ROK4BASE/bin/
cd ..
cp -r config/* $ROK4BASE/config/
cp -r docs/* $ROK4BASE/docs/
tar -cvzf rok4-rev${SVN_REVISION}.tgz bin/rok4 docs
mv rok4-rev${SVN_REVISION}.tgz $ROK4BASE/builds/
cp -r ../rok4/tests/html/* $ROK4BASE/tests


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
bash tests.sh http://localhost/rok4/bin/rok4
if [ $? -ne 0 ] ; then
	echo "[error] Echec des tests de non regression"
	exit 4
fi

exit 0
