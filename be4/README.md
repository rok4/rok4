![Logo Be4](../docs/images/BE4/be4.png)

La suite d'outils BE4 permet de générer, mettre à jour, composer, supprimer ou extraire des pyramide d'images. Ces outils sont écrits en Perl. Le travail de génération peut nécessiter l'utilisation d'outils de traitement d'images (réechantillonnage, reprojection, décimation, composition) écrits en C++.

Afin de paralléliser le travail de génération, certains outils Perl vont avoir pour fonction d'identifier le travail à faire, de le partager équitablement et d'écrire des scripts Shell. C'est alors l'exécution de ces scripts Shell qui vont calculer et écrire les dalles de la pyramide.

### Outils Perl

| Outil                | Description                                                                                                                                                                             | Parallélisable ? | Interface graphique |
| -------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------- | ------------------- |
| be4-file.pl          | Génère une pyramide à partir :<ul><li>d'images géoréférencées ou d'un serveur WMS</li><li>éventuellement d'un ancêtre à mettre à jour</li></ul> et la stocke dans un système de fichier | oui              | non                 |
| be4-ceph.pl          | Génère une pyramide à partir :<ul><li>d'images géoréférencées ou d'un serveur WMS</li><li>éventuellement d'un ancêtre à mettre à jour</li></ul> et la stocke dans un pool CEPH          | oui              | non                 |
| be4-s3.pl            | Génère une pyramide à partir :<ul><li>d'images géoréférencées ou d'un serveur WMS</li><li>éventuellement d'un ancêtre à mettre à jour</li></ul> et la stocke dans un bucket S3          | oui              | non                 |
| be4-swift.pl         | Génère une pyramide à partir :<ul><li>d'images géoréférencées ou d'un serveur WMS</li><li>éventuellement d'un ancêtre à mettre à jour</li></ul> et la stocke dans un container SWIFT    | oui              | non                 |
| joinCache-file.pl    | Génère une pyramide composée à partir de pyramides sources (convertit potentiellement les caractéristiques des images) et la stocke dans un système de fichier                          | oui              | non                 |
| joinCache-ceph.pl    | Génère une pyramide composée à partir de pyramides sources (convertit potentiellement les caractéristiques des images) et la stocke dans un pool CEPH                                   | oui              | non                 |
| joinCache-s3.pl      | Génère une pyramide composée à partir de pyramides sources (convertit potentiellement les caractéristiques des images) et la stocke dans un bucket S3                                   | oui              | non                 |
| sup-pyr.pl           | Supprime une pyramide. Utilise le fichier liste pour une pyramide stockée dans un système objet                                                                                         | non              | non                 |
| pyr2pyr.pl           | Convertit une pyramide stockée dans un système de fichier en un pyramide stockée dans un pool CEPH ou un bucket S3                                                                      | oui              | non                 |
| exPyr.pl             | Extraie une selon une zone géographique une partie d'une pyramide source                                                                                                                | non              | non                 |
| coord2image.pl       | Convertit des coordonnées terrain en chemin vers la dalle contenant ce point                                                                                                            | non              | non                 |
| tms-converter-gui.pl | Convertit des coordonnées, requêtes getTile en indices/chemin de dalles et tuiles et réciproquement                                                                                     | non              | oui                 |
| wmtSalaD.pl          | Génère une pyramide à la demande                                                                                                                                                        | non              | non                 |
| create-layer.pl      | Génère le descripteur de couche à partir du descripteur de pyramide                                                                                                                     | non              | non                 |
| be4-simulator.pl     | Renseigne le nombre de dalle que contiendrait une pyramide à partir de la géométrie de la couverture des données et de la taille de dalle voulue                                        | non              | non                 |


### Outils C++

| Outil         | Description                                                                                       | Utilisée par les outils BE4 |
| ------------- | ------------------------------------------------------------------------------------------------- | --------------------------- |
| cache2work    | Convertit une dalle de pyramide en une timage de travail (compression zip et non tuilée)          | be4-X, joinCache-X          |
| composeNtiff  | Convertit une mosaïque régulière d'image en une seule images                                      | be4-X                       |
| decimateNtiff | Génère une image à partir de plusieurs par décimation (on ne garde qu'un 1 pixel sur N)           | be4-X                       |
| manageNodata  | Génère un masque associé ou modifie les couleurs de donnée et nodata à partir d'une couleur cible |                             |
| merge4tiff    | Génère une image à partir de 4 (2x2) de même dimensions en moyennant les pixels 4 par 4           | be4-X                       |
| mergeNtiff    | Génère une image avec réechantillonnage ou reprojection à partir d'images géoréférencées          | be4-X                       |
| overlayNtiff  | Génère une image à partir de plusieurs par superposition (en tenant compte de la transparence)    | joinCache-X                 |
| slab2tiles    | Découpe une dalle de pyramide en ses N tuile                                                      | pyr2pyr                     |
| work2cache    | Convertit une image de travail en dalle de pyramide                                               | be4-X, joinCache-X          |

Plus de détails dans les dossiers des outils.
