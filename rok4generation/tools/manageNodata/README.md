# MANAGENODATA

[Vue générale](../../README.md#gestion-du-nodata)

![manageNodata](../../../docs/images/ROK4GENERATION/tools/manageNodata.png)

Cet outil permet d'identifier et de modifier une couleur dans une image considérée comme du nodata. Il permet également d'écrire le masque associé à l'image sur la base de cette valeur de nodata, et de réserver cette valeur au nodata (modification des pixels de données de cette couleur en une autre). Dans ce dernier cas, un pixel de nodata est un pixel de la couleur cible "relié au bord" (on proprage le nodata depuis les pixels de la couleur cible en bord d'image).

## Usage

`manageNodata -target <VAL> [-tolerance <VAL>] [-touch-edges] -format <VAL> [-nodata <VAL>] [-data <VAL>] <INPUT FILE> [<OUTPUT FILE>] [-mask-out <VAL>] [-d]`

* `-target <COLOR>` : couleur cible, permettant d'identifier le nodata
* `-tolerance <INTEGER>` : delta de tolérance autour de la couleur cible
* `-touche-edges` : précise que le nodata a la couleur cible et "est relié au bord".
* `-data <COLOR>` : nouvelle couleur pour la donnée, afin de réserver la couleur cible au nodata. N'a de sens qu'avec l'option -touch-edges
* `-nodata <COLOR>` : nouvelle couleur pour le nodata
* `-mask-out <FILE>` : chemin vers le masque à écrire, associé à l'image en entrée. Si aucun pixel de nodata n'est trouvé, le masque n'est pas écrit
* `-format <FORMAT>` : format des canaux : uint8, float32
* `-channels <INTEGER>` : nombre de canaux
* `-d` : activation des logs de niveau DEBUG

L'image en entrée n'est modifiée que si une nouvelle couleur de donnée ou de nodata différente de la couleur cible est précisée, et qu'aucune image en sortie n'est précisée.

## Exemples

* `manageNodata -target 255,255,255 -touch-edges -data 254,254,254 input_image.tif output_image.tif -channels 3 -format uint8`
* `manageNodata -target -99999 -tolerance 10 input_image.tif -mask-out mask.tif -channels 1 -format float32`
