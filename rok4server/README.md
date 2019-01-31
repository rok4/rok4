![Logo ROK4SERVER](../docs/images/rok4server.png)

<!-- TOC START min:1 max:3 link:true update:true -->
- [Utiliser ROK4SERVER via NGINX](#utiliser-rok4server-via-nginx)
    - [Configurer et lancer ROK4SERVER en mode statique](#configurer-et-lancer-rok4server-en-mode-statique)
    - [Installer et configurer NGINX](#installer-et-configurer-nginx)
- [Utiliser ROK4SERVER via APACHE](#utiliser-rok4server-via-apache)
- [Accès aux capacités du serveur](#accès-aux-capacités-du-serveur)

<!-- TOC END -->

# Utiliser ROK4SERVER via NGINX

ROK4SERVER est lancé en mode statique et NGINX interprète et redirige les requêtes sur le bon port.

## Configurer et lancer ROK4SERVER en mode statique

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

La ligne de commande permettant de lancer ROK4 comme instance autonome est la suivante :
```
rok4 -f /chemin/vers/fichier/server.conf &
```

Il se peut que votre instance autonome ROK4 se lance en français (cela dépend de votre configuration). Cela pose un problème lors de la lecture des nombres décimaux par exemple (confusion point et virgule). Il est donc conseillé de lancer la commande suivante :
```
LANG=en rok4 -f /chemin/vers/fichier/server.conf &
```

## Installer et configurer NGINX

* Sous Debian : `apt install nginx`
* Sous Centos : `yum install nginx`

Remplacer le fichier `default` présent dans le répertoire `/etc/nginx/sites-enabled` par le contenu suivant :

```
upstream rok4 { server localhost:9000; }

server {
    listen 80;
    root /var/www;
    server_name localhost;

    access_log /var/log/rok4_access.log;
    error_log /var/log/rok4_error.log;

    location /rok4 {
        fastcgi_pass rok4;
        include fastcgi_params;
    }
}
```

On redémarre nginx : `systemctl restart nginx`

# Utiliser ROK4SERVER via APACHE


# Accès aux capacités du serveur

L'URL "racine" de ROK4SERVER liste les différents services et getCapabilities : http://localhost/tms

On redémarre nginx : `systemctl restart nginx`
* WMS : http://localhost/rok4?request=GetCapabilities&service=WMS
* WMTS : http://localhost/rok4?request=GetCapabilities&service=WMTS
* TMS : http://localhost/rok4/1.0.0
