# MERGE4TIFF

[Vue générale](../../README.md#sous-réechantillonnage-de-4-images)

![merg4tiff](../../../docs/images/ROK4GENERATION/tools/merge4tiff.png)

Cet outil génère une image à partir 4 images de même dimension disposées en carré, en moyennant les pixels 4 par 4. L'image en sortie a les dimensions des images en entrée. Une image à utiliser comme fond peut être donnée. Il est possible de préciser une valeur de gamma pour exagérer les contrastes. Cet outil est utilisé pour générer une dalle d'un niveau à partir du niveau inférieur dans le cas d'une pyramide utilisant un TileMatrixSet de type Quad Tree.

Les informations sur les canaux (nombre, taille en bits et format) peuvent :

* être fournies et des conversions à la volée seront potentiellement faites sur les images n'ayant pas les mêmes
* ne pas être fournies, auquel cas toutes les images en entrée doivent avoir les même caractéristiques

## Usage

`merge4tiff [-g <VAL>] -n <VAL> [-c <VAL>] [-iX <FILE> [-mX<FILE>]] -io <FILE> [-mo <FILE>]`

* `-g <FLOAT>` : valeur de gamma permettant d'augmenter les contrastes (si inférieur à 1) ou de les réduire (si supérieur à 1)
* `-n <COLOR>` : couleur de nodata, valeurs décimales pour chaque canal, séparées par des virgules (exemple : 255,255,255 pour du blanc sans transparence)
* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw, pkb
* `-io <FILE>` : chemin de l'image de sortie
* `-mo <FILE>` : chemin du masque de sortie associé (optionnel)
* `-iX <FILE>` : chemin d'une image en entrée
    * X = [1..4] : position de l'image
```
image1 | image2
-------+-------
image3 | image4
```
    * X = b : image de fond
* `-mX <FILE>` : X = [1..4] ou b, masque associé à l'image en entrée
* `-a <FORMAT>` : format des canaux : float, uint
* `-b <INTEGER>` : nombre de bits pour un canal : 8, 32
* `-s <INTEGER>` : nombre de canaux : 1, 2, 3, 4
* `-d` : activation des logs de niveau DEBUG

Les options a, b et s doivent être toutes fournies ou aucune.

## Exemples

* `merge4tiff -g 1 -n 255,255,255 -c zip -ib backgroundImage.tif -i1 image1.tif -i3 image3.tif -io imageOut.tif`
* `merge4tiff -g 1 -n 255,255,255 -c zip -i1 image1.tif -m1 mask1.tif -i3 image3.tif -m3 mask3.tif -mo maskOut.tif  -io imageOut.tif`
