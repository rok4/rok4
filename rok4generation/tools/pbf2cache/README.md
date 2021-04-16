# PBF2CACHE

[Vue générale](../../README.md#écriture-dune-dalle-vecteur)

![pbf2cache](../../../docs/images/ROK4GENERATION/tools/pbf2cache.png)

Cet outil écrit une dalle à partir des tuiles PBF rangées par coordonnées (`<dossier racine>/x/y.pbf`). La dalle écrite est au format ROK4, c'est-à-dire un fichier TIFF, dont les données sont tuilées : le TIFF ne sert que de conteneurs pour regrouper les tuiles PBF. L'en-tête est de taille fixe (2048 octets).

## Usage

`pbf2cache -r <DIRECTORY> -t <VAL> <VAL> -ultile <VAL> <VAL> <OUTPUT FILE/OBJECT> [-pool <POOL NAME>|-bucket <BUCKET NAME>|-container <CONTAINER NAME>] [-d]`

* `-r <DIRECTORY>` : dossier contenant l'arborescence de tuiles PBF
* `-t <VAL> <VAL>` : nombre de tuiles dans une dalle, en largeur et en hauteur
* `-ultile <VAL> <VAL>` : indice de la tuile en haut à gauche dans la dalle
* `-d` : activation des logs de niveau DEBUG
* `-pool <POOL NAME>` : précise le nom du pool CEPH dans lequel écrire la dalle
* `-bucket <BUCKET NAME>` : précise le nom du bucket S3 dans lequel écrire la dalle
* `-container <CONTAINER NAME>` : précise le nom du conteneur SWIFT dans lequel écrire la dalle

## Exemple

Avec la commande suivante : `pbf2cache -r /home/IGN/pbfs -t 3 2 -ultile 17 36 /home/IGN/output.tif` (on veut 3x2 tuiles dans une dalle, et l'indice de la tuile en haut à gauche est (17,36)), les fichiers suivants seront cherchés et intégrés à la dalle fichier `/home/IGN/output.tif` si présents dans cet ordre :

* `/home/IGN/pbfs/17/36.pbf`
* `/home/IGN/pbfs/18/36.pbf`
* `/home/IGN/pbfs/19/36.pbf`
* `/home/IGN/pbfs/17/37.pbf`
* `/home/IGN/pbfs/18/37.pbf`
* `/home/IGN/pbfs/19/37.pbf`

Si une tuile est absente (cela arrive si elle ne devait pas contenir d'objets), on précise dans la dalle que l'on a une tuile de taille 0.
