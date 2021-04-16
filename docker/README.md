# Dockerisation du projet ROK4

Disponible sur [Docker Hub](https://hub.docker.com/u/rok4)

Dockerfiles et images de sortie :

* `docker/rok4server/buster.Dockerfile` -> `rok4/rok4server:<VERSION>-buster`
* `docker/rok4server/stretch.Dockerfile` -> `rok4/rok4server:<VERSION>-stretch`
* `docker/rok4server/centos7.Dockerfile` -> `rok4/rok4server:<VERSION>-centos7`
* `docker/rok4generation/buster.Dockerfile` -> `rok4/rok4generation:<VERSION>-buster`

Utilisation du script `build.sh` :
```
./build.sh [--rok4server] [--rok4generation] --os buster|stretch|centos7 [--proxy http://proxy.host:port]
```

[Utilisation de la partie serveur](./rok4server/README.md)
[Utilisation de la partie outils](./rok4generation/README.md)
