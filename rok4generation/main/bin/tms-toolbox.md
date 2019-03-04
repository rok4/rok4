# 4ALAMO

[Vue générale](../../README.md#convertisseur-tms)

## Usage

### Commandes

* `tms-toolbox.pl --tms <FILE PATH> [--slabsize <INT>x<INT>] [--storage FILE[:<INT>]|CEPH|S3|SWIFT] [--level <STRING>] [--ratio <INT>] --from <STRING> --to <STRING> [--add] [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--add` Si un fichier est en sortie, précise que l'on souhaite écrire à la suite du fichier s'il existe
* `--tms <file path>` TMS à utiliser
* `--level <string>` Niveau du TMS à considérer
* `--storage FILE[:<integer>]|CEPH|S3|SWIFT` Stockage, type et éventuellement profondeur d'arborescence. `FILE:2` par défaut.
* `--ratio <integer>` ratio à appliquer pour certaines conversion. `1` par défaut.
* `--slabsize <integer>x<integer>` Nombre de tuiles, en largeur et en hauteur, dans la dalle
* `--from <string>` Données en entrée du convertisseur
* `--to <string>` Données en sortie du convertisseur

## Entrées et sorties

| Types                              | Entrée | Sortie    | Description                                                                         |
| ---------------------------------- | ------ | --------- | ----------------------------------------------------------------------------------- |
| `PYRAMID_LIST:<FILE PATH>`         | x      |           | Fichier liste d'une pyramide                                                        |
| `SLAB_INDICES_LIST:<FILE PATH>`    | x      | x (--add) | Fichier listant des indices de dalles `COL,ROW`                                     |
| `GEOM_FILE:<FILE PATH>`            | x      | x         | Fichier contenant une géométrie en GML, KML, JSON ou WKT (filtrage sur l'extention) |
| `SQL_FILE:<FILE PATH>`             |        | x (--add) | Fichier SQL d'insertion de dalles (level, col, row, geom) dans la table slabs       |
| `TFW_FILE:<FILE PATH>`             |        | x         | Fichier TFW de géoréférencement                                                     |
| `POINT:<X>,<Y>`                    | x      |           | Coordonnées d'un point                                                              |
| `BBOX:<XMIN>,<YMIN>,<XMAX>,<YMAX>` | x      |           | Rectangle englobant                                                                 |
| `SLAB_INDICES:<COL>,<ROW>`         | x      |           | Indice de dalle, colonne et ligne                                                   |
| `TILE_INDICES:<COL>,<ROW>`         | x      |           | Indice de tuile, colonne et ligne                                                   |
| `SLAB_INFO`                        |        | x         | Indices et informations de stockage d'une dalle, pour un ou plusieurs niveaux       |
| `TILE_INFO`                        |        | x         | Indices d'une tuile, pour un ou plusieurs niveaux                                   |
| `GETMAP_PARAMS_LIST:<FILE PATH>`   |        | x (--add) | Fichier listant les paramètres d'un GetMap WMS WIDTH, HEIGHT, CRS et BBOX           |
| `GETTILE_PARAMS_LIST:<FILE PATH>`  |        | x (--add) | Fichier listant les paramètres d'un GetTile WMTS TILEMATRIX, TILECOL et TILEROW     |

Pour les paramètres avec <FILE PATH>, le fichier doit exister si en entrée, et ne doit pas exister si en sortie.

## Conversions possibles

| Entrée            | Sortie              | Options obligatoires | Options facultatives |
| ----------------- | ------------------- | -------------------- | -------------------- |
| SLAB_INDICES_LIST | GETMAP_PARAMS_LIST  | level, slabsize      | ratio                |
| PYRAMID_LIST      | GETTILE_PARAMS_LIST | level, slabsize      | ratio                |
| PYRAMID_LIST      | GEOM_FILE           | level, slabsize      |                      |
| GEOM_FILE         | SLAB_INDICES_LIST   | level, slabsize      |                      |
| GEOM_FILE         | SQL_FILE            |                      |                      |
| GEOM_FILE         | GETTILE_PARAMS_LIST |                      |                      |
| BBOX              | SLAB_INDICES_LIST   | level, slabsize      |                      |
| BBOX              | SQL_FILE            | level, slabsize      |                      |
| BBOX              | GETTILE_PARAMS_LIST | level, slabsize      |                      |
| SLAB_INDICES      | TFW_FILE            | level, slabsize      |                      |
| TILE_INDICES      | SLAB_INFO           | level, slabsize      | storage              |
| POINT             | SLAB_INFO           | slabsize             | level, storage       |
| POINT             | TILE_INFO           |                      | level                |
