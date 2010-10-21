L'image nodata doit etre au format TIFF du cache.
Utiliser tiff2tile pour la generer a partir d'un fichier TIFF.
Exemples:
tiff2tile nodata.tif -c jpeg -p rgb -t 256 256 -b 8 nodata_tiled_jpeg.tif
tiff2tile nodata.tif -c none -p rgb -t 256 256 -b 8 nodata_tiled_raw.tif
tiff2tile nodata.tif -c png -p rgb -t 256 256 -b 8 nodata_tiled_png.tif
