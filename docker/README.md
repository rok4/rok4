# Dockerisation du projet ROK4

Disponible sur [Docker Hub](https://hub.docker.com/u/rok4)

Dockerfiles et images de sortie :

* `docker/rok4server/debian10.Dockerfile` -> `rok4/rok4server:<VERSION>-debian10`
* `docker/rok4server/centos7.Dockerfile` -> `rok4/rok4server:<VERSION>-centos7`
* `docker/rok4generation/debian10.Dockerfile` -> `rok4/rok4generation:<VERSION>-debian10`

Utilisation du script `build.sh` :
```
./build.sh [--rok4server] [--rok4generation] --os debian10|centos7 [--proxy http://proxy.host:port]
```

* [Utilisation de la partie serveur](./rok4server/README.md)
* [Utilisation de la partie outils](./rok4generation/README.md)
