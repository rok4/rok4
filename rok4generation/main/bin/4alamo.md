# 4ALAMO

[Vue générale](../../README.md#la-suite-4alamo)

## Usage

### Commandes

* `4alamo-file.pl --conf /home/IGN/conf.txt [--env /home/IGN/env.txt] [--help|--usage|--version]`
* `4alamo-ceph.pl --conf /home/IGN/conf.txt [--env /home/IGN/env.txt] [--help|--usage|--version]`

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
log_file = 4alamo_2019-02-01.txt
log_level = INFO
```

### Section `datasource`

#### Paramètres

| Paramètre     | Description                                                                      | Obligatoire ou valeur par défaut |
| ------------- | -------------------------------------------------------------------------------- | -------------------------------- |
| filepath_conf | Chemin vers le fichier de configuration de la source de données (au format JSON) | obligatoire                      |

#### Exemple
```
[ datasource ]
filepath_conf = /home/IGN/SOURCE/sources.json
```

### Section `pyramid`

#### Paramètres

| Paramètre     | Description                                                                   | Obligatoire ou valeur par défaut |
|---------------|-------------------------------------------------------------------------------|----------------------------------|
| pyr_name_new  | Nom de la nouvelle pyramide                                                   | obligatoire                      |
| pyr_desc_path | Dossier dans le quel écrire le descripteur de pyramide et la liste des dalles | obligatoire                      |
| image_width   | Nombre de tuiles dans une dalle dans le sens de la largeur                    | `16`                             |
| image_height  | Nombre de tuiles dans une dalle dans le sens de la hauteur                    | `16`                             |
| tms_name      | Nom du Tile Matrix Set de la pyramide, avec l'extension `.tms`                | obligatoire si pas d'ancêtre     |
| tms_path      | Dossier contenant le TMS                                                      | obligatoire                      |
| pyr_level_top | Niveau du haut de la pyramide, niveau haut du TMS utilisé si non fourni       |                                  |

##### Stockage de la pyramide

| Type de stockage | Paramètre               | Description                                                                 | Obligatoire ou valeur par défaut |
|------------------|-------------------------|-----------------------------------------------------------------------------|----------------------------------|
| FILE             | pyr_data_path           | Dossier racine de stockage des données de la pyramide                       | obligatoire                      |
| FILE             | dir_depth               | Nombre de sous-dossiers utilisé dans l'arborescence pour stocker les dalles | `2` si pas d'ancêtre             |
| CEPH             | pyr_data_pool_name      |                                                                             | obligatoire                      |

Dans le cas du stockage objet, certaines variables d'environnement doivent être définies sur les machines d'exécution :
* CEPH
    - ROK4_CEPH_CONFFILE
    - ROK4_CEPH_USERNAME
    - ROK4_CEPH_CLUSTERNAME

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

Seuls les TMS QuadTree PM et WGS84G sont gérés par tippecanoe, donc par 4alamo.

#### Exemple

```
[ pyramid ]

pyr_data_path = /home/IGN/PYRAMIDS
pyr_desc_path = /home/IGN/DESCRIPTOR
pyr_name_new = BDTOPO
pyr_level_top = 6

tms_name = PM.tms
tms_path = /home/IGN/TMS

dir_depth = 2
image_width = 16
image_height = 16
```

### Section `process`

#### Paramètres

| Paramètre        | Description                                                                                             | Obligatoire ou valeur par défaut |
|------------------|---------------------------------------------------------------------------------------------------------|----------------------------------|
| job_number       | Niveau de parallélisation de la génération de la pyramide.                                              | obligatoire                      |
| path_temp        | Dossier temporaire propre à chaque script. Un sous dossier au nom de la pyramide et du script sera créé | obligatoire                      |
| path_temp_common | Dossier temporaire commun à tous les scripts. Un sous dossier COMMON sera créé                          | obligatoire                      |
| path_shell       | Dossier où écrire les scripts                                                                           | obligatoire                      |

#### Exemple
```
[ process ]
path_temp = /tmp
path_temp_common = /mnt/share/
path_shell  = /home/IGN/SCRIPT/
job_number = 4
```

## La configuration des sources de données

Pour générer une pyramide vecteur, il faut renseigner pour chaque niveau de coupure (niveau pour lequel les sources de données sont différentes) le serveur PostgreSQL, les tables et attributs à utiliser ainsi que l'étendue sur laquelle les utiliser. Le fichier est au format JSON.

### Paramètres

* `Identifiant du niveau` : correpondant à ceux du TMS utilisé.
    - `extent` : étendue sur laquelle on veut calculer la pyramide, soit un rectangle englobant (de la forme `xmin,ymin,xmax,ymax`), soit le chemin vers un fichier contenant une géométrie WKT, GeoJSON ou GML
    - `srs` : projection de l'étendue fournie, ainsi que celle des données en base
    - `db`
        - `host` : hôte du serveur PostgreSQL contenant la base de données
        - `port` : port d'écoute du serveur PostgreSQL contenant la base de données. 5432 par défaut
        - `database` : nom de la base de données
        - `user` : utilisateur PostgreSQL
        - `password` : mot de passe de l'utilisateur PostgreSQL
    - `tables`
        - `schema` : nom du schéma contenant la table
        - `native_name` : nom en base de la table
        - `final_name` : nom dans les tuiles vecteur finale de la pyramide de cette table. Est égal au nom natif si non fourni.
        - `attributes` : attribut à exporter dans les tuiles vecteur de la pyramide. Une chaîne vide ou absent pour n'exporter que la géométrie, "\*" tous les exporter.
        - `filter` : filtre attributaire (optionnel)

### Exemple

```json
{
    "10":{
        "srs": "EPSG:3857",
        "extent": "/home/IGN/FXX.wkt",
        "db": {
            "host": "postgis.ign.fr",
            "port": "5433",
            "database": "geobase",
            "user": "ign",
            "password": "pwd"
        },
        "tables": [
            {
                "schema": "bdcarto",
                "native_name": "limites_administratives",
                "filter" : "genre = 'Limite de département'"
            }
        ]
    },
    "15":{
        "srs": "EPSG:3857",
        "extent": "/home/IGN/D008.wkt",
        "db": {
            "host": "postgis.ign.fr",
            "port": "5433",
            "database": "geobase",
            "user": "ign",
            "password": "pwd"
        },
        "tables": [
            {
                "schema": "bdtopo",
                "native_name": "limites_administratives"
            },
            {
                "schema": "bdtopo",
                "native_name": "routes",
                "final_name": "roads",
                "attributes": "nom",
                "filter": "importance = '10'"
            }
        ]
    },
    "18":{
        "srs": "EPSG:3857",
        "extent": "/home/IGN/D008.wkt",
        "db": {
            "host": "postgis.ign.fr",
            "port": "5433",
            "database": "geobase",
            "user": "ign",
            "password": "pwd"
        },
        "tables": [
            {
                "schema": "bdtopo",
                "native_name": "limites_administratives",
                "attributes": "nom"
            },
            {
                "schema": "bdtopo",
                "native_name": "routes",
                "final_name": "roads",
                "attributes": "*"
            }
        ]
    }
}
```

## Résumé des fichiers et dossiers manipulés

Avec les configurations mises en exemple :
* La configuration principale `/home/IGN/conf.txt`
* La configuration d'environnement `/home/IGN/env.txt`
* La configuration de la source de données `/home/ign/SOURCE/sources.json`
* Le TMS `/home/IGN/TMS/PM.tms`
* Le fichier de logs `/var/log/4alamo_2019-02-01.txt`
* Le descripteur de pyramide `/home/IGN/DESCRIPTOR/BDTOPO.pyr`
* La liste des dalles `/home/IGN/DESCRIPTOR/BDTOPO.list`.
* Les scripts :
    - `/home/IGN/SCRIPT/SCRIPT_1.sh`, `/home/IGN/SCRIPT/SCRIPT_2.sh`, `/home/IGN/SCRIPT/SCRIPT_3.sh`, `/home/IGN/SCRIPT/SCRIPT_4.sh`, exécutables en parallèle et sur des machines différentes. Les exécutables externes au projet ROK4 `tippecanoe` et `ogr2ogr` doivent être présents sur ces machines.
    - `/home/IGN/SCRIPT/SCRIPT_FINISHER.sh`, à exécuter quand tous les splits sont terminés en succès.
* Le dossier temporaire commun `/mnt/share/COMMON/`
* Les dossiers temporaires propres à chaque script `/tmp/BDTOPO/SCRIPT_1/`, `/tmp/BDTOPO/SCRIPT_2/`, `/tmp/BDTOPO/SCRIPT_3/`, `/tmp/BDTOPO/SCRIPT_4/`
* Dans le cas d'un stockage fichier : le dossier contenant les données de la pyramide `/home/IGN/PYRAMIDS/BDTOPO/`
