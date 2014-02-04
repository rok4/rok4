
mai 2012

---------------------------------------------------------------------------------------------
overlayNtiff -transparent 255,255,255 -opaque 0,0,0 -channels 4 -input source1.tif source2.tif source3.tif -output result.tif
---------------------------------------------------------------------------------------------

Ce programme est destine a etre utilise dans la chaine de generation de cache joinCache.
Il est appele pour calculer les dalles avec plusieurs sources.

Parametres d'entree :
1. Une couleur qui sera considérée comme transparente
2. Une couleur qui sera utilisée si on veut supprimer le canal alpha
3. Le nombre de canaux de l'image de sortie
4. Une liste d'images source à superposer
5. Un fichier de sortie

En sortie, un fichier TIFF au format dit de travail brut non compressé entrelace sur 1,3 ou 4 canaux entiers.

Toutes les images ont la même taille et sont sur 4 canaux entiers.
Le canal alpha est considéré comme l'opacité (0 = transparent)



