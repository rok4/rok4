![Logo ROK4SERVER](../docs/images/rok4server.png)

<!-- TOC START min:1 max:3 link:true update:true -->
- [Utiliser ROK4SERVER via NGINX](#utiliser-rok4server-via-nginx)
    - [Installer et configurer NGINX](#installer-et-configurer-nginx)
    - [Configurer et lancer ROK4SERVER en mode statique](#configurer-et-lancer-rok4server-en-mode-statique)
- [Utiliser ROK4SERVER via APACHE](#utiliser-rok4server-via-apache)
    - [Installer et configurer APACHE](#installer-et-configurer-apache)
        - [Mise en oeuvre avec mod_fcgid](#mise-en-oeuvre-avec-mod_fcgid)
        - [Mise en oeuvre avec mod_fastcgi](#mise-en-oeuvre-avec-mod_fastcgi)
- [Accès aux capacités du serveur](#accès-aux-capacités-du-serveur)
- [Fonctionnement général de ROK4SERVER](#fonctionnement-général-de-rok4server)
    - [Identification du service et du type de requête](#identification-du-service-et-du-type-de-requête)
    - [Accès aux données](#accès-aux-données)

<!-- TOC END -->

# Utiliser ROK4SERVER via NGINX

ROK4SERVER est lancé en mode statique et NGINX interprète et redirige les requêtes sur le bon port.

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

On redémarre nginx : `systemctl restart nginx`

# Utiliser ROK4SERVER via APACHE

## Installer et configurer APACHE

* Sous Debian : `apt install apache2`
* Sous Centos : `yum install apache2`

### Mise en oeuvre avec mod_fcgid

* Installation De Mod_fcgid
    - Sous Debian : `apt install libapache2-mod-fcgid`
    - Sous Centos : `yum install `
* Activation Du Mod_fcgid : `sudo a2enmod fcgid`
* Configuration Du VirtualHost : dans la configuration du VirtualHost Apache2  ajouter les lignes suivantes :

```
ScriptAlias / /opt/rok4/bin/

<Directory "/opt/rok4/bin/">
    SetHandler fcgid-script
    Options +ExecCGI
    AllowOverride None
    Order allow,deny
    Allow from all
</Directory>
```
* Lancer Apache : `sudo systemctl restart apache2`


### Mise en oeuvre avec mod_fastcgi

* Installation De Mod_fastcgi : suivre les instructions [ici](https://fastcgi-archives.github.io/mod_fastcgi/INSTALL.html)
* Le module mod_fastcgi.so devrait désormais être présent dans `/usr/lib/apache2/modules`. Dans `/etc/apache2/mods-enabled`, créer le fichier `mod_fastcgi.load` avec le contenu suivant :

```
LoadModule fastcgi_module /usr/lib/apache2/modules/mod_fastcgi.so
```
* Dans `/etc/apache2/mods-enabled` créer le fichier `mod_fastcgi.conf` avec le contenu suivant :

```
<IfModule mod_fastcgi.c>
    AddHandler fastcgi-script .fcgi
    FastCgiIpcDir /var/lib/apache2/fastcgi
</IfModule>
```
* Dans la configuration du VirtualHost Apache, ajouter les lignes suivantes

```
# En supposant que /opt/rok4/ soit le répertoire d'installation
ScriptAlias / /opt/rok4/bin/

<Directory "/opt/rok4/bin/">
    AllowOverride None
    Options None
    SetHandler fastcgi-script
    Order allow,deny
    Allow from all
</Directory>

FastCgiServer /opt/rok4/bin/rok4 -init-start-delay 5 -port 1998 -processes 2
```
* Lancer Apache : `sudo systemctl restart apache2`

# Accès aux capacités du serveur

L'URL "racine" de ROK4SERVER liste les différents services et getCapabilities : http://localhost/tms

On redémarre nginx : `systemctl restart nginx`
* WMS : http://localhost/rok4?request=GetCapabilities&service=WMS
* WMTS : http://localhost/rok4?request=GetCapabilities&service=WMTS
* TMS : http://localhost/rok4/1.0.0

# Fonctionnement général de ROK4SERVER

## Identification du service et du type de requête

Lorsque le serveur reçoit une requête, si c'est une requête POST, le corps est interprété pour extraire les informations. Seuls le getCapabilities, le getMap et le getTile en WMTS et WMS sont disponibles en POST.

Ensuite le service et le type de requête sont identifiés. Une requête sans paramètre sera considérée comme une requête TMS. Sinon le paramètre SERVICE est cherché et les valeurs gérées sont WMS et WMTS (le serveur est insensible à la casse).

Pour une requête TMS, la version "1.0.0" est cherchée dans le contexte :
* Si elle n'est pas trouvée, on retourne la liste des getCapabilities disponibles sur le serveur (getServices TMS)
* Si elle est trouvée, on regarde la profondeur du contexte à partir de cette version :
    - pas de profondeur supplémentaire (`.../1.0.0/`) : on demande les capacités pour le service TMS (getCapabilities TMS)
    - une profondeur supplémentaire (`.../1.0.0/<chaîne>/`) : on demande les informations sur la couche, si elle existe (getLayer TMS)
    - deux profondeurs supplémentaires (`.../1.0.0/<chaîne>/metadata.json`) : on demande les informations détaillées sur la couche, si elle existe (getMetadata TMS). Surtout utile dans le cas de données vecteur, cela permet de connaître les tables et attributs dans les tuiles.
    - quatre profondeurs supplémentaires (`.../1.0.0/<chaîne>/<z>/<x>/<y>.<ext>`) : on demande une tuile de la couche, si elle existe (getTile TMS). L'extension doit être en accord avec le format de la couche (jpg, png, pbf...)

Pour une requête WMTS ou WMS, le type de requête est défini par le paramètre REQUEST et les valeurs gérées sont :
* en WMS :
    - GetCapabilities
    - GetMap
    - GetFeatureInfo
* en WMTS :
    - GetCapabilities
    - GetTile
    - GetFeatureInfo

## Accès aux données

L'accès aux données stockées dans les pyramides se fait toujours par tuile. Dans le cas du TMS et WMTS, la requête doit contenir les indices (colonne et ligne) de la tuile voulue. La tuile est ensuite renvoyée sans traitement, ou avec simple ajout/modification de l'en-tête (en TIFF et en PNG). Dans le cas d'un GetMap en WMS, l'emprise demandée est convertie dans le système de coordonnées de la pyramide, et on identifie ainsi la liste des indices des tuiles requises pour calculée l'image voulue. De la même manière qu'en WMTS et TMS, le serveur sait à partir des indices où récupérer la donnée dans l'espace de stockage des pyramides.

Avec les indices de la tuile à lire, le serveur calcule le nom de la dalle qui la contient et le numéro de la tuile dans cette dalle. Le serveur commence par récupérer le header et l'index de la dalle, contenant les offsets et les tailles de toutes les tuiles de la dalle. Le header fait toujorus 2048 octets et l'index a une taille connue par le serveur.

Dans le cas du stockage objet (CEPH, S3, SWIFT), les objets symboliques ne font jamais plus de 2047 octets. Cette première lecture permet donc de les identifier (on lit moins que voulu). Dans ce cas, ce qu'on a lu contient le nom de l'objet contenant réellement la donnée. On va donc reproduire l'opération sur ce nouvel objet, qui lui ne doit pas être un objet symbolique (pas de lien en cascade). En mode fichier, ce mécanisme est transparent pour le serveur car géré par le système de fichiers.

Une fois que l'on a récupéré l'index, et grâce au numéro de la tuile dans la dalle, on va pouvoir connaître l'offset et la taille. On va donc faire une deuxième lecture de la dalle pour récupérer la donnée de la tuile.
