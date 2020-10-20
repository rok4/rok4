# CACHE2WORK

[Vue générale](../../README.md#stockage-final-en-dalle)

![cache2work](../../../docs/images/ROK4GENERATION/tools/cache2work.png)

Cet outil lit une dalle ROK4 raster et la convertit en une image TIFF non tuilée, dont on peut choisir la compression.

## Usage

`cache2work -c <COMPRESSION> <INPUT FILE/OBJECT> <OUTPUT FILE> [-pool <POOL NAME>|-bucket <BUCKET NAME>|-container <CONTAINER NAME> [-ks] [-token <TOKEN_FILE_PATH>]] [-d]`

* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw, pkb
* `-pool <POOL NAME>` : précise le nom du pool CEPH dans lequel lire la dalle
* `-bucket <BUCKET NAME>` : précise le nom du bucket S3 dans lequel lire la dalle
* `-container <CONTAINER NAME>` : précise le nom du conteneur SWIFT dans lequel lire la dalle
* `-ks` : dans le cas d'un stockage dans SWIFT, précise si l'on souhaite avoir une authentification keystone
* `-token <TOKEN FILE PATH>` : précise un chemin fichier accessible en lecture et en écriture, censé contenir un jeton d'authentification swift ou keystone, selon la présence ou non du switch `-ks`.
    * Si le fichier n'est pas vide, son contenu sera initialement utilisé par l'outil pour s'authentifier sur Swift
    * Si le fichier n'existe pas, il sera créé vide
    * Dans tous les cas, le contenu sera mis à jour, en fin d'exécution de l'outil, avec le jeton valide le plus récent utilisé lors de l'exécution. (Pour le réutiliser lors de l'exécution suivante.)
* `-d` : activation des logs de niveau DEBUG


## Exemples

* `cache2work -c zip /home/IGN/slab.tif /home/IGN/workimage.tif`
* `cache2work -c zip -pool ign slab /home/IGN/workimage.tif`
