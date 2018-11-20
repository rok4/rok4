![Logo ROK4SERVER](../docs/images/rok4server.png)

<!-- TOC START min:1 max:3 link:true update:true -->
    - [Utiliser ROK4SERVER via NGINX](#utiliser-rok4server-via-nginx)
        - [Configuration de ROK4SERVER](#configuration-de-rok4server)
        - [Lancement de ROK4SERVER en mode statique](#lancement-de-rok4server-en-mode-statique)
        - [Installation de NGINX](#installation-de-nginx)
        - [Configuration de NGINX](#configuration-de-nginx)

<!-- TOC END -->

## Utiliser ROK4SERVER via NGINX

ROK4SERVER est lancé en mode statique et NGINX interprète et redirige les requêtes sur le bon port.

### Configuration de ROK4SERVER

Dans le fichier `server.conf`, on précise le port d'écoute :

```xml
<serverPort>:9000</serverPort>
```

On configure les logs de manière à les retrouver dans un fichier par jour :

```xml
<logOutput>rolling_file</logOutput>
<logFilePrefix>/var/log/rok4</logFilePrefix>
<logFilePeriod>86400</logFilePeriod>
```

### Lancement de ROK4SERVER en mode statique

La ligne de commande permettant de lancer ROK4 comme instance autonome est la suivante :
```
rok4 -f /chemin/vers/fichier/server.conf &
```

Il se peut que votre instance autonome ROK4 se lance en français (cela dépend de votre configuration). Cela pose un problème lors de la lecture des nombres décimaux par exemple (confusion point et virgule). Il est donc conseillé de lancer la commande suivante :
```
LANG=en rok4 -f /chemin/vers/fichier/server.conf &
```

### Installation de NGINX

`apt install nginx` ou `yum install nginx`

### Configuration de NGINX

Remplacer le fichier `default` présent dans le répertoire `/etc/nginx/sites-enabled` par le contenu suivant :

```
upstream rok4 { server localhost:9000; }

server {
    listen 80;
    root /var/www;
    server_name localhost;

    access_log /var/log/rok4_access.log;
    error_log /var/log/rok4_error.log;

    location /wmts {
        fastcgi_pass rok4;
        include fastcgi_params;
    }
    location /wms {
        fastcgi_pass rok4;
        include fastcgi_params;
    }
}
```

On redémarre nginx : `systemctl restart nginx`

Le service WMS est disponible à l’adresse suivante : http://localhost/wms?request=GetCapabilities&service=WMS

Le service WMTS est disponible à l’adresse suivante : http://localhost/wmtsrequest=GetCapabilities&service=WMTS
