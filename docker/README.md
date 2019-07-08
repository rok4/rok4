# Docker

## Compilation

Il faut lancer les commandes de build depuis le dossier racine du code.

### Image debian Buster

Nom de l'image de sortie : `rok4server-debian10`

#### Sans proxy

```
docker build \
    -f docker/images/debian10.rok4server.dockerfile \
    -t rok4server-debian10 \
    .
```

#### Avec proxy

```
docker build \
    -f docker/images/debian10.rok4server.dockerfile \
    -t rok4server-debian10 \
    --build-arg http_proxy=http://<PROXY HOST>:<PROXY PORT> \
    .
```

### Image centos 7

Nom de l'image de sortie : `rok4server-centos7`

#### Sans proxy

```
docker build \
    -f docker/images/centos7.rok4server.dockerfile \
    -t rok4server-centos7 \
    .
```

#### Avec proxy

```
docker build \
    -f docker/images/centos7.rok4server.dockerfile \
    -t rok4server-centos7 \
    --build-arg http_proxy=http://<PROXY HOST>:<PROXY PORT> \
    .
```
