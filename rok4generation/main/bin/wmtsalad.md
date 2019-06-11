# WMTSALAD

[Vue générale](../../README.md#génération-de-pyramide-à-la-demande)

## Usage

### Commandes

* `wmtsalad.pl --conf /home/IGN/configuration.txt --dsrc /home/IGN/datasources.txt [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--conf <file path>` Execute l'outil en prenant en compte ce fichier de configuration principal
* `--dsrc <file path>` Execute l'outil en prenant en compte ce fichier de configuration des sources de données

## La configuration principale

La configuration principale est au format INI :
```
[ section ]
parameter = value
```

### Section `logger`

#### Paramètres

| Paramètre | Description | Obligatoire ou valeur par défaut |
| --------- | ----------- | -------------------------------- |
| log_path  | Dossier dans lequel écrire les logs. Les logs ne sont pas écrits dans un fichier si ce paramètre n'est pas fourni.                                                                                   |                                  |
| log_file  | Fichier dans lequel écrire les logs, en plus de la sortie standard. Les logs ne sont pas écrits dans un fichier si ce paramètre n'est pas fourni. Le fichier écrit sera donc `<log_path>/<log_file>` |                                  |
| log_level | Niveau de log : DEBUG - INFO - WARN - ERROR - ALWAYS                                                                                                                                                 | `WARN`                             |


#### Exemple
```
[ logger ]
log_path = /var/log
log_file = wmtsalad_2019-06-06.txt
log_level = INFO
```

### Section `pyramid`

#### Paramètres

| Paramètre       | Description                                                                                       | Obligatoire ou valeur par défaut |
| --------------- | ------------------------------------------------------------------------------------------------- | -------------------------------- |
| pyr_name        | Nom de la nouvelle pyramide                                                                       | obligatoire                      |
| pyr_desc_path   | Dossier dans le quel écrire le descripteur de pyramide                                            | obligatoire                      |
| tms_name        | Nom du Tile Matrix Set de la pyramide, avec l'extension `.tms`                                    | obligatoire si pas d'ancêtre     |
| tms_path        | Dossier contenant le TMS                                                                          | obligatoire                      |
| compression     | Compression des données dans les tuiles                                                           | `raw`                            |
| color           | Valeur de nodata, une valeur par canal, cohérent avec son format                                  |                                  |
| bitspersample   | Nombre de bits par canal, `8` ou `32`                                                             |                                  |
| sampleformat    | Format des canaux, `uint` ou `float`                                                              |                                  |
| samplesperpixel | Nombre de canaux : entre 1 et 4                                                                   |                                  |
| photometric     | Interprétation des canaux : `gray` ou `rgb`                                                       | `rgb`                            |
| interpolation   | Interpolation de réechantillonnage utilisée par mergeNtiff : `nn`, `linear`, `bicubic`, `lanczos` | `bicubic`                        |
| persistent      | Veut-on stocker les dalles lors de la consultation                                                | `false`                          |

Si on souhaite avoir de la persistence :

| Paramètre     | Description                                                                 | Obligatoire ou valeur par défaut |
| ------------- | --------------------------------------------------------------------------- | -------------------------------- |
| pyr_data_path | Dossier dans le quel écrire le descripteur de pyramide                      | obligatoire                      |
| dir_depth     | Nombre de sous-dossiers utilisé dans l'arborescence pour stocker les dalles | obligatoire                      |
| image_width   | Nombre de tuiles dans une dalle dans le sens de la largeur                  | obligatoire                      |
| image_height  | Nombre de tuiles dans une dalle dans le sens de la hauteur                  | obligatoire                      |


#### Exemple

```
[ pyramid ]

pyr_name = PYR-OD
pyr_desc_path = /home/IGN/descriptors

tms_name = PM.tms
tms_path = /home/IGN/TMS

persistent = true
pyr_data_path = /home/IGN/data
image_width = 16
image_height = 16
dir_depth = 2

compression = jpg
bitspersample = 8
sampleformat = int
samplesperpixel = 3
photometric = rgb
interpolation = bicubic

color = 255,255,255
```

## La configuration des sources de données

La configuration principale est à un format INI-like, avec la possibilité de faire des sous sections :
```
[ section ]
parameter = value
[ sous-section ]
parameter = value
```

### Paramètres

Chaque section définit un ensemble de niveaux contigus, utilisant les même sources. Le nom de la section n'a aucune importance (les niveaux llimites sont définis en paramètres dans la section). Il ne doit pas y avoir de niveau de recouvrement entre les sections, mais il peut y avoir des trous. Les sources sont définies dans des sous-sections, dont le nom est l'ordre d'importance en commençant par 1. La source 1 sera au dessus lors de la représentation. Les sous-sections doivent aller de 1 au nombre de sources, sans sauter d'entier.

#### Commun


| Paramètre | Description                                                                       | Obligatoire ou valeur par défaut |
| --------- | --------------------------------------------------------------------------------- | -------------------------------- |
| lv_top    | Identifiant du niveau du haut, jusqu'auquel utiliser les sources qui suivent      | obligatoire                      |
| lv_bottom | Identifiant du niveau du bas, à partir duquel utiliser les sources qui suivent    | obligatoire                      |
| extent    | Rectangle englobant (de la forme `xmin,ymin,xmax,ymax`) de définition des niveaux |                                  |


#### Source WMS


| Paramètre     | Description                                                                      | Obligatoire ou valeur par défaut |
| ------------- | -------------------------------------------------------------------------------- | -------------------------------- |
| wms_url       | URL du service WMS à utiliser                                                    | obligatoire                      |
| wms_version   | Version du service WMS : 1.1.1 ou 1.3.0                                          | obligatoire                      |
| wms_layers    | Liste des couches à requêter, séparées par des virgules                          | obligatoire                      |
| wms_styles    | Liste des styles associés aux couches, dans le même ordre                        | obligatoire                      |
| wms_nodata    | Couleur de fond en héxadécimal (exemple : 0xFFFFFF)                              | obligatoire                      |
| wms_channels  | Nombre de canaux des images moissonnées                                          | obligatoire                      |
| wms_extent    | Rectangle englobant limite pour le moissonnage (de la forme xmin,ymin,xmax,ymax) | obligatoire                      |
| wms_crs       | Système de coordonées à utiliser pour les requêtes                               |                                  |
| wms_timeout   |                                                                                  |                                  |
| wms_retry     |                                                                                  |                                  |
| wms_interval  |                                                                                  |                                  |
| wms_proxy     |                                                                                  |                                  |
| wms_user      |                                                                                  |                                  |
| wms_password  |                                                                                  |                                  |
| wms_referer   |                                                                                  |                                  |
| wms_userAgent |                                                                                  |                                  |
| wms_format    | Format de moissonnage des images : image/png ou image/jpeg                       |                                  |
| wms_option    | Liste d'option à ajouter aux requêtes                                            |                                  |


#### Source Pyramide

| Paramètre   | Description                                         | Obligatoire ou valeur par défaut |
| ----------- | --------------------------------------------------- | -------------------------------- |
| file        | Chemin vers le descripteur de la pyramide sources   | obligatoire                      |
| style       | Style à appliquer aux données à la diffusion        |                                  |
| transparent | Précise si le nodata est afficher avec transparence | `FALSE`                          |

### Exemples

```
[ level_range_19_11 ]
lv_bottom = 19
lv_top = 11
extent = -770850,4929770,1279780,6783830

[[ 1 ]]
file = /home/IGN/source1.pyr
style = style1
transparent = false

[[ 2 ]]
wms_url             =  http://target.server.net/wms
wms_timeout         =  60
wms_retry           =  10
wms_version         =  1.3.0
wms_layers          =  LAYER_1,LAYER_2,LAYER_3
wms_styles          =  STYLE_FOR_LAYER_1,STYLE_FOR_LAYER_2,STYLE_FOR_LAYER_3
wms_format          =  image/png
wms_crs             =  EPSG:2154
wms_extent          =  634500,6855000,636800,6857700
wms_channels        =  3
wms_nodata          =  0xFFA2FA

[ level_range_9_6 ]
lv_bottom = 9
lv_top = 6
extent = -770850,4929770,1279780,6783830

[[ 1 ]]
file = /home/IGN/source2.pyr
```

## Résumé des fichiers et dossiers manipulés

Avec les configurations mises en exemple (pas le service WMS) :
* La configuration principale `/home/IGN/configuration.txt`
* La configuration de la source de données `/home/IGN/datasources.txt`
* Le TMS `/home/IGN/TMS/PM.tms`
* Le fichier de logs `/var/log/wmtsalad_2019-06-06.txt`
* Le descripteur de pyramide en sortie `/home/IGN/descriptors/PYR-OD.pyr`
* Les descripteurs des pyramides sources utilisées par la pyramide à la demande `/home/IGN/source1.pyr` et `/home/IGN/source2.pyr`
* Dans le cas de la persistence : le dossier contenant les données de la pyramide `/home/IGN/data/PYR-OD/`
