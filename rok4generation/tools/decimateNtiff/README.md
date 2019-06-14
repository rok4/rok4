# DECIMATENTIFF

[Vue générale](../../README.md#décimation-dune-image)

![decimateNtiff](../../../docs/images/ROK4GENERATION/tools/decimateNtiff.png)

Cet outil génère une image à partir de plusieurs image en phase entre elles (même résolution et même décalage) en ne gardant qu'un pixel sur N. Cet outil est utilisé pour générer une dalle d'un niveau à partir de dalles d'un niveau inférieur dans le cas d'une pyramide utilisant un TileMatrixSet "plus proche voisin" (une valeur de la donnée source n'est pas interpolée et se retrouve dans les niveaux supérieurs). Le centre d'un pixel de l'image de sortie doit être aligné avec un pixel d'une image en entrée. Le niveau de décimation (N) est déduit de la différence de résolution entre les entrées et la sortie.

Les informations sur les canaux (nombre, taille en bits et format) peuvent :
* être fournies et des conversions à la volée seront potentiellement faites sur les images n'ayant pas les mêmes
* ne pas être fournies, auquel cas toutes les images en entrée doivent avoir les même caractéristiques

## Usage

`decimateNtiff -c <COMPRESSION> <INPUT FILE> <OUTPUT FILE>  [-d]`

* `-f <FILE>` : fichier de configuration contenant l'image en sortie et la liste des images en entrée, avec leur géoréférencement et les masques éventuels
* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw, pkb
* `-n <COLOR>` : couleur de nodata, valeurs décimales pour chaque canal, séparées par des virgules (exemple : 255,255,255 pour du blanc sans transparence)
* `-a <FORMAT>` : format des canaux : float, uint
* `-b <INTEGER>` : nombre de bits pour un canal : 8, 32
* `-s <INTEGER>` : nombre de canaux : 1, 2, 3, 4
* `-d` : activation des logs de niveau DEBUG

Les options a, b et s doivent être toutes fournies ou aucune.

### Le fichier de configuration

Une ligne du fichier de configuration a la forme suivante : `IMG <CHEMIN> <XMIN> <YMAX> <XMAX> <YMIN> <RESX> <RESY>` pour une image de donnée ou `MSK <CHEMIN>` pour une image de masque.

Par exemple : `IMG /home/IGN/image.tif 10 150 110 50 0.5 0.5`. Si  on veut associer à cette image un masque, on mettra sur la ligne suivante `MSK /home/IGN/masque.tif`. Une ligne de masque doit toujours suivre une ligne d'image.

La première image listée sera la sortie (avec éventuellement son masque). Les suivantes sont les images en entrée. La première entrée peut être une image de fond, compatible avec celle de sortie et non avec les autres images en entrée (qui seront décimées). Seulement dans ce cas nous avons une entrée non compatible avec les autres.

Exemple de configuration :
```
IMG /home/IGN/IMAGE.tif -499    1501    1501    -499    2       2
MSK /home/IGN/MASK.tif
IMG /home/IGN/sources/imagefond.tif -499    1501    1501    -499    2       2
MSK /home/IGN/sources/maskfond.tif
IMG /home/IGN/sources/image1.tif 0       1000    1000    0       1       1
IMG /home/IGN/sources/image2.tif 500     1500    1500    500     1       1
MSK /home/IGN/sources/mask2.tif
```
L'image `/home/IGN/IMAGE.tif` sera écrite ainsi que son masque associé `/home/IGN/MASK.tif`

## Exemple

* `decimateNtiff -f conf.txt -c zip -n -99999`