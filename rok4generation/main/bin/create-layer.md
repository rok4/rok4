# CREATE-LAYER

[Vue générale](../../README.md#création-dun-descripteur-de-couche)

## Usage

### Commandes

* `create-layer.pl --pyr /home/IGN/PYRAMID.pyr --tmsdir /home/IGN/tilematrixsets [--layerdir /home/IGN/layers] [--style /home/IGN/normal.stl] [--resampling nn|linear|bicubic|lanczos_2|lanczos_3|lanczos_4] [--help|--usage|--version]`

### Options

* `--help` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--usage` Affiche le lien vers la documentation utilisateur de l'outil et quitte
* `--version` Affiche la version de l'outil et quitte
* `--pyr <file path>` Chemin vers le descripteur de la pyramide que la couche doit utiliser
* `--tmsdir <directory path>` Dossier contenant les TMS, surtout celui utilisé par la pyramide en entrée
* `--layerdir <directory path>` Chemin vers le dossier où écrire le descripteur de couche. Il est écrit à l'endroit où est lancé la commande si cette option n'est pas précisée.

