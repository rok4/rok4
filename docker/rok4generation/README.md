# Utilisation des outils de génération ROK4 conteneurisé

Outils issus du projet [ROK4](https://github.com/rok4/rok4)


## Lancement d'une génération de pyramide raster

La configuration d'une génération de pyramide raster est détaillée [ici](https://github.com/rok4/rok4/tree/master/rok4generation).

La génération se fait en 2 temps. Tous les montages permettent la persistence des fichiers entre les deux étapes. La pyramide finale est écrite à la fin des deux étapes dans le dossier output avec les exemples suivant

### Exemple de configurations (source images RGB)

main.conf
```
[ datasource ]
filepath_conf = /datasources.conf

[ pyramid ]
pyr_data_path = /output
pyr_desc_path = /output
pyr_name_new = ma_pyramide

; informations sur le TMS
tms_name = PM.tms
tms_path = /etc/rok4/config/tileMatrixSet

; informations relatives aux images
compression = jpg

interpolation = bicubic

photometric = rgb
sampleformat = uint
bitspersample = 8
samplesperpixel = 3

image_width = 16
image_height = 16

; nodata
color = 255,255,255

[ process ]

job_number = 4
path_shell = /scripts
path_temp = /tmp
path_temp_common = /tmp
```

datasources.conf
```
[ 10 ]
srs = IGNF:LAMB93
path_image = /data/
```


### Génération des scripts

```
docker run --rm \
	-v $PWD/tmp:/tmp \
	-v $PWD/scripts:/scripts \
	-v $PWD/output:/output \
	-v $PWD/data/:/data:ro \
	-v $PWD/main.conf:/main.conf:ro \
	-v $PWD/datasources.conf:/datasources.conf:ro \
	rok4/rok4generation:<VERSION>-<OS> \
	be4-file.pl --conf /main.conf
```

### Exécution des scripts

```
docker run --rm \
	-v $PWD/tmp:/tmp \
	-v $PWD/scripts:/scripts \
	-v $PWD/output:/output \
	-v $PWD/data/:/data:ro \
	rok4/rok4generation:<VERSION>-<OS> \
	bash /main.sh
```