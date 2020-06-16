# Utilisation du serveur ROK4 conteneurisé

Serveur WMS, WMTS et TMS issu du projet [ROK4](https://github.com/rok4/rok4)

## Lancement rapide

```
docker run --publish 9000:9000 rok4/rok4server:<VERSION>-<OS>
```

## Configuration personnalisée

Vous pouvez voir les valeurs par défaut se retrouvant dans les deux fichiers de configuration du serveur ROK4 dans le fichier [defaults](./defaults).

Il est possible de surcharger chacune de ces valeurs de configuration via des variables d'environnement. Exemple :

`docker run --publish 9000:9000 -e SERVICE_TITLE='"Mon serveur ROK4"' rok4/rok4server:<VERSION>-<OS>`

Afin de définir des valeurs avec des espaces (comme dans l'exemple), il faut bien encapsuler la chaîne avec des des doubles quotes et des simples.

Il est aussi possible de définir toutes les variables d'environnement dans un fichier (une variable par ligne) et de faire l'appel suivant :

`docker run --publish 9000:9000 --env-file=custom_env rok4/rok4server:<VERSION>-<OS>`

## Lancement au sein d'une stack 

Afin de tester facilement le serveur, il est possible de lancer une stack comprennant :
* Un front NGINX, permettant l'interrogation du serveur en HTTP, avec une configuration minimale
* Un serveur ROK4
* Des jeux de données, disponible sous forme d'[images](https://hub.docker.com/r/rok4/dataset)

nginx.conf.template
```
upstream rok4server { server middle:9000; }
                                               
server {
    listen 80 default_server;

    location /${ROKSERVER_PREFIX} {
        fastcgi_pass rok4server;
        include fastcgi_params;
    }
}
```

docker-compose.yaml
```yaml
version: "3"
services:
  front:
    image: nginx
    ports:
      - "80:80"
    links:
      - middle
    environment:
      - ROKSERVER_PREFIX=data
    volumes:
      - ./nginx.conf.template:/etc/nginx/templates/default.conf.template

  middle:
    image: rok4/rok4server:<VERSION>-<OS>
    volumes:
      - volume-limadm:/pyramids/LIMADM
      - volume-alti:/pyramids/ALTI
      - volume-ortho:/pyramids/BDORTHO

  data-limadm:
    image: rok4/dataset:geofla-martinique
    volumes:
      - volume-limadm:/pyramids/LIMADM

  data-alti:
    image: rok4/dataset:bdalti-martinique
    volumes:
      - volume-alti:/pyramids/ALTI

  data-ortho:
    image: rok4/dataset:bdortho5m-martinique
    volumes:
      - volume-ortho:/pyramids/BDORTHO

volumes:
  volume-limadm:
  volume-alti:
  volume-ortho:
```

Avec ces configurations, les capacités des 3 services rendus (WMS, WMTS et TMS) sont disponibles aux URL :
* WMS : http://localhost/data?SERVICE=WMS&REQUEST=GetCapabilities&VERSION=1.3.0
* WMTS : http://localhost/data?SERVICE=WMTS&REQUEST=GetCapabilities&VERSION=1.0.0
* TMS : http://localhost/data/1.0.0