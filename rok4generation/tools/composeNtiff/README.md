# COMPOSENTIFF

[Vue générale](../../README.md#fusion-dun-dallage-dimages)

![composeNtiff](../../../docs/images/ROK4GENERATION/tools/composeNtiff.png)

Cet outil fusionne plusieurs images disposées en un dallage régulier en une seule. Il est utilisé lorsqu'une image est moissonnée en plusieurs fois à cause de sa taille, pour recomposer l'image désirée. Toutes les images en entrée doivent voir les même dimensions et les même caractéristiques.

## Usage

`composeNtiff -s <DIRECTORY> -g <VAL> <VAL> -c <VAL> <OUTPUT FILE> [-d]`

* `-s` : dossier contenant toutes les images du quadrillage. Elles sont lues dans l'ordre alpha-numérique. Si il a plus d'images que nécessaires, les dernières ne sont pas utilisées
* `-c <COMPRESSION>` : compression des données dans l'image TIFF en sortie : jpg, raw (défaut), zip, lzw, pkb
* `-g <INTEGER> <INTEGER>` : largeur et hauteur de la grille en nombre d'images
* `-d` : activation des logs de niveau DEBUG

## Exemple

* `composeNtiff -c zip -s /home/IGN/tiles/ -g 4 4 /home/IGN/workimage.tif`