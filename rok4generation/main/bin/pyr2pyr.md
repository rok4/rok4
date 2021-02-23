# PYR2PYR

[Vue générale](../../README.md#transfert-de-pyramide)

## Usage

### Commandes

* `pyr2pyr.pl --conf /home/IGN/conf.txt [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--conf <file path>` Execute l'outil en prenant en compte ce fichier de configuration

## La configuration principale

La configuration principale est au format INI :
```
[ section ]
parameter = value
```

### Section `logger`

#### Paramètres

| Paramètre   | Description                                                                                                                                                                                          | Obligatoire ou valeur par défaut |
| ----------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------- |
| `log_path`  | Dossier dans lequel écrire les logs. Les logs ne sont pas écrits dans un fichier si ce paramètre n'est pas fourni.                                                                                   |                                  |
| `log_file`  | Fichier dans lequel écrire les logs, en plus de la sortie standard. Les logs ne sont pas écrits dans un fichier si ce paramètre n'est pas fourni. Le fichier écrit sera donc `<log_path>/<log_file>` |                                  |
| `log_level` | Niveau de log : DEBUG - INFO - WARN - ERROR - ALWAYS                                                                                                                                                 | `WARN`                           |


#### Exemple
```
[ logger ]
log_path = /var/log
log_file = pyr2pyr_2019-02-01.txt
log_level = INFO
```

### Section `from`

#### Paramètres

| Paramètre       | Description                                                               | Obligatoire ou valeur par défaut |
| --------------- | ------------------------------------------------------------------------- | -------------------------------- |
| `pyr_desc_file` | Chemin vers le descripteur de la pyramide à compiler                      | obligatoire                      |
| `tms_path`      | Chemin vers le répertoire des Tile Matrix Set                             | obligatoire                      |
| `follow_links`  | Précise si l'on souhaite suivre les liens lors de la copie de la pyramide | `TRUE`                           |

#### Exemple
```
[ from ]
pyr_desc_file=/home/IGN/SOURCE_PYRAMID.pyr
follow_links=FALSE
tms_path=/home/IGN/TMS/
```

### Section `to`

#### Paramètres communs

| Paramètre       | Description                                                                   | Obligatoire ou valeur par défaut |
| --------------- | ----------------------------------------------------------------------------- | -------------------------------- |
| `pyr_name_new`  | Nom de la nouvelle pyramide                                                   | obligatoire                      |
| `pyr_desc_path` | Dossier dans le quel écrire le descripteur de pyramide et la liste des dalles | obligatoire                      |

#### Stockage de la nouvelle pyramide

| Type de stockage | Paramètre                 | Description                                                                 | Obligatoire ou valeur par défaut |
| ---------------- | ------------------------- | --------------------------------------------------------------------------- | -------------------------------- |
| FILE             | `pyr_data_path`           | Dossier racine de stockage des données de la pyramide                       | obligatoire                      |
| FILE             | `dir_depth`               | Nombre de sous-dossiers utilisé dans l'arborescence pour stocker les dalles | `2`                              |
| CEPH             | `pyr_data_pool_name`      |                                                                             | obligatoire                      |
| S3               | `pyr_data_bucket_name`    |                                                                             | obligatoire                      |
| SWIFT            | `pyr_data_container_name` |                                                                             | obligatoire                      |

Dans le cas du stockage objet, certaines variables d'environnement doivent être définies sur les machines d'exécution :
* CEPH
    - `ROK4_CEPH_CONFFILE`
    - `ROK4_CEPH_USERNAME`
    - `ROK4_CEPH_CLUSTERNAME`
* S3
    - `ROK4_S3_URL`
    - `ROK4_S3_KEY`
    - `ROK4_S3_SECRETKEY`
* SWIFT
    * Toujours
        - `ROK4_SWIFT_AUTHURL`
        - `ROK4_SWIFT_USER`
        - `ROK4_SWIFT_PASSWD`
        - `ROK4_SWIFT_PUBLICURL`
    * Si authentification native, sans Keystone
        - `ROK4_SWIFT_ACCOUNT`
    * Si authentification avec Keystone (présence de `ROK4_KEYSTONE_DOMAINID`)
        - `ROK4_KEYSTONE_DOMAINID`
        - `ROK4_KEYSTONE_PROJECTID`

#### Exemple (copie vers une pyramide fichier)

```
[ to ]

pyr_data_path = /home/IGN/PYRAMIDS
pyr_desc_path = /home/IGN/DESCRIPTOR
pyr_name_new = DESTINATION_PYRAMID
dir_depth = 3
```

### Section `process`

#### Paramètres

| Paramètre          | Description                                                                                             | Obligatoire ou valeur par défaut                                 |
|--------------------|---------------------------------------------------------------------------------------------------------|------------------------------------------------------------------|
| `job_number`       | Niveau de parallélisation de la génération de la pyramide.                                              | obligatoire                                                      |
| `path_temp`        | Dossier temporaire propre à chaque script. Un sous dossier au nom de la pyramide et du script sera créé | obligatoire                                                      |
| `path_temp_common` | Dossier temporaire commun à tous les scripts. Un sous dossier COMMON sera créé                          | obligatoire                                                      |
| `path_shell`       | Dossier où écrire les scripts                                                                           | obligatoire                                                      |


#### Exemple
```
[ process ]
path_temp = /tmp
path_temp_common = /mnt/share/
path_shell  = /home/IGN/SCRIPT/
job_number = 2
```

## Résumé des fichiers et dossiers manipulés

Avec les configurations mises en exemple (pas le service WMS) :
* La configuration `/home/IGN/conf.txt`
* Le descripteur de la pyramide à copier `/home/IGN/SOURCE_PYRAMID.pyr`
* La liste des dalles de la pyramide à copier `/home/IGN/SOURCE_PYRAMID.list`
* Le descripteur de la nouvelle pyramide `/home/IGN/DESTINATION_PYRAMID.pyr`
* La liste des dalles de la nouvelle pyramide `/home/IGN/DESTINATION_PYRAMID.list`
* Le fichier de logs `/var/log/pyr2pyr_2019-02-01.txt`
* Les scripts :
    - `/home/IGN/SCRIPT/SCRIPT_1.sh`, `/home/IGN/SCRIPT/SCRIPT_2.sh`, exécutables en parallèle et sur des machines différentes.
    - `/home/IGN/SCRIPT/SCRIPT_FINISHER.sh`, à exécuter quand tous les splits sont terminés en succès.
* Le dossier temporaire commun `/mnt/share/COMMON/`
* Les dossiers temporaires propres à chaque script, `/tmp/SCRIPT_1/` et `/tmp/SCRIPT_2/` et `/tmp/SCRIPT_FINISHER/`
* Dans le cas d'un stockage fichier : le dossier contenant les données de la nouvelle pyramide `/home/IGN/PYRAMIDS/DESTINATION_PYRAMID/`
