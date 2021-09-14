# CACHE2WORK

[Vue générale](../../README.md#stockage-final-en-dalle)

![cache2work](../../../docs/images/ROK4GENERATION/tools/cache2work.png)

Cet outil lit une dalle ROK4 raster et la convertit en une image TIFF non tuilée, dont on peut choisir la compression.

## Usage

`cache2work -c <COMPRESSION> <INPUT FILE/OBJECT> <OUTPUT FILE> [-pool <POOL NAME>|-bucket <BUCKET NAME>|-container <CONTAINER NAME>] [-d]`

* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, jpg90, raw (défaut), zip, lzw, pkb
* `-pool <POOL NAME>` : précise le nom du pool CEPH dans lequel lire la dalle
* `-bucket <BUCKET NAME>` : précise le nom du bucket S3 dans lequel lire la dalle
* `-container <CONTAINER NAME>` : précise le nom du conteneur SWIFT dans lequel lire la dalle
* `-d` : activation des logs de niveau DEBUG


## Exemples

* `cache2work -c zip /home/IGN/slab.tif /home/IGN/workimage.tif`
* `cache2work -c zip -pool ign slab /home/IGN/workimage.tif`
