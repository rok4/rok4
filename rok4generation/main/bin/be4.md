# 4ALAMO

[Vue générale](../../README.md#la-suite-be4)

## Usage

### Commandes

* `be4-file --conf /home/IGN/conf.txt [--env /home/IGN/env.txt] [--help|--usage|--version]`
* `be4-ceph --conf /home/IGN/conf.txt [--env /home/IGN/env.txt] [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--conf <file path>` Execute l'outil en prenant en compte ce fichier de configuration principal
* `--env <file path>` Execute l'outil en prenant en compte ce fichier d'environnement'

## La configuration principale

La configuration principale est au format INI :
```
[ section ]
parameter = value
```

Il est possible d'utiliser une configuration d'environnement, au même format, dont les valeurs seront surchargées par celles dans la configuration principale.

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
log_file = be4_2019-02-01.txt
log_level = INFO
```

### Section `datasource`

#### Paramètres

| Paramètre     | Description                                                                     | Obligatoire ou valeur par défaut |
|---------------|---------------------------------------------------------------------------------|----------------------------------|
| filepath_conf | Chemin vers le fichier de configuration de la source de données (au format INI) | obligatoire                      |

#### Exemple
```
[ datasource ]
filepath_conf = /home/IGN/SOURCE/sources.txt
```

### Section `pyramid`

#### Paramètres

| Paramètre         | Description                                                                                                                                | Obligatoire ou valeur par défaut |
|-------------------|--------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------|
| pyr_name_new      | Nom de la nouvelle pyramide                                                                                                                | obligatoire                      |
| pyr_desc_path     | Dossier dans le quel écrire le descripteur de pyramide et la liste des dalles                                                              | obligatoire                      |
| image_width       | Nombre de tuiles dans une dalle dans le sens de la largeur                                                                                 | `16`                             |
| image_height      | Nombre de tuiles dans une dalle dans le sens de la hauteur                                                                                 | `16`                             |
| tms_name          | Nom du Tile Matrix Set de la pyramide, avec l'extension `.tms`                                                                             | obligatoire si pas d'ancêtre     |
| tms_path          | Dossier contenant le TMS                                                                                                                   | obligatoire                      |
| pyr_level_top     | Niveau du haut de la pyramide, niveau haut du TMS utilisé si non fourni                                                                    |                                  |
| compression       | Compression des données dans les tuiles                                                                                                    | `raw`                            |
| compressionoption | Option complémentaire à la compression                                                                                                     | `none`                           |
| color             | Valeur de nodata, une valeur par canal, cohérent avec son format                                                                           |                                  |
| bitspersample     | Nombre de bits par canal, `8` ou `32`                                                                                                      |                                  |
| sampleformat      | Format des canaux, `uint` ou `float`                                                                                                       |                                  |
| samplesperpixel   | Nombre de canaux : entre 1 et 4                                                                                                            |                                  |
| photometric       | Interprétation des canaux : `gray` ou `rgb`                                                                                                | `rgb`                            |
| interpolation     | Interpolation de réechantillonnage utilisée par mergeNtiff : `nn`, `linear`, `bicubic`, `lanczos`                                          | `bicubic`                        |
| export_masks      | Précise si on souhaite écrire les masques dans la pyramide, force l'utilisation des masques pendant la génération si l'export est souhaité | `FALSE`                          |


Valeurs pour `compression` :
* `raw`, `jpg`, `zip`, `lzw`, `pkb` : ces compressions officielles du format TIFF permettront une visualisation de la dalle dans un logiciel d'image externe
* `png` : ce format propre à ROK4 ne sera pas lisible en dehors du projet.

Valeurs pour `compressionoption` :
* `none` : ne change rien
* `crop` : uniquement disponible pour la compression JPEG, crop permet de remplir de blanc les blocs (16x16 pixels) contenant un pixel blanc.

Valeurs pour `color` : les valeur pour chaque canal sont séparées par des virgules.
* pour des canaux entiers non signés sur 8 bits : une valeur entière de 0 à 255. Exemple : `255,255,255` pour une pyramide RGB
* pour des canaux flottants sur 32 bits : une valeur entière potentiellement négative. Exemple : `-99999` pour une pyramide MNT

`bitspersample`, `sampleformat`, `samplesperpixel` et `photometric` ne sont pas obligatoires. S'ils ne sont pas fournis, alors il doit petre possible d'extraire des sources de données image une et une seule valeur pour chacun de ces paramètres (pas uniquement des services WMS, et pas plusieurs valeurs si plusieurs sources d'images géoréférencées). S'ils sont tous fournis, les images téléchargées à l'aide d'un service WMS et les images géoréférencées seront potentiellement converties à la volée si leur format diffère. La conversion à la volée n'est possible qu'avec des canaux entiers non signés sur 8 bits.

##### Stockage de la pyramide

| Type de stockage | Paramètre               | Description                                                                 | Obligatoire ou valeur par défaut |
|------------------|-------------------------|-----------------------------------------------------------------------------|----------------------------------|
| FILE             | pyr_data_path           | Dossier racine de stockage des données de la pyramide                       | obligatoire                      |
| FILE             | dir_depth               | Nombre de sous-dossiers utilisé dans l'arborescence pour stocker les dalles | `2` si pas d'ancêtre             |
| CEPH             | pyr_data_pool_name      |                                                                             | obligatoire                      |
| S3               | pyr_data_bucket_name    |                                                                             | obligatoire                      |
| SWIFT            | pyr_data_container_name |                                                                             | obligatoire                      |

Dans le cas du stockage objet, certaines variables d'environnement doivent être définies sur les machines d'exécution :
* CEPH
    - ROK4_CEPH_CONFFILE
    - ROK4_CEPH_USERNAME
    - ROK4_CEPH_CLUSTERNAME
* S3
    - ROK4_S3_URL
    - ROK4_S3_KEY
    - ROK4_S3_SECRETKEY
* SWIFT
    - ROK4_SWIFT_AUTHURL
    - ROK4_SWIFT_ACCOUNT
    - ROK4_SWIFT_USER
    - ROK4_SWIFT_PASSWD

##### Cas d'une pyramide ancêtre

| Paramètre         | Description                                                                              | Obligatoire ou valeur par défaut          |
| ----------------- | ---------------------------------------------------------------------------------------- | ----------------------------------------- |
| pyr_name_old      | Nom de la pyramide à mettre à jour. La présence de ce paramètre implique une mise à jour |                                           |
| pyr_desc_path_old | Dossier contenant le descripteur et la liste des dalles de la pyramide à mettre à jour   | obligatoire si `pyr_name_old` est présent |
| update_mode       | Mode de mise à jour                                                                      | obligatoire si `pyr_name_old` est présent |

Valeurs pour `update_mode` :
* `slink` : une nouvelle pyramide est créée, et les dalles de la pyramide ancêtre sont référencées avec un lien symbolique ou un objet symbolique
* `hlink` : disponible pour le stockage fichier uniquement, une nouvelle pyramide est créée, et les dalles de la pyramide ancêtre sont référencées avec un lien physique
* `copy` : une nouvelle pyramide est créée, et les dalles de la pyramide ancêtre sont recopiée dans la nouvelle pyramide
* `inject` : il n'y a pas de nouvelle pyramide créée, et la pyramide ancêtre est modifiée


#### Exemple

```
[ pyramid ]

pyr_data_path = /home/IGN/PYRAMIDS
pyr_desc_path = /home/IGN/DESCRIPTOR
pyr_name_new = BDORTHOHR
pyr_level_top = 6

tms_name = PM.tms
tms_path = /home/IGN/TMS

dir_depth = 2
image_width = 16
image_height = 16

export_masks = TRUE

compression         = jpg
bitspersample       = 8
sampleformat        = uint
photometric         = rgb
samplesperpixel     = 3
interpolation       = bicubic

; red comme couleur de nodata
color               = 255,0,0
```

### Section `process`

#### Paramètres

| Paramètre        | Description                                                                                             | Obligatoire ou valeur par défaut                                 |
|------------------|---------------------------------------------------------------------------------------------------------|------------------------------------------------------------------|
| job_number       | Niveau de parallélisation de la génération de la pyramide.                                              | obligatoire                                                      |
| path_temp        | Dossier temporaire propre à chaque script. Un sous dossier au nom de la pyramide et du script sera créé | obligatoire                                                      |
| path_temp_common | Dossier temporaire commun à tous les scripts. Un sous dossier COMMON sera créé                          | obligatoire                                                      |
| path_shell       | Dossier où écrire les scripts                                                                           | obligatoire                                                      |
| use_masks        | Précise si on souhaite utilisé les masques associés aux données.                                        | `FALSE` si on ne souhaite pas exporter les masques, `TRUE` sinon |


#### Exemple
```
[ process ]
path_temp = /tmp
path_temp_common = /mnt/share/
path_shell  = /home/IGN/SCRIPT/
job_number = 2
use_masks = TRUE
```

## La configuration des sources de données

Pour générer une pyramide raster, il faut renseigner pour chaque niveau de coupure (niveau pour lequel les sources de données sont différentes ou lorsque l'on veut forcer un nouveau moissonnage) :
* Soit le dossier des images géoréférencées
* Soit le service WMS à interroger

### Paramètres

#### Images géoréférencées


| Paramètre          | Description                                                                                                    | Obligatoire ou valeur par défaut |
|--------------------|----------------------------------------------------------------------------------------------------------------|----------------------------------|
| srs                | Système de coordonnées des images géoréférencées                                                               | obligatoire                      |
| path_image         | Dossier contenant les images. Les sous-dossier seront parcourus                                                | obligatoire                      |
| preprocess_command | Permet d'appliquer un prétraitement à chaque image en entrée retenue                                           |                                  |
| preprocess_opt_beg | Définit les options à mettre avant le fichier en entrée à la commande de prétraitement                         |                                  |
| preprocess_opt_mid | Définit les options à mettre entre le fichier en entrée et le fichier en sortie à la commande de prétraitement |                                  |
| preprocess_opt_end | Définit les options à mettre après le fichier en sortie à la commande de prétraitement                         |                                  |
| preprocess_tmp_dir | Dossier dans lequel écrire les images prétraitées                                                              |                                  |

Les images géoréférencées retenues sont les fichiers :
* TIFF (extensions .tif, .TIF, .tiff and .TIFF)
* PNG (extensions .png, .PNG)
* JPEG2000 (extensions .jp2, .JP2)
* BIL (extensions .bil, .BIL, .zbil, .ZBIL)

À chaque image retenue, on regarde si un fichier avec le même nom mais l'extension `.msk` existe. Si oui, ce fichier est considéré comme étant le masque associé à l'image.

#### Service WMS

L'interrogation du service WMS se fait via des requêtes GetMap en HTTP.

| Paramètre               | Description                                                                                                                                                                     | Obligatoire ou valeur par défaut |
|-------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------|
| srs                     | Système de coordonnées de l'étendue éventuellement fournie                                                                                                                      | obligatoire                      |
| extent                  | Étendue sur laquelle on veut calculer la pyramide, soit un rectangle englobant (de la forme `xmin,ymin,xmax,ymax`), soit le chemin vers un fichier contenant une géométrie WKT. |                                  |
| list                    | Si aucune étendue n'est précisée, il est possible de fournir un fichier contenant la liste des dalles à générer, chaque ligne du fichier étant de la forme `COL,ROW`            |                                  |
| wms_layer               | Nom de la ou des couches à moissonner                                                                                                                                           | obligatoire                      |
| wms_url                 | URL du serveur WMS, avec le contexte, sans protocole                                                                                                                            | obligatoire                      |
| wms_version             |                                                                                                                                                                                 | obligatoire                      |
| wms_format              | Format des images téléchargées                                                                                                                                                  | obligatoire                      |
| wms_transparent         | Précise si on souhaite télécharger les images avec de la transparence                                                                                                           | `FALSE`                          |
| wms_bgcolor             | Couleur de fond à transmettre au service WMS                                                                                                                                    |                                  |
| max_width et max_height | Ces deux paramètres doivent être précisés ensemble ou pas du tout, et permettent de moissonner en plusieurs fois une dalle. Les valeurs doivent être des diviseurs de la largeur et de la hauteur de la dalle finale                                                      |                                  |
| min_size                | Permet de supprimer des images moissonnées dont la taille est inférieure ou égale à cette valeur (suppression d'image monochromes en PNG par exemple)                           |                                  |

Valeurs pour `wms_format` :
* `image/png`
* `image/tiff`
* `image/jpeg`
* `image/x-bil;bits=32`
* `image/tiff&format_options=compression:deflate`
* `image/tiff&format_options=compression:lzw`
* `image/tiff&format_options=compression:packbits`
* `image/tiff&format_options=compression:raw`

### Exemples

```
[ 17 ]
srs = IGNF:WGS84G
extent = /home/IGN/Polygon.txt

wms_layer   = LIMADM
wms_url     = http://wms.ign.fr/rok4
wms_version = 1.3.0
wms_format  = image/png
wms_style   = line
wms_transparent  = true
max_width = 2048
max_height = 2048
```

```
[ 19 ]
; Georeferenced images with preprocessing command
srs = IGNF:LAMB93
path_image = /home/IGN/BDORTHOHR/
```

## Résumé des fichiers et dossiers manipulés

Avec les configurations mises en exemple (pas le service WMS) :
* La configuration principale `/home/IGN/conf.txt`
* La configuration d'environnement `/home/IGN/env.txt`
* La configuration de la source de données `/home/ign/SOURCE/sources.txt`
* Le dossier contenant les images géoréférencées `/home/ign/HR/`
* Le TMS `/home/IGN/TMS/PM.tms`
* Le fichier de logs `/var/log/be4_2019-02-01.txt`
* Le descripteur de pyramide `/home/IGN/DESCRIPTOR/BDORTHOHR.pyr`
* La liste des dalles `/home/IGN/DESCRIPTOR/BDORTHOHR.list`.
* Les scripts : cela dépend du type de TMS utilisé
    - Si le TMS est un QuadTree
        - `/home/IGN/SCRIPT/SCRIPT_1.sh`, `/home/IGN/SCRIPT/SCRIPT_2.sh`, exécutables en parallèle et sur des machines différentes.
        - `/home/IGN/SCRIPT/SCRIPT_FINISHER.sh`, à exécuter quand tous les splits sont terminés en succès.
    - Si le TMS est un NNGraph
        - par niveau N de la pyramide : `/home/IGN/SCRIPT/LEVEL_N_SCRIPT_1.sh`, `/home/IGN/SCRIPT/LEVEL_N_SCRIPT_2.sh`, exécutables en parallèle et sur des machines différentes, niveau par niveau, en partant du bas.
        - `/home/IGN/SCRIPT/SCRIPT_FINISHER_1.sh` et `/home/IGN/SCRIPT/SCRIPT_FINISHER_2.sh`, à exécuter quand tous les splits par niveau sont terminés en succès.
        - `/home/IGN/SCRIPT/SCRIPT_FINISHER.sh`, à exécuter quand tous les scripts précédents sont terminés en succès.
* Le dossier temporaire commun `/mnt/share/COMMON/`
* Les dossiers temporaires propres à chaque script, `/tmp/BDORTHOHR/LEVEL_N_SCRIPT_1/` et `/tmp/BDORTHOHR/LEVEL_N_SCRIPT_2/` (pour chaque niveau N), `/tmp/BDORTHOHR/SCRIPT_FINISHER_1/`, `/tmp/BDORTHOHR/SCRIPT_FINISHER_2/` et `/tmp/BDORTHOHR/SCRIPT_FINISHER_2/`
* Dans le cas d'un stockage fichier : le dossier contenant les données de la pyramide `/home/IGN/PYRAMIDS/BDORTHOHR/`
