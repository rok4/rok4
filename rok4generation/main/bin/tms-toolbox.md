# 4ALAMO

[Vue générale](../../README.md#convertisseur-tms)

## Usage

### Commandes

* `tms-toolbox.pl --tms <FILE PATH> [--slabsize <INT>x<INT>] [--level <STRING>] --from <STRING> --to <STRING> [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--tms <file path>` TMS à utiliser
* `--slabsize <integer>x<integer>` Nombre de tuiles, en largeur et en hauteur, dans la dalle
* `--from <string>` Données en entrée du convertisseur
* `--to <string>` Données en sortie du convertisseur

## Entrées et sorties

Les possibilités sont :
* `SLAB_INDICES_LIST:<FILE PATH>` :
    - `<LEVEL>` est l'identifiant du niveau (issu du TMS) des dalles dont les indices sont listés
    - `<FILE PATH>` est le chemin vers la liste des dalles, où chaque ligne est de la forme `COL,ROW`. Doit exister si en entrée, ne doit pas exister si en sortie.
* `GETMAP_PARAMS:<FILE PATH>`
    - `<FILE PATH>` est le chemin vers la liste des paramètres GetMap, une ligne = un GetMap. Doit exister si en entrée, ne doit pas exister si en sortie.
* `GEOM_FILE:<FILE PATH>`
    - `<FILE PATH>` est le chemin vers le fichier contenant la géométrie en WKT, GeoJSON ou GML.
* `SQL_FILE:<FILE PATH>`
    - `<FILE PATH>` est le chemin vers le fichier SQL contenant l'insertion de ligne par COPY FROM STDIN.
* `POINT:<X>,<Y>` : coordonnées d'un point
* `SLAB_INFO` : uniquement en sortie, indices et chemin d'une dalle pour chaque niveau du TMS. Plusieurs possibilités
    - `SLAB_INFO` : stockage fichier, avec une profondeur d'arborescence de 2
    - `SLAB_INFO:<STORAGE TYPE>` : on précise le type de stockage (FILE, CEPH, S3 ou SWIFT). Si FILE, la profondeur est de 2
    - `SLAB_INFO:FILE:<DEPTH>` : on précise la profondeur d'arborescence pour le stockage fichier (nombre entier strictement positif)
* `TILE_INFO` : uniquement en sortie, indices de tuile pour chaque niveau du TMS

## Conversions possibles

| Entrée            | Sortie            | Description                                                                                                                                                               |
| ----------------- | ----------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| SLAB_INDICES_LIST | GETMAP_PARAMS     | Chaque indice de dalle est convertit en paramètres GetMap WIDTH, HEIGHT, CRS et BBOX                                                                                      |
| POINT             | SLAB_INFO         | Affiche les indices de la dalle contenant le point, ainsi que le chemin de la dalle fichier ou  le suffixe de la dalle objet, pour chaque niveau si pas de niveau précisé |
| POINT             | TILE_INFO         | Affiche les indices de la tuile contenant le point, pour chaque niveau si pas de niveau précisé                                                                           |
| GEOM_FILE         | SLAB_INDICES_LIST | Génère la liste des indices des dalles intersectant la géométrie fournie pour le niveau précisé                                                                           |
| GEOM_FILE         | SQL_FILE          | Génère le script SQL insérant une ligne par dalle intersectant la géométrie fournie pour le niveau fourni, sans la création de la table `slabs`                           |
