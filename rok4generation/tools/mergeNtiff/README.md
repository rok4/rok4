# MERGENTIFF

[Vue générale](../../README.md#réechantillonnage-et-reprojection-dimages)

![mergeNtiff](../../../docs/images/ROK4GENERATION/tools/mergeNtiff.png)

Cet outil génère une image, définie par son rectangle englobant, sa projection et la résolution pixel, à partir d'images géoréférencées. Ces dernières peuvent avoir des projections et des résolutions différentes, se recouvrir, ne pas recouvrir l'intégralité de l'image en sortie, avoir des caractéristiques différentes. Cet outil est utilisé pour générer le niveau le mieux résolu dans une pyramide à partir des images en entrée.

Les informations sur les canaux (nombre, taille en bits et format) peuvent :

* être fournies et des conversions à la volée seront potentiellement faites sur les images n'ayant pas les mêmes
* ne pas être fournies, auquel cas toutes les images en entrée doivent avoir les même caractéristiques

## Usage

`mergeNtiff -f <FILE> [-r <DIR>] -c <VAL> -i <VAL> -n <VAL> [-a <VAL> -s <VAL> -b <VAL>]`

* `-f <FILE>` : fichier de configuration contenant l'image en sortie et la liste des images en entrée, avec leur géoréférencement et les masques éventuels
* `-r <DIRECTORY>` : dossier racine à utiliser pour les images dont le chemin commence par un `?` dans le fichier de configuration. Le chemin du dossier doit finir par un `/`
* `-i <INTERPOLATION>` : interpolation à utiliser pour les reprojections et le réechantillonnage : nn (plus proche voisin), linear, bicubic, lanzos
* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw, pkb
* `-n <COLOR>` : couleur de nodata, valeurs décimales pour chaque canal, séparées par des virgules (exemple : 255,255,255 pour du blanc sans transparence)
* `-a <FORMAT>` : format des canaux : float, uint
* `-b <INTEGER>` : nombre de bits pour un canal : 8, 32
* `-s <INTEGER>` : nombre de canaux : 1, 2, 3, 4
* `-d` : activation des logs de niveau DEBUG

Les options a, b et s doivent être toutes fournies ou aucune.


### Le fichier de configuration

Une ligne du fichier de configuration a la forme suivante : `IMG <CHEMIN> <CRS> <XMIN> <YMAX> <XMAX> <YMIN> <RESX> <RESY>` pour une image de donnée ou `MSK <CHEMIN>` pour une image de masque.

Par exemple : `IMG /home/IGN/image.tif EPSG:1234 10 150 110 50 0.5 0.5`. Si  on veut associer à cette image un masque, on mettra sur la ligne suivante `MSK /home/IGN/masque.tif`. Une ligne de masque doit toujours suivre une ligne d'image. Si on souhaite préciser un chemin relatif à la racine passée via l'option `-r`, on mettra le chemin `MSK ?masque.tif`

La première image listée sera la sortie (avec éventuellement son masque). Les suivantes sont les images en entrée.

Exemple de configuration :
```
IMG /home/IGN/IMAGE.tif EPSG:1234 -499    1501    1501    -499    2       2
MSK /home/IGN/MASK.tif
IMG /home/IGN/sources/imagefond.tif EPSG:1234  -499    1501    1501    -499    2       2
MSK /home/IGN/sources/maskfond.tif
IMG /home/IGN/sources/image1.tif EPSG:4567  0       1000    1000    0       1       1
IMG /home/IGN/sources/image2.tif EPSG:4567  500     1500    1500    500     1       1
MSK /home/IGN/sources/mask2.tif
```

L'image `/home/IGN/IMAGE.tif` sera écrite ainsi que son masque associé `/home/IGN/MASK.tif`

## Exemples

* `mergeNtiff -f conf.txt -c zip -i bicubic -n 255,255,255`
* `mergeNtiff -f conf.txt -c zip -i nn -s 1 -b 32 -p gray -a float -n -99999`