# WORK2CACHE

[Vue générale](../../README.md#stockage-final-en-dalle)

![work2cache](../../../docs/images/ROK4GENERATION/tools/work2cache.png)

Cette commande va produire une image TIFF, tuilée, avec une en-tête de taille fixe de 2048 octets afin que ROK4SERVER puisse ne pas la lire (toutes les informations lui sont déjà connues grâce au descripteur de pyramide).

Par défaut, on ne précise pas les caractéristique de l'image en sortie (nombre de canaux, format des canaux...) mais on peut préciser l'ensemble pour réaliser une conversion (passage en noir et blanc par exemple).

La taille de tuile précisée doit être cohérente avec la taille totale de la dalle (doit être un diviseur des dimensions totales).


## Usage

`work2cache -c <VAL> -t <VAL> <VAL> <INPUT FILE> <OUTPUT FILE/OBJECT> [-pool <POOL NAME>|-bucket <BUCKET NAME>|-container <CONTAINER NAME> [-ks]] [-a <VAL> -s <VAL> -b <VAL>] [-crop]`

* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw, pkb, png
* `-t <INTEGER> <INTEGER>` : taille pixel d'une tuile, enlargeur et hauteur. Doit être un diviseur de la largeur et de la hauteur de l'image en entrée
* `-pool <POOL NAME>` : précise le nom du pool CEPH dans lequel écrire la dalle
* `-bucket <BUCKET NAME>` : précise le nom du bucket S3 dans lequel écrire la dalle
* `-container <CONTAINER NAME>` : précise le nom du conteneur SWIFT dans lequel écrire la dalle
* `-ks` : dans le cas d'un stockage dans SWIFT, précise si l'on souhaite avoir une authentification keystone
* `-token <TOKEN FILE PATH>` : précise le chemin vers un fichier accessible en lecture et en écriture, censé contenir un jeton d'authentification swift ou keystone, selon la présence ou non du switch `-ks`.
    * Si le fichier n'est pas vide, son contenu sera initialement utilisé par l'outil pour s'authentifier sur Swift
    * Dans tous les cas, le contenu sera mis à jour, en fin d'exécution de l'outil, avec le jeton valide le plus récent utilisé lors de l'exécution. (Pour le réutiliser lors de l'exécution suivante.)
* `-a <FORMAT>` : format des canaux : float, uint
* `-b <INTEGER>` : nombre de bits pour un canal : 8, 32
* `-s <INTEGER>` : nombre de canaux : 1, 2, 3, 4
* `-crop` : dans le cas d'une compression des données en JPEG, un bloc (16x16 pixels, base d'application de la compression) qui contient un pixel blanc est complètement remplis de blanc
* `-d` : activation des logs de niveau DEBUG

Les options a, b et s doivent être toutes fournies ou aucune.

La compression PNG a la particularité de ne pas être un standard du TIFF. Une image dans ce format, propre à ROK4, contient des tuiles qui sont des images PNG indépendantes, avec les en-têtes PNG. Cela permet de renvoyer sans traitement une tuile au format PNG. Ce fonctionnement est calqué sur le format JPEG.

## Exemples

* Stockage fichier sans conversion : `work2cache input.tif -c png -t 256 256 output.tif`
* Stockage fichier avec conversion : `work2cache input.tif -c png -t 256 256 -a uint -b 8 -s 1 output.tif`
* Stockage CEPH sans conversion : `work2cache input.tif -pool PYRAMIDS -c png -t 256 256 output.tif`
* Stockage S3 sans conversion : `work2cache input.tif -bucket PYRAMIDS -c png -t 256 256 output.tif`
* Stockage SWIFT sans conversion : `work2cache input.tif -container PYRAMIDS -c png -t 256 256 output.tif`
* Stockage SWIFT sans conversion avec authentification keystone : `work2cache input.tif -container PYRAMIDS -ks -c png -t 256 256 output.tif`