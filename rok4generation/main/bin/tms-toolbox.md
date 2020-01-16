# 4ALAMO

[Vue générale](../../README.md#convertisseur-tms)

## Usage

### Commandes

* `tms-toolbox.pl --tms <FILE PATH> [--slabsize <INT>x<INT>] [--storage FILE[:<INT>]|CEPH|S3|SWIFT] [--level <STRING>] [--above <STRING>] [--ratio <INT>] --from <STRING> --to <STRING> [--add] [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--add` Si un fichier est en sortie, précise que l'on souhaite écrire à la suite du fichier s'il existe
* `--tms <file path>` TMS à utiliser
* `--level <string>` Niveau du TMS à considérer
* `--above <string>` Niveau du TMS jusqu'auquel travailler (uniquement pour les QTree)
* `--storage FILE[:<integer>]|CEPH|S3|SWIFT` Stockage, type et éventuellement profondeur d'arborescence. `FILE:2` par défaut.
* `--ratio <integer>` ratio à appliquer pour certaines conversion. `1` par défaut.
* `--slabsize <integer>x<integer>` Nombre de tuiles, en largeur et en hauteur, dans la dalle
* `--from <string>` Données en entrée du convertisseur
* `--to <string>` Données en sortie du convertisseur

## Entrées et sorties

| Types                              | Entrée | Sortie    | Description                                                                         |
| ---------------------------------- | ------ | --------- | ----------------------------------------------------------------------------------- |
| `BBOX:<XMIN>,<YMIN>,<XMAX>,<YMAX>` | x      |           | Rectangle englobant                                                                 |
| `BBOXES_LIST:<FILE PATH>`          | x      |           | Fichier listant des rectangle englobants `XMIN,YMIN,XMAX,YMAX`                      |
| `GEOM_FILE:<FILE PATH>`            | x      | x         | Fichier contenant une géométrie en GML, KML, JSON ou WKT (filtrage sur l'extention) |
| `GETMAP_PARAMS_LIST:<FILE PATH>`   |        | x (--add) | Fichier listant les paramètres d'un GetMap WMS WIDTH, HEIGHT, CRS et BBOX           |
| `GETTILE_PARAMS_LIST:<FILE PATH>`  |        | x (--add) | Fichier listant les paramètres d'un GetTile WMTS TILEMATRIX, TILECOL et TILEROW     |
| `POINT:<X>,<Y>`                    | x      |           | Coordonnées d'un point                                                              |
| `PYRAMID_LIST:<FILE PATH>`         | x      |           | Fichier liste d'une pyramide                                                        |
| `SLAB_INDICES:<COL>,<ROW>`         | x      |           | Indice de dalle, colonne et ligne                                                   |
| `SLAB_INDICES_LIST:<FILE PATH>`    | x      | x (--add) | Fichier listant des indices de dalles `COL,ROW`                                     |
| `SLAB_INFO`                        |        | x         | Indices et informations de stockage d'une dalle, pour un ou plusieurs niveaux       |
| `SLAB_PATH:<STORAGE NAME>`         | x      |           | Nom de stockage de la dalle, fichier pou objet, contenant au moins les indices      |
| `SLAB_PATHS_LIST:<FILE PATH>`      | x      | x (--add) | Fichier listant des suffixes de stockage de dalles                                  |
| `SLABS_COUNT`                      |        | x         | Nombre de dalles                                                                    |
| `SQL_FILE:<FILE PATH>`             |        | x (--add) | Fichier SQL d'insertion de dalles (level, col, row, geom) dans la table slabs       |
| `TFW_FILE:<FILE PATH>`             |        | x         | Fichier TFW de géoréférencement                                                     |
| `TILE_INDICES:<COL>,<ROW>`         | x      |           | Indice de tuile, colonne et ligne                                                   |
| `TILE_INFO`                        |        | x         | Indices d'une tuile, pour un ou plusieurs niveaux                                   |

Pour les paramètres avec <FILE PATH>, le fichier doit exister si en entrée, et ne doit pas exister si en sortie sans option `--add`.

## Conversions possibles

| Entrée            | Sortie              | Options obligatoires | Options facultatives |
| ----------------- | ------------------- | -------------------- | -------------------- |
| BBOX              | GETTILE_PARAMS_LIST | level, slabsize      |                      |
| BBOX              | SLAB_INDICES_LIST   | level, slabsize      |                      |
| BBOX              | SQL_FILE            | level, slabsize      |                      |
| BBOXES_LIST       | SLAB_INDICES_LIST   | level, slabsize      |                      |
| GEOM_FILE         | GETTILE_PARAMS_LIST | level, slabsize      |                      |
| GEOM_FILE         | SLAB_INDICES_LIST   | level, slabsize      |                      |
| GEOM_FILE         | SLABS_COUNT         | level, slabsize      |                      |
| GEOM_FILE         | SQL_FILE            | level, slabsize      |                      |
| POINT             | SLAB_INFO           | slabsize             | level, storage       |
| POINT             | TILE_INFO           |                      | level                |
| PYRAMID_LIST      | GEOM_FILE           | level, slabsize      |                      |
| PYRAMID_LIST      | GETTILE_PARAMS_LIST | slabsize             | ratio                |
| SLAB_INDICES      | TFW_FILE            | level, slabsize      |                      |
| SLAB_INDICES_LIST | GETMAP_PARAMS_LIST  | level, slabsize      | ratio                |
| SLAB_INDICES_LIST | SLAB_PATH_LIST      | level, slabsize      | above                |
| SLAB_PATH         | GEOM_FILE           | level, slabsize      | storage              |
| TILE_INDICES      | SLAB_INFO           | level, slabsize      | storage              |
