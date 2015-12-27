rok4
====

[![Join the chat at https://gitter.im/rok4/rok4](https://badges.gitter.im/rok4/rok4.svg)](https://gitter.im/rok4/rok4?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Rok4 est un serveur WMS/WMTS open source développé par l'IGN France.
[![Build Status](https://travis-ci.org/rok4/rok4.svg?branch=master)](https://travis-ci.org/rok4/rok4)

# A propos

Le serveur rok4 permet de servir des données géographiques raster selon les protocoles WMS ou WMTS.
Il est développé par l'équipe Géoportail de l'Institut National de l'information Géographique et Forestière (IGN).
Il est associé à l'outil Be4 qui permet de générer les caches de données utilisés par Rok4.

* http://www.ign.fr [@IGNFrance](https://twitter.com/IGNFrance)
* http://www.geoportail.gouv.fr [@Geoportail](https://twitter.com/Geoportail)

# Compiler
## L'environnement de compilation
`sudo apt-get install build-essential cmake`

Pour Rok4 :
`sudo apt-get install gettext nasm automake`

Pour Be4 :
`sudo apt-get install perl libxml2-dev libgdal-perl liblog-log4perl-perl libconfig-inifiles-perl libxml-libxml-simple-perl libfile-copy-link-perl`

Pour générer la documation du code :
`sudo apt-get install doxygen graphviz naturaldocs`

## La compilation
```
mkdir build 
cd build 
cmake .. [-DOPTION1 -DOPTION2]
make 
make [install|package]`
```

Exemple
-------
```
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$HOME/rok4/target ..
make
make doc
make install
```
