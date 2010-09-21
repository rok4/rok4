L'image nodata doit etre au format TIFF du cache.
Utiliser tiff2tile pour la generer a partir d'un fichier TIFF.
Exemple:
tiff2til nodata.tif -c jpeg -p rgb -t 256 256 -b 8 nodata_tiled.tif
