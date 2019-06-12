# cache2work

![cache2work](../../../docs/images/ROK4GENERATION/tools/cache2work.png)

Cet outil lit une dalle ROK4 raster et la convertit en une image TIFF non tuilée, dont on peut choisir la compression.

## Usage

`cache2work -c <COMPRESSION> <INPUT FILE/OBJECT> <OUTPUT FILE> [-pool <POOL NAME>|-bucket <BUCKET NAME>|-container <CONTAINER NAME> [-ks]] [-d]`

* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw
* `-d` : activation des logs de niveau DEBUG
* `-pool <POOL NAME>` : précise le nom du pool CEPH dans lequel écrire la dalle
* `-bucket <BUCKET NAME>` : précise le nom du bucket S3 dans lequel écrire la dalle
* `-container <CONTAINER NAME>` : précise le nom du conteneur SWIFT dans lequel écrire la dalle
* `-ks` : dans le cas d'un stockage dans SWIFT, précise si l'on souhaite avoir une authentification keystone

## Exemples

* `cache2work -c zip /home/IGN/slab.tif /home/IGN/workimage.tif`
* `cache2work -c zip -pool ign slab /home/IGN/workimage.tif`
