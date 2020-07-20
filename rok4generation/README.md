![Logo ROK4GENERATION](../docs/images/rok4generation.png)

La suite d'outils ROK4GENERATION permet de générer, mettre à jour, composer, supprimer ou extraire des pyramide raster ou vecteur. Ces outils sont écrits en Perl. Le travail de génération peut nécessiter l'utilisation d'outils de traitement d'images (réechantillonnage, reprojection, décimation, composition) écrits en C++.

<!-- TOC START min:1 max:3 link:true update:true -->
- [Outils principaux](#outils-principaux)
    - [Génération de pyramide raster](#génération-de-pyramide-raster)
        - [La suite BE4](#la-suite-be4)
        - [La suite JOINCACHE](#la-suite-joincache)
    - [Génération de pyramide à la demande](#génération-de-pyramide-à-la-demande)
    - [Génération de pyramide vecteur](#génération-de-pyramide-vecteur)
        - [La suite 4ALAMO](#la-suite-4alamo)
    - [Gestion des pyramides](#gestion-des-pyramides)
        - [Suppression de pyramide](#suppression-de-pyramide)
        - [Réecriture d'une tête de pyramide](#réecriture-dune-tête-de-pyramide)
        - [Transfert de pyramide](#transfert-de-pyramide)
    - [Les outils de débogage](#les-outils-de-débogage)
        - [Création d'un descripteur de couche](#création-dun-descripteur-de-couche)
        - [Création du fichier liste d'une pyramide](#création-du-fichier-liste-dune-pyramide)
        - [Convertisseur TMS](#convertisseur-tms)
- [Outils de manipulation](#outils-de-manipulation)
    - [Manipulation raster](#manipulation-raster)
        - [Passage au format de travail d'une dalle ROK4](#passage-au-format-de-travail-dune-dalle-rok4)
        - [Contrôle d'une image de travail](#contrôle-dune-image-de-travail)
        - [Fusion d'un dallage d'images](#fusion-dun-dallage-dimages)
        - [Décimation d'une image](#décimation-dune-image)
        - [Gestion du nodata](#gestion-du-nodata)
        - [Sous réechantillonnage de 4 images](#sous-réechantillonnage-de-4-images)
        - [Réechantillonnage et reprojection d'images](#réechantillonnage-et-reprojection-dimages)
        - [Superposition d'images](#superposition-dimages)
        - [Stockage final en dalle](#stockage-final-en-dalle)
    - [Manipulation vecteur](#manipulation-vecteur)
        - [Écriture d'une dalle vecteur](#écriture-dune-dalle-vecteur)

<!-- TOC END -->


# Outils principaux

Écrits en Perl.

Quand un outil est dit parallélisable, c'est qu'il identifie le travail à faire, le partage équitablement et écrit les scripts Shell (nombre configurable). C'est alors l'exécution de ces scripts qui fait réellement le travail (calcul de dalles, copie de pyramide...).

Lorsque l'on est dans ce cas parallélisable, il est possible que les scripts Shell sachent faire de la reprise sur erreur. Dans chaque dossier temporaire individuel, un fichier liste contient le travail déjà réalisé. Au lancement du script, si ce fichier liste existe déjà, il identifie la dernière dalle générée et ignorera toutes les instructions jusqu'à retomber sur cette dalle. On peut donc en cas d'erreur relancer le script sans paramétrage et reprendre où il en était à l'exécution précédente.

De même, un fichier .prog à côté du script peut être mis à jour avec le pourcentage de progression (calculé à partir des lignes du script).

## Génération de pyramide raster

### La suite BE4

Outils : `be4-file.pl`, `be4-ceph.pl`, `be4-s3.pl`, `be4-swift.pl`

Les outils BE4 génèrent une pyramide raster à partir d'images géoréférencées ou d'un service WMS. Ils permettent de mettre à jour une pyramide raster existante. Si des images sont en entrée, elles peuvent être converties à la volée dans le format de la pyramide en sortie.

Stockages gérés : FICHIER, CEPH, S3, SWIFT

Parallélisable, reprise sur erreur, progression.

Outils internes utilisés :
* cache2work
* checkWork
* composeNtiff
* decimateNtiff
* merge4tiff
* mergeNtiff
* work2cache

Outils externes utilisés :
* wget

_Étape 1_
![BE4 étape 1](../docs/images/ROK4GENERATION/be4_part1.png)

_Étape 2 (QTree)_
![BE4 étape 2 QTree](../docs/images/ROK4GENERATION/be4_part2_qtree.png)
_Étape 2 (NNGraph)_
![BE4 étape 2 NNGraph](../docs/images/ROK4GENERATION/be4_part2_nngraph.png)

[Détails](./main/bin/be4.md)

### La suite JOINCACHE

Outils : `joinCache-file.pl`, `joinCache-ceph.pl`, `joinCache-s3.pl`

Les outils JOINCACHE génèrent une pyramide raster à partir d'autres pyramide raster compatibles (même TMS, dalles de même dimensions, canaux au même format). La composition se fait verticalement (choix des pyramides sources par niveau) et horizontalement (choix des pyramides source par zone au sein d'un niveau). La fusion de plusieurs dalles sources peut se faire selon plusieurs méthodes (masque, alpha top, multiplication)

Stockages gérés : FICHIER, CEPH, S3

Parallélisable, reprise sur erreur, progression.

Outils internes utilisés :
* cache2work
* overlayNtiff
* work2cache


_Étape 1_
![JOINCACHE étape 1](../docs/images/ROK4GENERATION/joinCache_part1.png)

_Étape 2_
![JOINCACHE étape 2](../docs/images/ROK4GENERATION/joinCache_part2.png)

[Détails](./main/bin/joincache.md)

## Génération de pyramide à la demande

Outil : `wmtSalaD.pl`

Une pyramide à la demande ne contient pas de données à la génération. Cela consiste en un simple descripteur de pyramide renseignant les sources à utiliser pour répondre aux requêtes WMTS.

[Détails](./main/bin/wmtsalad.md)

## Génération de pyramide vecteur

### La suite 4ALAMO

Outils : `4alamo-file.pl`, `4alamo-ceph.pl`

Les outils 4ALAMO génèrent une pyramide vecteur à partir d'une base de données PostgreSQL. Ils permettent de mettre à jour une pyramide vecteur existante.

Stockages gérés : FICHIER, CEPH

Parallélisable, reprise sur erreur, progression.

Outils internes utilisés :
* pbf2cache

Outils externes utilisés :
* ogr2ogr
* tippecanoe

_Étape 1_
![4ALAMO étape 1](../docs/images/ROK4GENERATION/4alamo_part1.png)

_Étape 2_
![4ALAMO étape 2](../docs/images/ROK4GENERATION/4alamo_part2.png)

[Détails](./main/bin/4alamo.md)

## Gestion des pyramides

### Suppression de pyramide

Outil : `sup-pyr.pl`

Cet outil supprime une pyramide à partir de son descripteur. Pour une pyramide stockée en fichier, il suffit de supprimer le dossier des données. Dans le cas de stockage objet, le fichier liste est parcouru et les dalles sont supprimées une par une.

Stockages gérés : FICHIER, CEPH, S3, SWIFT

#### Commande

`sup-pyr.pl --pyr=file [--full] [--stop] [--help|--usage|--version]`

#### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--pyr` Précise le chemin vers le descripteur de la pyramide à supprimer
* `--full` Précise si on supprime également le fichier liste et le descripteur de la pyramide à la fin
* `--stop` Précise si on souhaite arrêter la suppression lorsqu'une erreur est rencontrée


### Réecriture d'une tête de pyramide

Outil : `4head.pl`

Cet outil permet de regénérer des niveaux de la pyramide en partant d'un de ses niveaux. La pyramide est modifiée et sa liste, qui fait foi en terme de contenu de la pyramide, est mise à jour pour toujours correspondre au contenu final de la pyramide. L'outil perl modifie la liste et le descripteur et génère des script shell dont l'exécution modifiera les dalles de la pyramide. Seuls les niveaux entre celui de référence (non inclus) et le niveau du haut fournis (inclus) sont modifiés. Potentiellement des nouveaux niveaux sont ajoutés (lorsque l'outil est utilisé pour construire la tête de la pyramide qui n'existait pas).

Par défaut, l'outil génère deux scripts (`SCRIPT_1.sh` et `SCRIPT_FINISHER.sh`). Si on précise un niveau de parallélisation (via l'option `--parallel`) de N, on aura alors N scripts `SCRIPT_X.sh` et toujours `SCRIPT_FINISHER.sh` pour regénérer l'ensemble des dalles. Tous les scripts `SCRIPT_X.sh` peuvent être exécuter en parallèle, mais il faut attendre la fin de tous ces scripts pour lancer `SCRIPT_FINISHER.sh`.

Le script `main.sh` permet de lancer proprement tous ces scripts sur la même machine. Il ne permet donc pas de répartir les exécutions sur un pool de machine. L'appel à faire est loggé en fin d'exécution de `4head.pl`.

Stockages gérés : FICHIER, CEPH, S3, SWIFT

Parallélisable.

Types de pyramides gérés : RASTER QTREE

#### Commande

`4head.pl --pyr /home/ign/PYRAMID.pyr --tmsdir /home/ign/TMS/ --reference-level 19 --top-level 4 --tmp /home/ign/tmp/ --scripts /home/ign/scripts/ [--parallel 10] [--help|--usage|--version]`

#### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--pyr` Précise le chemin vers le descripteur de la pyramide à modifier
* `--tmsdir` Précise le dossier contenant au moins le TMS utilisé par la pyramide à modifier
* `--reference-level` Précise le niveau de la pyramide d'où partir pour regénérer les niveaux supérieurs
* `--top-level` Précise le niveau jusqu'auquel regénérer les dalles
* `--tmp` Précise un dossier à utiliser comme espace temporaire de génération
* `--script` Précise un dossier où écrire les scripts
* `--parallel` Précise le nombre de scripts pour modifier les dalles du niveau au dessus du niveau de référence (Optionnel, 1 par défaut)

### Transfert de pyramide

Outil : `pyr2pyr.pl`

Cet outil copie une pyramide d'un stockage à un autre.

Conversions possibles :
* FICHIER -> FICHIER, CEPH, S3, SWIFT
* CEPH -> CEPH, FICHIER

Parallélisable.

_Étape 1_
![PYR2PYR étape 1](../docs/images/ROK4GENERATION/pyr2pyr_part1.png)

_Étape 2_
![PYR2PYR étape 2](../docs/images/ROK4GENERATION/pyr2pyr_part2.png)

[Détails](./main/bin/pyr2pyr.md)


## Les outils de débogage

### Création d'un descripteur de couche

Outil : `create-layer.pl`

Cet outil génère un descripteur de couche pour ROK4SERVER à partir du descripteur de pyramide et du dossier des TileMatrixSets. Il est basique (titre, nom de couche, résumé par défaut) mais fonctionnel. La couche utilisera alors la pyramide en entrée dans sa globalité.

[Détails](./main/bin/create-layer.md)

### Création du fichier liste d'une pyramide

Outil : `create-list.pl`

Cet outil génère le fichier liste pour une pyramide fichier qui n'en aurait pas, à partir du dossier des données.

Stockage géré : FICHIER

[Détails](./main/bin/create-list.md)

### Convertisseur TMS

Outil : `tms-toolbox.pl`

Ce outil permet de réaliser de nombreuses conversion entre indices de dalles, de tuiles, requêtes getTile ou getMap, liste de fichiers, géométrie WKT... grâce au TMS utilisé (ne nécessite pas de pyramide).

[Détails](./main/bin/tms-toolbox.md)


# Outils de manipulation

Écrits en C++.

Plus de détails dans les dossiers des outils.

Voici la légende utilisée pour identifié le format des images dans les documentations par commande :

![Formats](../docs/images/ROK4GENERATION/tools/formats.png)

## Manipulation raster

### Passage au format de travail d'une dalle ROK4

Outil : `cache2work`

Cet outil transforme une dalle ROK4 raster en une image TIFF de même dimension mais non tuilée et potentiellement non compressée. Il est utilisé lorsque l'on veut retravailler une dalle d'une pyramide.

![cache2work](../docs/images/ROK4GENERATION/tools/cache2work.png)  

[Détails](./tools/cache2work/README.md)

### Contrôle d'une image de travail

Outil : `checkWork`

Cet outil prend seulement une image en entrée et tente de la lire. Cela permet de détecter d'éventuelles erreurs.

[Détails](./tools/checkWork/README.md)

### Fusion d'un dallage d'images

Outil : `composeNtiff`

Cet outil fusionne plusieurs images disposées en un dallage régulier en une seule. Il est utilisé lorsqu'une image est moissonnée en plusieurs fois à cause de sa taille, pour recomposer l'image désirée.

![composeNtiff](../docs/images/ROK4GENERATION/tools/composeNtiff.png)

[Détails](./tools/composeNtiff/README.md)

### Décimation d'une image

Outil : `decimateNtiff`

Cet outil génère une image à partir de plusieurs image en phase entre elles (même résolution et même décalage) en ne gardant qu'un pixel sur N. Cet outil est utilisé pour générer une dalle d'un niveau à partir de dalles d'un niveau inférieur dans le cas d'une pyramide utilisant un TileMatrixSet "plus proche voisin" (une valeur de la donnée source n'est pas interpolée et se retrouve dans les niveaux supérieurs).

![decimateNtiff](../docs/images/ROK4GENERATION/tools/decimateNtiff.png)

[Détails](./tools/decimateNtiff/README.md)

### Gestion du nodata

Outil : `manageNodata`

Cet outil permet d'identifier et de modifier une couleur dans une image considérée comme du nodata. Il permet également d'écrire le masque associé à l'image sur la base de cette valeur de nodata, et de réserver cette valeur au nodata (modification des pixels de données de cette couleur en une autre).

![manageNodata](../docs/images/ROK4GENERATION/tools/manageNodata.png)

[Détails](./tools/manageNodata/README.md)

### Sous réechantillonnage de 4 images

Outil : `merge4tiff`

Cet outil génère une image à partir 4 images de même dimension disposées en carré, en moyennant les pixels 4 par 4. L'image en sortie a les dimensions des images en entrée. Il est possible de préciser une valeur de gamma pour exagérer les contrastes. Cet outil est utilisé pour générer une dalle d'un niveau à partir du niveau inférieur dans le cas d'une pyramide utilisant un TileMatrixSet de type Quad Tree.

![merge4tiff](../docs/images/ROK4GENERATION/tools/merge4tiff.png)

[Détails](./tools/merge4tiff/README.md)

### Réechantillonnage et reprojection d'images

Outil : `mergeNtiff`

Cet outil génère une image, définie par son rectangle englobant, sa projection et la résolution pixel, à partir d'images géoréférencées. Ces dernières peuvent avoir des projections et des résolutions différentes. Si le nombre de canaux n'est pas le même entre les sources et la sortie, la conversion est fait à la volée. Cet outil est utilisé pour générer le niveau le mieux résolu dans une pyramide à partir des images en entrée.

![mergeNtiff](../docs/images/ROK4GENERATION/tools/mergeNtiff.png)

[Détails](./tools/mergeNtiff/README.md)

### Superposition d'images

Outil : `overlayNtiff`

Cet outil génère une image à partir de plusieurs images de même dimension par superposition. Le calcul est fait pixel par pixel à partir de ceux sources avec le choix du mode : par transparence, par multiplication, en tenant compte des masques associés... Cet outil est utilisé lors de générations JOINCACHE lorsque plusieurs dalles de différentes pyramides sont trouvées pour une même dalle en sortie.

![overlayNtiff](../docs/images/ROK4GENERATION/tools/overlayNtiff.png)

[Détails](./tools/overlayNtiff/README.md)

### Stockage final en dalle

Outil : `work2cache`

Cet outil génère une dalle au format ROK4 à partir d'une image au format de travail. Une dalle ROK4 est une image TIFF dont les données sont tuilées, et avec un en-tête de taille fixe (2048 octets).

![work2cache](../docs/images/ROK4GENERATION/tools/work2cache.png)

[Détails](./tools/work2cache/README.md)

## Manipulation vecteur

### Écriture d'une dalle vecteur

Outil : `pbf2cache`

Cet outil écrit une dalle à partir des tuiles PBF rangées par coordonnées (<dossier racine>/x/y.pbf). La dalle écrite est au format ROK4, c'est-à-dire un fichier TIFF, dont les données sont tuilées : le TIFF ne sert que de conteneurs pour regrouper les tuiles PBF. L'en-tête est de taille fixe (2048 octets).

![pbf2cache](../docs/images/ROK4GENERATION/tools/pbf2cache.png)

[Détails](./tools/pbf2cache/README.md)
