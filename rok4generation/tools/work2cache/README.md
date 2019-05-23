### Description globale

Cache2work permet de convertir une image de travail (fichier) au format final, utilisé par ROK4SERVER (fichier ou objet)

![Logo Be4](../../docs/images/BE4/work2cache.png)

### Description précise

Cette commande va produire une image TIFF, tuilée, avec une en-tête de taille fixe de 2048 octets afin que ROK4SERVER puisse ne pas la lire (toutes les informations lui sont déjà connues grâce au descripteur de pyramide).

Par défaut, on ne précise pas les caractéristique de l'image en sortie (nombre de canaux, format des canaux...) mais on peut préciser l'ensemble pour réaliser une conversion (passage en noir et blanc par exemple).

La taille de tuile précisée doit être cohérente avec la taille totale de la dalle (doit être un diviseur des dimensions totales).

### Utilisation

* Stockage fichier sans conversion : work2cache input.tif -c png -t 256 256 output.tif
* Stockage fichier avec conversion : work2cache input.tif -c png -t 256 256 -a uint -b 8 -s 1 output.tif
* Stockage CEPH sans conversion : work2cache input.tif -pool PYRAMIDS -c png -t 256 256 output.tif
* Stockage S3 sans conversion : work2cache input.tif -bucket PYRAMIDS -c png -t 256 256 output.tif
* Stockage SWIFT sans conversion : work2cache input.tif -container PYRAMIDS -c png -t 256 256 output.tif
* Stockage SWIFT sans conversion avec authentification keystone : work2cache input.tif -container PYRAMIDS -ks -c png -t 256 256 output.tif