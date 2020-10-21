# Utilisation du serveur ROK4 conteneurisé

Serveur WMS, WMTS et TMS issu du projet [ROK4](https://github.com/rok4/rok4)

## Lancement rapide

```
docker run --publish 9000:9000 rok4/rok4server:<VERSION>-<OS>
```

## Configuration personnalisée

Vous pouvez voir les valeurs par défaut se retrouvant dans les deux fichiers de configuration du serveur ROK4 dans le fichier [defaults](https://github.com/rok4/rok4/blob/master/docker/rok4server/defaults).

Il est possible de surcharger chacune de ces valeurs de configuration via des variables d'environnement. Exemple :

`docker run --publish 9000:9000 -e SERVICE_TITLE='"Mon serveur ROK4"' rok4/rok4server:<VERSION>-<OS>`

Afin de définir des valeurs avec des espaces (comme dans l'exemple), il faut bien encapsuler la chaîne avec des des doubles quotes et des simples.

Il est aussi possible de définir toutes les variables d'environnement dans un fichier (une variable par ligne) et de faire l'appel suivant :

`docker run --publish 9000:9000 --env-file=custom_env rok4/rok4server:<VERSION>-<OS>`

## Lancement au sein d'une stack

Afin de tester facilement le serveur, il est possible de lancer une stack comprennant :
* Un front NGINX, permettant l'interrogation du serveur rok4 en HTTP et une page "leaflet" configuré pour afficher une couche du géoportail ainsi qu'une couche gérée par le serveur Rok4 de la stack
* Un serveur ROK4
* Des jeux de données, disponible sous forme d'[images](https://hub.docker.com/r/rok4/dataset)

le fichier **docker-compose.yaml** du dépot permet de monter cette stack

```bash
docker-compose up
```

Avec les configurations de cette stack, les capacités des 3 services rendus (WMS, WMTS et TMS) sont disponibles aux URL :
* WMS : http://localhost/data?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.3.0
* WMTS : http://localhost/data?SERVICE=WMTS&REQUEST=GetCapabilities&VERSION=1.0.0
* TMS : http://localhost/data/1.0.0
et le leaflet d'exemple directement sur http://localhost/
Vous pouvez éditer le docker-compose si votre port 80 est déjà utilisé, pour le remplacer par le port 8888 par exemple :

```yml
...
 image: nginx
    ports:
      - "8888:80"
...
```
L'accès se fera alors via http://localhost:8888/
